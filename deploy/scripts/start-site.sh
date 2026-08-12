#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=site-common.sh
source "$SCRIPT_DIR/site-common.sh"

foreground=0
if [[ "${1:-}" == "--foreground" ]]; then
    foreground=1
    shift
fi

resolve_site "${1:-}" "${2:-}"
load_site_environment

require_variable RYFMACH_API_LOG_PATH
require_variable RYFMACH_WEB_LOG_PATH

mkdir -p \
    "$STATE_DIR" \
    "$(dirname -- "$RYFMACH_API_LOG_PATH")" \
    "$(dirname -- "$RYFMACH_WEB_LOG_PATH")"

if existing_pid="$(runner_pid 2>/dev/null)" &&
   runner_is_alive "$existing_pid"; then
    die "site is already running with PID $existing_pid"
fi

rm -f -- "$STATE_DIR/runner.pid" "$STATE_DIR/ready"

if ((foreground == 1)); then
    exec "$SCRIPT_DIR/site-runner.sh" "$SITE_DIR" "$ENV_FILE"
fi

command -v setsid >/dev/null || die "setsid is required for detached startup"

nohup setsid "$SCRIPT_DIR/site-runner.sh" "$SITE_DIR" "$ENV_FILE" \
    >>"$RYFMACH_WEB_LOG_PATH" 2>&1 </dev/null &
started_pid=$!

start_timeout="${RYFMACH_SITE_START_TIMEOUT:-70}"
deadline=$((SECONDS + start_timeout))
while ((SECONDS < deadline)); do
    if [[ -f "$STATE_DIR/ready" ]]; then
        echo "started $SITE_NAME with runner PID $started_pid"
        exit 0
    fi
    if ! runner_is_alive "$started_pid"; then
        echo "site failed to start; inspect $RYFMACH_WEB_LOG_PATH" >&2
        exit 1
    fi
    sleep 0.25
done

kill -TERM "$started_pid" 2>/dev/null || true
echo "site did not become ready within ${start_timeout}s" >&2
exit 1
