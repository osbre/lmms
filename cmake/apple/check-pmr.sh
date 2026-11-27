#!/usr/bin/env bash

# Fails if any binary in the given build directory imports std::pmr symbols.
# Apple's libc++ only ships std::pmr starting with macOS 14 (see #8590).
# Usage: cmake/apple/check-pmr.sh <build-dir>

build_dir="${1:?Usage: $0 <build-dir>}"
found=0

while IFS= read -r -d '' file; do
	file -b "$file" | grep -q Mach-O || continue
	if nm -u "$file" | grep -q '3pmr'; then
		echo "::error::$file uses std::pmr, which requires macOS 14. Use MemoryResource.h instead."
		nm -u "$file" | grep '3pmr' | c++filt
		found=1
	fi
done < <(find "$build_dir" -type f \( -perm -u+x -o -name '*.so' -o -name '*.dylib' \) \
	-not -path '*/CMakeFiles/*' -print0)

exit "$found"
