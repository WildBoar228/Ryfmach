#!/usr/bin/env bash

set -Eeuo pipefail

die() {
    echo "error: $*" >&2
    exit 1
}

usage() {
    cat >&2 <<'EOF'
Usage: install-release.sh ARCHIVE [RELEASE_NAME]

Validates and atomically installs a Ryfmach archive under
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

for command_name in awk file flock grep ldd mkdir mktemp mv python3 readlink rm \
    sha256sum tar
do
    command -v "$command_name" >/dev/null 2>&1 ||
        die "required command is unavailable: $command_name"
done

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
actual_checksum="$(sha256sum -- "$archive" | awk '{print $1}')"
[[ "${actual_checksum,,}" == "${expected_checksum,,}" ]] ||
    die "archive checksum does not match $checksum_file"

archive_root="$(
    python3 - "$archive" <<'PY'
import pathlib
import sys
import tarfile

try:
    with tarfile.open(sys.argv[1], "r:gz") as release:
        members = release.getmembers()
        if not members:
            raise ValueError("archive is empty")
        roots = set()
        for member in members:
            name = member.name
            path = pathlib.PurePosixPath(name)
            if (
                "\0" in name
                or "\n" in name
                or "\r" in name
                or path.is_absolute()
                or not path.parts
                or any(part in ("", ".", "..") for part in path.parts)
            ):
                raise ValueError(f"unsafe archive path: {name!r}")
            if not (member.isdir() or member.isfile()):
                raise ValueError(f"unsupported archive member: {name!r}")
            roots.add(path.parts[0])
        if len(roots) != 1:
            raise ValueError("archive must contain exactly one top-level directory")
except (OSError, tarfile.TarError, ValueError) as error:
    print(f"error: {error}", file=sys.stderr)
    raise SystemExit(1)

print(roots.pop())
PY
)" || die "release archive validation failed"

release_name="${release_name_argument:-$archive_root}"
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

required_paths=(
    "bin/ryfmach"
    "data/sound_compatibility.tsv"
    "frontend/static/templates/index.html"
    "python/config.py"
    "python/main.py"
    "python/requirements.txt"
    ".env.example"
    "VERSION"
    "SHA256SUMS"
)
for required_path in "${required_paths[@]}"; do
    [[ -e "$stage_dir/$required_path" ]] ||
        die "release is missing required path: $required_path"
done

[[ -x "$stage_dir/bin/ryfmach" ]] ||
    die "release binary is not executable"
[[ -s "$stage_dir/VERSION" ]] || die "release VERSION is empty"

python3 - "$stage_dir" <<'PY'
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
manifest = root / "SHA256SUMS"
pattern = re.compile(r"^[0-9a-fA-F]{64} [ *](.+)$")
listed = set()

try:
    for number, line in enumerate(
        manifest.read_text(encoding="utf-8").splitlines(), start=1
    ):
        match = pattern.fullmatch(line)
        if not match:
            raise ValueError(f"invalid SHA256SUMS line {number}")
        path = pathlib.PurePosixPath(match.group(1))
        parts = path.parts[1:] if path.parts[:1] == (".",) else path.parts
        if not parts or any(part in ("", ".", "..") for part in parts):
            raise ValueError(f"unsafe SHA256SUMS path on line {number}")
        normalized = pathlib.PurePosixPath(*parts).as_posix()
        if normalized == "SHA256SUMS" or normalized in listed:
            raise ValueError(f"invalid SHA256SUMS path: {normalized}")
        listed.add(normalized)

    actual = {
        path.relative_to(root).as_posix()
        for path in root.rglob("*")
        if path.is_file() and path.name != "SHA256SUMS"
    }
    if listed != actual:
        missing = sorted(actual - listed)
        extra = sorted(listed - actual)
        details = []
        if missing:
            details.append("unlisted files: " + ", ".join(missing))
        if extra:
            details.append("missing files: " + ", ".join(extra))
        raise ValueError("; ".join(details))
except (OSError, UnicodeError, ValueError) as error:
    print(f"error: {error}", file=sys.stderr)
    raise SystemExit(1)
PY

(
    cd -- "$stage_dir"
    sha256sum --check --strict SHA256SUMS
)

file "$stage_dir/bin/ryfmach" | grep -q ELF ||
    die "bin/ryfmach is not an ELF binary"
ldd_output="$(ldd "$stage_dir/bin/ryfmach" 2>&1)" ||
    die "ldd could not inspect bin/ryfmach: $ldd_output"
if grep -q 'not found' <<<"$ldd_output"; then
    printf '%s\n' "$ldd_output" >&2
    die "bin/ryfmach has unresolved shared-library dependencies"
fi

mv -- "$stage_dir" "$release_dir"
stage_dir=""

echo "Installed release: $release_dir"
echo "The release was not activated. Switch a site with switch-site-release.sh."
