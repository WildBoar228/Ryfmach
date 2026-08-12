#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
site_argument="${1:-}"
env_argument="${2:-}"

[[ -n "$site_argument" ]] || {
    echo "usage: $0 SITE_DIR [ENV_FILE]" >&2
    exit 2
}

"$SCRIPT_DIR/stop-site.sh" "$site_argument" "$env_argument"
"$SCRIPT_DIR/start-site.sh" "$site_argument" "$env_argument"
