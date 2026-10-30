#!/usr/bin/env bash
# Build port/ (doa4) on Linux x86-64 (the Linux counterpart of tools/build.ps1; from
# graw360-recomp's and doax2-360-recomp's) against the SDK tools/setup-linux.sh built. Codegen
# (translating default.xex to C++) runs as part of the build whenever doa4_manifest.toml or the
# executable changed; the first run takes a while.
#   tools/build-linux.sh                         # RelWithDebInfo
#   tools/build-linux.sh --config Release --jobs 12
# DOA4_SDK_DIR: the SDK checkout tools/setup-linux.sh used; DOA4_BUILD_DIR: the build folder
# (default port/out/build/<preset>). tools/build-deck.sh sets both.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
config=RelWithDebInfo
jobs=$(( $(nproc) > 2 ? $(nproc) - 2 : 1 ))
while [[ $# -gt 0 ]]; do
  case "$1" in
    --config) config="$2"; shift ;;
    --jobs) jobs="$2"; shift ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
  shift
done
case "$config" in
  Debug|Release|RelWithDebInfo) ;;
  *) echo "unknown configuration $config" >&2; exit 2 ;;
esac

port="$root/port"
sdk_src="${DOA4_SDK_DIR:-$root/third_party/rexglue-sdk-bd833a2}"
sdk_install="$sdk_src/out/install/linux-amd64"
[[ -x "$sdk_install/bin/rexglue" ]] || { echo "The SDK isn't built: run tools/setup-linux.sh" >&2; exit 1; }
xex="$port/assets/default.xex"
[[ -f "$xex" ]] || { echo "port/assets/default.xex is missing (docs/SETUP.md)" >&2; exit 1; }
if [[ "$(sha256sum "$xex" | cut -d' ' -f1)" != 0dba23465e091a03c119a9fb06765ac9cd51e7c14cc22e2130f0c11bf28e7530 ]]; then
  echo "port/assets/default.xex is not the supported Dead or Alive 4 executable (Rev 1 USA/Europe, title 544307D1)" >&2
  exit 1
fi

preset="linux-amd64-${config,,}"
# x86-64-v3 as on Windows (tools/build.ps1); the Steam Deck has it. The presets name clang-20;
# any clang 20 or later works.
build="${DOA4_BUILD_DIR:-$port/out/build/$preset}"
args=(--preset "$preset" -B "$build" -DCMAKE_PREFIX_PATH="$sdk_install"
      -DCMAKE_C_COMPILER="${CC:-clang}" -DCMAKE_CXX_COMPILER="${CXX:-clang++}"
      -DCMAKE_C_FLAGS=-march=x86-64-v3 -DCMAKE_CXX_FLAGS=-march=x86-64-v3)
cd "$port"
cmake "${args[@]}"
first_time=0
[[ -f generated/default/sources.cmake ]] || first_time=1
# Codegen first (a no-op when nothing changed), then reconfigure once if the generated sources
# didn't exist yet (the recomp target only appears once they do), then build.
cmake --build "$build" --parallel "$jobs" --target doa4_codegen
# The installed code generator is the patched one; the check keeps a stock generator from
# bringing the vpkd3d128 bug back unnoticed.
python3 -I "$root/tools/check_vector_aliasing.py" generated/default >/dev/null || {
  echo "The generated code has the vpkd3d128 aliasing bug: is the SDK patched? (tools/setup-linux.sh)" >&2
  exit 1
}
if [[ $first_time -eq 1 ]]; then cmake "${args[@]}"; fi
cmake --build "$build" --parallel "$jobs"
# The program finds its shared libraries beside itself ($ORIGIN): the SDK stages the renderer
# plugin there, but its runtime and Tracy libraries only on Windows and macOS.
postfix=$(case "$config" in Debug) echo d ;; RelWithDebInfo) echo rd ;; *) echo ;; esac)
for lib in librexruntime libTracyClient; do
  [[ -f "$sdk_install/lib/$lib$postfix.so" ]] && cp -u "$sdk_install/lib/$lib$postfix.so" "$build/"
done
echo "Built $build/doa4"
