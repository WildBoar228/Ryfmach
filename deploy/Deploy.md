# Ryfmach deployment plan

This deployment targets restricted CloudLinux hosting where systemd and the
global Apache/Nginx configuration are unavailable to the account. The hosting
platform starts one site-level master process. That process must run both the
C++ API and Gunicorn.

Production and test use the same immutable release files, but have separate
environment files, ports, logs, Flask secrets, and mutable databases.

## Directory structure

All paths below are inside the hosting account's home directory.

```text
~/
├── config/
│   ├── ryfmach.by.env
│   └── ryfmach.xyz.env
├── db/
│   ├── shared/
│   │   └── Slounik5.db
│   ├── ryfmach.by/
│   │   └── RhymeLikes.db
│   └── ryfmach.xyz/
│       └── RhymeLikes.db
├── logs/
│   ├── ryfmach.by/
│   └── ryfmach.xyz/
├── bin/
│   └── ryfmach-deploy/
│       ├── install-release.sh
│       ├── restart-site.sh
│       ├── site-common.sh
│       ├── site-runner.sh
│       ├── start-site.sh
│       ├── stop-site.sh
│       └── switch-site-release.sh
├── releases/
│   ├── 2026_.../
│   └── maintenance/
└── www/
    ├── ryfmach.by/
    │   ├── .venv/
    │   ├── server.py
    │   └── Ryfmach -> ~/releases/<production-release>
    └── ryfmach.xyz/
        ├── .venv/
        ├── server.py
        └── Ryfmach -> ~/releases/<candidate-release>
```

The configuration, database, and log directories must not be exposed as web
document roots.

Recommended permissions:

```bash
chmod 700 ~/config ~/db ~/logs
chmod 600 ~/config/ryfmach.by.env ~/config/ryfmach.xyz.env
chmod 600 ~/db/ryfmach.by/RhymeLikes.db ~/db/ryfmach.xyz/RhymeLikes.db
```

## Release format

Each release directory is immutable after it passes staging checks.

```text
release-directory/
├── bin/
│   └── ryfmach
├── data/
│   └── sound_compatibility.tsv
├── frontend/
│   └── static/
│       ├── css/
│       ├── img/
│       ├── js/
│       └── templates/
├── python/
│   ├── config.py
│   ├── main.py
│   └── requirements.txt
├── .env.example
└── VERSION
```

A release must not contain or receive a site `.env`, mutable database, or log
file. Do not copy `.env` into a candidate or current release: the same release
can be used by test and production at the same time.

The maintenance release contains the minimum Python and static files needed to
serve `maintenance.html`. Its application must return HTTP 503 and a
`Retry-After` header for application pages. It does not start the C++ API.

## Site environment

`site-runner.sh` loads exactly one site environment before starting any
children. Environment variables are process-local and inherited only by that
site's Gunicorn and C++ processes. They must not be exported globally from the
account's shell profile.

The launcher loads:

```text
~/config/<site-directory-name>.env
```

For example, running the scripts for `~/www/ryfmach.by` loads
`~/config/ryfmach.by.env`. These files are sourced by Bash, so they must contain
shell-compatible `NAME=value` assignments and must be writable only by the
hosting account. Do not define hosting-owned variables such as `PORT` in these
files.

The release's `python/config.py` consumes the inherited environment. It must
not search for `.env` relative to the release directory.

### Production example

```dotenv
RYFMACH_HOST_NAME=127.0.0.1
RYFMACH_API_HOST=127.0.0.1
RYFMACH_API_PORT=8081
RYFMACH_JINJA_PORT=8082

RYFMACH_SOUND_COMPATIBILITY_PATH=/home/USERNAME/www/ryfmach.by/Ryfmach/data/sound_compatibility.tsv
SLOUNIK_DB_PATH=/home/USERNAME/db/shared/Slounik5.db
RHYME_LIKES_DB_PATH=/home/USERNAME/db/ryfmach.by/RhymeLikes.db

FLASK_SECRET_KEY=replace-with-a-persistent-random-production-secret
PUBLIC_BASE_URL=https://ryfmach.by

RYFMACH_APP_LOG_PATH=/home/USERNAME/logs/ryfmach.by/app.log
RYFMACH_API_LOG_PATH=/home/USERNAME/logs/ryfmach.by/api.log
RYFMACH_WEB_LOG_PATH=/home/USERNAME/logs/ryfmach.by/web.log
RYFMACH_API_START_TIMEOUT=30
RYFMACH_WEB_START_TIMEOUT=30
RYFMACH_SITE_START_TIMEOUT=70
RYFMACH_SITE_STOP_TIMEOUT=20
```

### Test example

```dotenv
RYFMACH_HOST_NAME=127.0.0.1
RYFMACH_API_HOST=127.0.0.1
RYFMACH_API_PORT=8181
RYFMACH_JINJA_PORT=8182

RYFMACH_SOUND_COMPATIBILITY_PATH=/home/USERNAME/www/ryfmach.xyz/Ryfmach/data/sound_compatibility.tsv
SLOUNIK_DB_PATH=/home/USERNAME/db/shared/Slounik5.db
RHYME_LIKES_DB_PATH=/home/USERNAME/db/ryfmach.xyz/RhymeLikes.db

FLASK_SECRET_KEY=replace-with-a-persistent-random-test-secret
PUBLIC_BASE_URL=https://ryfmach.xyz

RYFMACH_APP_LOG_PATH=/home/USERNAME/logs/ryfmach.xyz/app.log
RYFMACH_API_LOG_PATH=/home/USERNAME/logs/ryfmach.xyz/api.log
RYFMACH_WEB_LOG_PATH=/home/USERNAME/logs/ryfmach.xyz/web.log
RYFMACH_API_START_TIMEOUT=30
RYFMACH_WEB_START_TIMEOUT=30
RYFMACH_SITE_START_TIMEOUT=70
RYFMACH_SITE_STOP_TIMEOUT=20
```

Production and test must use different API ports, Flask secrets, mutable
databases, and log paths. The read-only dictionary database may be shared.

Flask must connect to `RYFMACH_API_HOST:RYFMACH_API_PORT`; it must not
hard-code port 8081. Canonical URLs and sitemap generation should use
`PUBLIC_BASE_URL`.

Flask writes its application log to `RYFMACH_APP_LOG_PATH` using
`WatchedFileHandler`, allowing an external tool to rotate it. The launcher
writes C++ and Gunicorn stdout/stderr to `RYFMACH_API_LOG_PATH` and
`RYFMACH_WEB_LOG_PATH`. Log rotation remains the responsibility of the hosting
platform or a user cron job.

## Master process contract

The hosting platform must start one master launcher per site and restart it
after reboot or unexpected exit.

The master is `site-runner.sh`. It starts:

1. `Ryfmach/bin/ryfmach` with the site environment.
2. `server.py`, which only configures and starts Gunicorn for
   `Ryfmach/python/main.py`.

The master must:

- acquire a per-site singleton lock before starting;
- start the C++ API only once;
- wait for the C++ `/health` endpoint before starting Gunicorn;
- forward TERM, INT, and HUP signals to both children;
- stop the remaining child if either child exits;
- wait for both children so no zombie processes remain;
- exit non-zero after an unexpected child failure so the hosting platform can
  restart the complete site;
- redirect child stdout and stderr to the site-specific environment-controlled
  log paths.

Do not start `bin/ryfmach` as an unowned background process from an
interactive SSH deployment session.

Do not add C++ process management to `server.py`, and do not spawn the C++ API
from `python/main.py`. Keeping both children under the shell runner ensures
that Gunicorn never starts duplicate API instances.

The maintenance release is a special case: the launcher starts only Gunicorn
when the resolved release is `~/releases/maintenance`.

## Site management scripts

Install all files from `deploy/scripts/` together, for example in
`~/bin/ryfmach-deploy/`, and preserve their executable permissions. The scripts
require Bash 4.3 or newer plus GNU `flock`, `setsid`, `readlink`, `mv`, `tar`,
and `sha256sum`, all normally supplied by the EL9 base system.

Start a detached site from SSH:

```bash
~/bin/ryfmach-deploy/start-site.sh ~/www/ryfmach.xyz
```

If the hosting panel can execute a shell command and expects the master to stay
in the foreground, configure it to run:

```bash
~/bin/ryfmach-deploy/start-site.sh --foreground ~/www/ryfmach.by
```

Stop or restart a site:

```bash
~/bin/ryfmach-deploy/stop-site.sh ~/www/ryfmach.xyz
~/bin/ryfmach-deploy/restart-site.sh ~/www/ryfmach.xyz
```

The detached commands are intended for sites managed directly from SSH. Do not
mix detached startup with a hosting panel that is already supervising the
foreground runner. For a panel-managed site, use the panel's restart action so
it replaces the foreground master cleanly.

Atomically switch a site and restart it:

```bash
~/bin/ryfmach-deploy/switch-site-release.sh \
    ~/www/ryfmach.xyz \
    ~/releases/2026_08_12_VERSION
```

The switch command waits for the new API and Gunicorn to become reachable. If
startup fails, it restores the old symlink and starts the old release again.
Each site's `Ryfmach` symlink points directly to the release used by that site.

An explicit environment-file path may be supplied as the final argument to any
command. Otherwise the scripts derive `~/config/<site-name>.env`.

## Preparing a release

GitHub CI builds and tests the release in an EL9-compatible environment. The
produced archive must have a SHA-256 checksum.

On the production server:

```bash
~/bin/ryfmach-deploy/install-release.sh \
    ~/incoming/ryfmach-COMMIT-el9-x86_64.tar.gz
```

Keep the workflow-generated `.tar.gz.sha256` beside the archive. The installer
derives the release-directory name from the archive filename by removing
`.tar.gz` or `.tgz`. An optional second argument overrides that name. Set
`RYFMACH_RELEASES_DIR` only when releases do not live in `~/releases`.

The installer verifies the archive checksum, extracts into a temporary
directory, checks the main runtime files, and renames the directory into place
atomically. It never changes a site symlink, starts a process, copies a site
environment, or overwrites an existing release. Release archives are trusted
to come from this repository's CI workflow rather than an untrusted source.

1. Acquire a deployment lock so two deployments cannot run concurrently.
2. Upload the archive into a temporary incoming directory.
3. Verify the archive checksum.
4. Extract into a new, uniquely named release directory. Never extract over an
   existing release.
5. Verify the required binary, Python, requirements, and frontend files.
6. Verify that `bin/ryfmach` is executable.
7. Switch the site with `switch-site-release.sh`. Before changing the release
   symlink, it installs the candidate's `python/requirements.txt` into the
   site's existing `.venv` using the configured package index.

For example:

```bash
~/bin/ryfmach-deploy/switch-site-release.sh \
    ~/www/ryfmach.xyz \
    ~/releases/ryfmach-COMMIT-el9-x86_64
```

The switch may download packages, so production dependency installation
requires outbound network access. If installation fails, the release symlink
is not changed. Binary and dependency compatibility is confirmed when the
candidate site is started and tested.

The current per-site virtual environments are acceptable while dependencies
remain backward-compatible, but mutating a shared venv can break rollback. If
requirements begin changing between releases, use a versioned venv per release
or per requirements hash and retain the previous venv with the previous
release.

## Staging and promotion pipeline

1. Run GitHub CI and produce an EL9 release archive and checksum.
2. Prepare the new release using the checks above.
3. Record the existing target of `~/www/ryfmach.xyz/Ryfmach`.
4. Atomically point the test site's `Ryfmach` symlink directly to the
   candidate release.
5. Restart the `ryfmach.xyz` master launcher.
6. Verify from SSH that the test C++ API is healthy on its private port.
7. Verify the test site's `/`, `/phonetics`, and `/morphemics` pages.
8. Send at least one functional request through the public test `/api`
   route.
9. Verify that a test like/dislike writes only to the test database.
10. Switch `~/www/ryfmach.by/Ryfmach` directly to the candidate release with
    `switch-site-release.sh`; it restarts the production site and rolls back
    automatically if startup fails.
11. Verify from SSH that the production C++ API is healthy on its private port.
12. Verify the production pages and send a functional request through the
    public production `/api` route.
13. If a later production check fails, switch the production site back to its
    recorded previous release and repeat the health checks.

Create a temporary symlink and rename it over the destination for atomic
switches. Do not unlink the active symlink before creating its replacement.

Switching a symlink alone does not update already running Gunicorn workers or
the C++ process; the site master must always be restarted after a switch.

## Database handling

`Slounik5.db` is treated as read-only and may be shared only while its schema
is compatible with both the current and candidate binaries.

`RhymeLikes.db` is mutable and must be different for production and test.
Never package it in a release or replace the production copy during deployment.

Before a release that changes a database schema:

1. Create an SQLite-consistent backup rather than copying a live database file.
2. Document whether the migration is backward-compatible.
3. Test the migration against a copy through the test site.
4. Define the database rollback procedure before promoting the release.

## Maintenance mode

1. Record the current target of `~/www/ryfmach.by/Ryfmach`.
2. Atomically point `~/www/ryfmach.by/Ryfmach` directly to
   `~/releases/maintenance`.
3. Restart the production master. It must start only the maintenance Gunicorn
   application and stop the production C++ API.
4. Verify that application URLs return HTTP 503 with `Retry-After`, while the
   required static assets remain available.
5. Perform the maintenance work.
6. Atomically restore `~/www/ryfmach.by/Ryfmach` to its recorded release.
7. Restart the production master again.
8. Verify the C++ health endpoint, production pages, and one functional public
   API request.

## Rollback and retention

Always retain the previous production release until the new release has been
verified in production. Retain at least the last three known-good releases and
their compatible Python environments.

A normal rollback consists of:

1. Switch `~/www/ryfmach.by/Ryfmach` to the recorded previous release.
2. Restore its compatible Python environment if dependencies changed.
3. Restart the production master.
4. Verify the private C++ health endpoint and public application routes.

Release cleanup must never remove:

- the target currently used by the production site;
- the target currently used by the test site;
- the maintenance release;
- a release required for an active database rollback plan.
