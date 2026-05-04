# Skill: Platform, Build, and Legal Boundaries

This project runs on a hardened FreeBSD-based appliance and inherits both technical and legal constraints from that environment. Read this skill before adding dependencies, wiring CI, or assuming a feature exists.

## 1. Legal: pfSense **CE only**

- This package targets **pfSense Community Edition**. It is published as open source and **must not** include or reference proprietary pfSense Plus source, internals, or paid-only behaviors.
- If you cannot find the symbol, file, or feature in pfSense CE source (https://github.com/pfsense/pfsense), you cannot use it here.
- When in doubt, stop and verify CE availability before writing the code.
- Security vulnerabilities are reported privately to a maintainer, not via public issues or PRs (see `docs/index.md`).

## 2. Build environment

The package is a FreeBSD package (`.pkg`) installed at `/usr/local/pkg/RESTAPI/` on the pfSense host. Building it from source therefore requires FreeBSD.

### Easiest path: VirtualBox + Vagrant

```bash
sh vagrant-build.sh
# Output: pfSense-pkg-RESTAPI-<version>.pkg in the repo root
```

Optional environment overrides:

- `FREEBSD_VERSION` — Vagrant box (default `freebsd/FreeBSD-14.0-CURRENT`).
- `BUILD_VERSION` — package version tag (default `0.0_0-dev`).

`vagrant-build.sh` does **not** auto-destroy the VM. Run `vagrant destroy` when done.

### CI path

`.github/workflows/build.yml` runs the same build on self-hosted FreeBSD 15 / 16 VMs and a pfSense 2.8.x VM matrix. CI is the authoritative gate before merge.

### Local-only checks (no pfSense, no FreeBSD)

```bash
./node_modules/.bin/prettier --check ./pfSense-pkg-RESTAPI/files
phplint -vvv --no-cache
black --check .
pylint $(git ls-files '*.py')
./phpdoc       # PHPDoc must build cleanly
```

These are also enforced by `.github/workflows/quality.yml`.

## 3. Runtime expectations on the pfSense host

After installing the built `.pkg` on a pfSense test instance:

```bash
pfsense-restapi runtests                 # the full custom test suite
pfsense-restapi runtests <keyword>       # filter by keyword
pfsense-restapi buildschemas             # regenerate OpenAPI + GraphQL artifacts

# manage.php (called by pfsense-restapi) also exposes:
#   buildendpoints, buildforms, buildprivs, notifydispatcher <DispatcherName>, ...
```

Endpoint PHP files in the pfSense webroot are **generated** by `manage.php buildendpoints` from `RESTAPI\Endpoints\*` classes. Never commit hand-written endpoint files.

## 4. Test framework reality check

- The project does **not** use PHPUnit. The framework is `RESTAPI\Core\TestCase` (see `.github/skills/writing-tests.md`).
- Tests assume a live pfSense kernel: `pfctl`, `unbound`, `filter_configure()`, etc.
- The runner snapshots and restores `$config` between `test*` methods — but does not unwind external state. If you write a file or install a package, undo it.
- Tests run as `root` on the pfSense host. Treat anything destructive accordingly: never run `pfsense-restapi runtests` on a production firewall.

## 5. Dependency rules

- Runtime PHP deps live in `composer.json` (currently: `firebase/php-jwt`, `webonyx/graphql-php`). They get bundled into the package under `.resources/vendor/`.
- Dev tooling lives in `package.json` (Prettier + plugin-php, Spectral) and `requirements.txt` (Black, pylint, mkdocs).
- Do not add new runtime PHP dependencies without maintainer approval — pfSense has no PHP package manager and every additional library increases install footprint and audit surface.

## 6. CI matrix at a glance

`.github/workflows/`:

- `quality.yml` — Prettier, Black, phplint, pylint, phpdoc render. Runs on every push.
- `build.yml` — Builds the package on FreeBSD, installs on pfSense 2.8.0/2.8.1 VMs, fetches `/api/v2/schema/openapi`, runs Spectral lint, and finally runs `pfsense-restapi runtests`. Runs on PRs to `master`.
- `release.yml` — Tag-driven release artifact pipeline.
- `schedule_weekly.yml` — Periodic maintenance jobs.

If you change anything that touches Endpoints/Models/Fields/Dispatchers/Validators/Auth, expect the `check_tests` job in `build.yml` to be the slowest and most failure-prone gate. Make local pre-checks first.

## 7. Branches and releases

| Branch       | Use for                                                                                                                    |
| ------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `next_patch` | Small bug fixes, doc typos. Next patch release.                                                                            |
| `next_minor` | New features and minimal-impact breaking changes. Next minor release.                                                      |
| `next_major` | Large structural changes. Rarely accepted.                                                                                 |
| `master`     | Reflects the current stable release. Direct PRs only for security hotfixes, dependency bumps, or release-independent docs. |

## Checklist

- [ ] No pfSense Plus-only symbols, paths, or features referenced.
- [ ] Build verified with `vagrant-build.sh` (or relying on CI matrix) before merge.
- [ ] Tests added/updated under `RESTAPI/Tests/` and confirmed to pass via `pfsense-restapi runtests` on a real pfSense VM.
- [ ] Local pre-checks (Prettier, phplint, Black, pylint, phpdoc) all pass.
- [ ] No new runtime PHP dependency added without maintainer approval.
- [ ] Generated artifacts (webroot endpoints, OpenAPI/GraphQL files, privileges) are **not** committed by hand.
- [ ] PR targets the correct branch (`next_patch` / `next_minor` / `next_major`, not `master`).
