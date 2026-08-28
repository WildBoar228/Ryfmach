#!/usr/bin/env bash

set -Eeuo pipefail

die() {
    echo "error: $*" >&2
    exit 1
}

usage() {
    cat >&2 <<'EOF'
Usage: install-release.sh ARCHIVE [RELEASE_NAME]

Checks and atomically installs a Ryfmach archive under
RYFMACH_RELEASES_DIR (default: $HOME/releases). The checksum is read from
ARCHIVE.sha256 unless RYFMACH_ARCHIVE_CHECKSUM_FILE is set.
EOF
}

[[ $# -ge 1 && $# -le 2 ]] || {
    usage
    exit 2
}

archive_argument="$1"
release_name_argument="${2:-}"
releases_argument="${RYFMACH_RELEASES_DIR:-$HOME/releases}"
checksum_argument="${RYFMACH_ARCHIVE_CHECKSUM_FILE:-${archive_argument}.sha256}"

[[ -f "$archive_argument" ]] || die "archive does not exist: $archive_argument"
[[ -f "$checksum_argument" ]] || die "checksum does not exist: $checksum_argument"

archive="$(readlink -f -- "$archive_argument")"
checksum_file="$(readlink -f -- "$checksum_argument")"
mkdir -p -- "$releases_argument"
releases_dir="$(cd -- "$releases_argument" && pwd -P)"

exec 9>"$releases_dir/.install.lock"
flock -n 9 || die "another release installation is already running"

read -r expected_checksum _ < "$checksum_file" ||
    die "cannot read archive checksum: $checksum_file"
[[ "$expected_checksum" =~ ^[[:xdigit:]]{64}$ ]] ||
    die "invalid SHA-256 value in $checksum_file"
checksum_output="$(sha256sum -- "$archive")"
actual_checksum="${checksum_output%% *}"
[[ "${actual_checksum,,}" == "${expected_checksum,,}" ]] ||
    die "archive checksum does not match $checksum_file"

archive_filename="$(basename -- "$archive")"
default_release_name="${archive_filename%.tar.gz}"
if [[ "$default_release_name" == "$archive_filename" ]]; then
    default_release_name="${archive_filename%.tgz}"
fi
release_name="${release_name_argument:-$default_release_name}"
[[ "$release_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] ||
    die "invalid release name: $release_name"
release_dir="$releases_dir/$release_name"
[[ ! -e "$release_dir" && ! -L "$release_dir" ]] ||
    die "release already exists: $release_dir"

stage_dir="$(mktemp -d "$releases_dir/.install-${release_name}.XXXXXX")"
cleanup() {
    local exit_status=$?
    if [[ -n "${stage_dir:-}" ]]; then
        case "$stage_dir" in
            "$releases_dir"/.install-*) rm -rf -- "$stage_dir" ;;
            *) echo "warning: refusing to remove unexpected path: $stage_dir" >&2 ;;
        esac
    fi
    exit "$exit_status"
}
trap cleanup EXIT

tar --extract --gzip --file "$archive" --directory "$stage_dir" \
    --strip-components 1 --no-same-owner --no-same-permissions \
    --delay-directory-restore

if [[ "$release_name" == "maintenance" ]]; then
    required_files=(
        "frontend/static/css/style.css"
        "frontend/static/img/logo.png"
        "frontend/static/public/favicon.ico"
        "frontend/static/templates/maintenance.html"
        "python/config.py"
        "python/main.py"
        "python/requirements.txt"
        "VERSION"
    )
else
    required_files=(
        "bin/ryfmach"
        "frontend/static/public/favicon.ico"
        "frontend/static/public/sitemap.xml"
        "frontend/static/templates/index.html"
        "python/config.py"
        "python/main.py"
        "python/morphemics/Algo1.py"
        "python/morphemics/Algo2.py"
        "python/morphemics/finalAlgo.py"
        "python/requirements.txt"
    )
fi
for required_file in "${required_files[@]}"; do
    [[ -f "$stage_dir/$required_file" ]] ||
        die "release is missing required file: $required_file"
done

if [[ "$release_name" != "maintenance" ]]; then
    [[ -x "$stage_dir/bin/ryfmach" ]] ||
        die "release binary is not executable"
fi

mv -- "$stage_dir" "$release_dir"
stage_dir=""

echo "Installed release: $release_dir"
echo "The release was not activated. Switch a site with switch-site-release.sh."
