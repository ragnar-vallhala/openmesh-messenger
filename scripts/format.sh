#!/usr/bin/env bash
# Format all C/C++ sources in the repo with clang-format (uses .clang-format).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

mapfile -t files < <(find libs client servers tools \
    -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
    -not -path '*/build/*')

if [[ ${#files[@]} -eq 0 ]]; then
    echo "no sources found"
    exit 0
fi

clang-format -i "${files[@]}"
echo "formatted ${#files[@]} files"
