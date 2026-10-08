#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir=/workspace/.runner-tools
export LD_LIBRARY_PATH="$tools_dir/sysroot/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export XDG_RUNTIME_DIR="$tools_dir/runtime"
export MESA_SHADER_CACHE_DIR="$tools_dir/mesa-cache"
mkdir -p "$XDG_RUNTIME_DIR" "$MESA_SHADER_CACHE_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
# Cloud graphics validation uses Mesa software rendering, not a lab GPU.
export LIBGL_ALWAYS_SOFTWARE=1
xvfb_pid=''
run_dir=''
cleanup() {
  if [[ -n "$xvfb_pid" ]]; then kill "$xvfb_pid" 2>/dev/null || true; wait "$xvfb_pid" 2>/dev/null || true; fi
  if [[ -n "$run_dir" ]]; then rm -rf -- "$run_dir"; fi
}
trap cleanup EXIT
if [[ -z "${DISPLAY:-}" ]]; then
  run_dir="$(mktemp -d "$tools_dir/runtime/display.XXXXXX")"
  "$tools_dir/sysroot/usr/bin/Xvfb" -displayfd 3 -screen 0 1280x720x24 -nolisten tcp \
    3>"$run_dir/number" >"$run_dir/xvfb.log" 2>&1 &
  xvfb_pid=$!
  for ((attempt=0; attempt<50; ++attempt)); do
    if [[ -s "$run_dir/number" ]]; then break; fi
    if ! kill -0 "$xvfb_pid" 2>/dev/null; then cat "$run_dir/xvfb.log" >&2; exit 1; fi
    sleep 0.1
  done
  if [[ ! -s "$run_dir/number" ]]; then cat "$run_dir/xvfb.log" >&2; echo 'Xvfb startup timed out' >&2; exit 1; fi
  export DISPLAY=":$(cat "$run_dir/number")"
fi
cd "$repo_dir"
"$repo_dir/build/endless_runner" "$@"
