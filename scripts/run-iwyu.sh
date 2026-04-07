#!/usr/bin/env bash
# run-iwyu.sh — run include-what-you-use across the bust project
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
IWYU_TOOL="${IWYU_TOOL:-iwyu_tool.py}"   # ships with IWYU
MAPPING_FILE="${MAPPING_FILE:-}"          # optional .imp mapping file

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "No compile_commands.json at $BUILD_DIR — run 'cmake --preset mp' first." >&2
  exit 1
fi

ARGS=(-p "$BUILD_DIR" -j "$(sysctl -n hw.ncpu)")
# Filter to bust/ sources, skipping _deps/ and *_test.cpp files
while IFS= read -r src; do
  ARGS+=("$src")
done < <(find bust -type f \( -name '*.cpp' -o -name '*.cc' \) ! -name '*_test.cpp' ! -path '*/_deps/*')

IWYU_ARGS=(
  -Xiwyu --no_fwd_decls
  -Xiwyu --cxx17ns
  -Xiwyu --quoted_includes_first
)
[[ -n "$MAPPING_FILE" ]] && IWYU_ARGS+=(-Xiwyu --mapping_file="$MAPPING_FILE")

"$IWYU_TOOL" "${ARGS[@]}" -- "${IWYU_ARGS[@]}"
