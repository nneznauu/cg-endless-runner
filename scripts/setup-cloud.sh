#!/usr/bin/env bash
# Rootless setup for the Debian 13 Codex cloud image. No system files are changed.
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir="/workspace/.runner-tools"
. /etc/os-release
if [[ "${ID:-}" != debian || "${VERSION_CODENAME:-}" != trixie ]]; then
  echo 'This helper supports Debian 13 (trixie). Use the README for other systems.' >&2
  exit 1
fi
mkdir -p "$tools_dir/apt/lists-current/partial" "$tools_dir/apt/cache/archives/partial" "$tools_dir/apt/empty" \
         "$tools_dir/debs" "$tools_dir/sysroot" "$tools_dir/runtime" "$tools_dir/mesa-cache"
chmod 700 "$tools_dir/runtime"
cat > "$tools_dir/apt/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
EOF
# Read a local configuration; avoid global cleanup hooks that require root.
cat > "$tools_dir/apt/local.conf" <<EOF
Dir::Etc::parts "$tools_dir/apt/empty";
Dir::Etc::main "/dev/null";
Dir::Etc::sourcelist "$tools_dir/apt/sources.list";
Dir::Etc::sourceparts "$tools_dir/apt/empty";
Dir::State::lists "$tools_dir/apt/lists-current";
Dir::Cache "$tools_dir/apt/cache";
Debug::NoLocking "true";
EOF
export APT_CONFIG="$tools_dir/apt/local.conf"
apt-get update
(
  cd "$tools_dir/debs"
  # apt verifies the signed repository metadata and package hashes.
  apt-get download libglut-dev libglut3.12 libglm-dev libglu1-mesa-dev \
    libgl-dev libgl1 libglx-dev libglx0 libopengl-dev libopengl0 \
    xvfb xserver-common libxfont2 libfontenc1 libxcursor1 libxinerama1 \
    libxrandr2 libxi6 x11-xkb-utils
  for package in ./*.deb; do dpkg-deb -x "$package" "$tools_dir/sysroot"; done
)
if [[ ! -x "$tools_dir/venv/bin/python" ]]; then python3 -m venv "$tools_dir/venv"; fi
"$tools_dir/venv/bin/pip" install --disable-pip-version-check --no-cache-dir \
  cmake==3.31.6 ninja==1.11.1.4 glad==0.1.36 python-docx==1.1.2 Pillow==11.1.0
"$tools_dir/venv/bin/python" -m glad --profile core --api gl=3.3 --generator c \
  --extensions '' --out-path "$tools_dir/glad" --reproducible
"$tools_dir/venv/bin/cmake" -S "$repo_dir" -B "$repo_dir/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="$tools_dir/venv/bin/ninja" \
  -DCMAKE_PREFIX_PATH="$tools_dir/sysroot/usr" -DGLAD_ROOT="$tools_dir/glad"
"$tools_dir/venv/bin/cmake" --build "$repo_dir/build" -j 2
"$tools_dir/venv/bin/ctest" --test-dir "$repo_dir/build" --output-on-failure
bash "$repo_dir/scripts/cloud-run.sh" --smoke-test --screenshot "$repo_dir/artifacts/cloud-smoke.ppm"
