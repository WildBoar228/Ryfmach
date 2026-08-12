#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=site-common.sh
source "$SCRIPT_DIR/site-common.sh"

site_argument="${1:-}"
release_argument="${2:-}"
env_argument="${3:-}"

[[ -n "$site_argument" && -n "$release_argument" ]] ||
    die "usage: $0 SITE_DIR RELEASE_DIR [ENV_FILE]"

resolve_site "$site_argument" "$env_argument"
[[ -d "$release_argument" ]] || die "release directory does not exist: $release_argument"

RELEASE_TARGET="$(cd -- "$release_argument" && pwd -P)"
[[ -f "$RELEASE_TARGET/python/main.py" ]] ||
    die "release has no python/main.py: $RELEASE_TARGET"

mkdir -p "$STATE_DIR"
exec 8>"$STATE_DIR/deploy.lock"
flock -n 8 || die "another release switch is active for $SITE_DIR"

switch_link="$RELEASE_LINK"
site_link_target=""
if [[ -L "$RELEASE_LINK" ]]; then
    site_link_target="$(readlink -- "$RELEASE_LINK")"
    if [[ "$site_link_target" = /* ]]; then
        site_link_path="$site_link_target"
    else
        site_link_path="$(readlink -m -- "$SITE_DIR/$site_link_target")"
    fi

    # Production normally has Ryfmach -> ~/releases/current. Preserve that
    # stable site link and atomically update the current link instead.
    if [[ "$(basename -- "$site_link_path")" == "current" &&
          -L "$site_link_path" ]]; then
        switch_link="$site_link_path"
    fi
fi

previous_target=""
if [[ -L "$switch_link" ]]; then
    previous_target="$(readlink -- "$switch_link")"
elif [[ -e "$switch_link" ]]; then
    die "$switch_link exists but is not a symlink"
fi

switch_directory="$(dirname -- "$switch_link")"
temporary_link="$switch_directory/.$(basename -- "$switch_link").$$.tmp"
cleanup_temporary_link() {
    rm -f -- "$temporary_link"
}
trap cleanup_temporary_link EXIT

ln -s -- "$RELEASE_TARGET" "$temporary_link"
mv -Tf -- "$temporary_link" "$switch_link"

if "$SCRIPT_DIR/restart-site.sh" "$SITE_DIR" "$ENV_FILE"; then
    echo "switched $SITE_NAME to $RELEASE_TARGET"
    exit 0
fi

echo "new release failed to start; restoring the previous release" >&2
if [[ -n "$previous_target" ]]; then
    ln -s -- "$previous_target" "$temporary_link"
    mv -Tf -- "$temporary_link" "$switch_link"
    "$SCRIPT_DIR/restart-site.sh" "$SITE_DIR" "$ENV_FILE" ||
        die "rollback release also failed to start"
else
    rm -f -- "$switch_link"
fi

die "release switch was rolled back"
