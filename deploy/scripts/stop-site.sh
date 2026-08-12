#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=site-common.sh
source "$SCRIPT_DIR/site-common.sh"

resolve_site "${1:-}" "${2:-}"
mkdir -p "$STATE_DIR"

if ! pid="$(runner_pid 2>/dev/null)"; then
    echo "$SITE_NAME is not running"
    rm -f -- "$STATE_DIR/runner.pid" "$STATE_DIR/ready"
    exit 0
fi

if ! runner_is_alive "$pid"; then
    echo "$SITE_NAME has a stale runner PID file"
    rm -f -- "$STATE_DIR/runner.pid" "$STATE_DIR/ready"
    exit 0
fi

runner_matches_site "$pid" ||
    die "PID $pid does not look like the runner for $SITE_DIR; refusing to kill it"

kill -TERM "$pid"

stop_timeout="${RYFMACH_SITE_STOP_TIMEOUT:-20}"
deadline=$((SECONDS + stop_timeout))
while runner_is_alive "$pid" && ((SECONDS < deadline)); do
    sleep 0.25
done

if runner_is_alive "$pid"; then
    echo "runner did not stop in ${stop_timeout}s; killing its process group" >&2
    kill -KILL -- "-$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null || true
fi

rm -f -- "$STATE_DIR/runner.pid" "$STATE_DIR/ready"
echo "stopped $SITE_NAME"
