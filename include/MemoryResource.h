/*
 * MemoryResource.h - minimal replacement for std::pmr
 *
 * Copyright (c) 2026 Ostap Brehin <osbre/at/protonmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_MEMORY_RESOURCE_H
#define LMMS_MEMORY_RESOURCE_H

#include <cstddef>
#include <new>
#include <vector>

// std::pmr requires macOS 14 with Apple's libc++. These mirror the subset of it we use
// and can be replaced by std::pmr if macOS 14 becomes our target.

namespace lmms
{

//! Like std::pmr::memory_resource
class MemoryResource
{
public:
	virtual ~MemoryResource() = default;

	void* allocate(std::size_t bytes, std::size_t alignment) { return do_allocate(bytes, alignment); }
	void deallocate(void* p, std::size_t bytes, std::size_t alignment) { do_deallocate(p, bytes, alignment); }
	bool is_equal(const MemoryResource& other) const noexcept { return this == &other || do_is_equal(other); }

private:
	virtual void* do_allocate(std::size_t bytes, std::size_t alignment) = 0;
	virtual void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) = 0;
	virtual bool do_is_equal(const MemoryResource& other) const noexcept = 0;
};

//! Like std::pmr::new_delete_resource()
inline auto newDeleteResource() noexcept -> MemoryResource*
{
	class NewDeleteResource final : public MemoryResource
	{
		void* do_allocate(std::size_t bytes, std::size_t alignment) override
		{
			return ::operator new(bytes, std::align_val_t{alignment});
		}
		void do_deallocate(void* p, std::size_t, std::size_t alignment) override
		{
			// Sized deallocation isn't available everywhere (e.g. MinGW's libc++)
			::operator delete(p, std::align_val_t{alignment});
		}
		bool do_is_equal(const MemoryResource& other) const noexcept override { return this == &other; }
	};

	static auto resource = NewDeleteResource{};
	return &resource;
}

//! Like std::pmr::polymorphic_allocator
template<typename T>
class PolymorphicAllocator
{
public:
	using value_type = T;

	PolymorphicAllocator(MemoryResource* resource = newDeleteResource()) noexcept : m_resource{resource} {}

	template<typename U>
	PolymorphicAllocator(const PolymorphicAllocator<U>& other) noexcept : m_resource{other.resource()} {}

	T* allocate(std::size_t n) { return static_cast<T*>(m_resource->allocate(n * sizeof(T), alignof(T))); }
	void deallocate(T* p, std::size_t n) noexcept { m_resource->deallocate(p, n * sizeof(T), alignof(T)); }

	auto resource() const noexcept -> MemoryResource* { return m_resource; }

	template<typename U>
	bool operator==(const PolymorphicAllocator<U>& other) const noexcept
	{
		return m_resource->is_equal(*other.resource());
	}

private:
	MemoryResource* m_resource;
};

//! Like std::pmr::vector
template<typename T>
using PmrVector = std::vector<T, PolymorphicAllocator<T>>;

} // namespace lmms

#endif // LMMS_MEMORY_RESOURCE_H
