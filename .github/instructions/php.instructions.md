---
applyTo: "pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/**/*.inc"
---

# pfSense-pkg-RESTAPI — global PHP rules

These rules apply to **every** PHP `.inc` file under `pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/`. Path-specific instructions in sibling files extend these for `Models/`, `Endpoints/`, `Validators/`, `Dispatchers/`, and `Tests/`.

Authoritative reference: `AGENTS.md` and `.github/skills/`.

## File preamble (required, in this order)

```php
<?php

namespace RESTAPI\<Subnamespace>;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\<...>;
```

- File extension is **`.inc`** (not `.php`).
- File name matches the class name exactly (`SystemHostname.inc` → `class SystemHostname`).
- Namespace mirrors the directory: `RESTAPI\Models`, `RESTAPI\Endpoints`, `RESTAPI\Validators`, `RESTAPI\Dispatchers`, `RESTAPI\Tests`, etc.

## Conventions

- **PHP 8.2.** Use named arguments at call sites — the codebase consistently does this (`new StringField(required: true, default: '', ...)`).
- **Comments:** `#` for short inline notes (matches existing code). Use `/** ... */` PHPDoc on every public class and method — PHPDoc rendering is gated by CI.
- **`response_id` values:** `UPPER_SNAKE_CASE`, prefixed with the class/feature (e.g. `INVALID_FIREWALL_ALIAS_NAME`, `IP_ADDRESS_VALIDATOR_FAILED`).
- **Throw, don't return errors.** Use a `RESTAPI\Responses\*` class with a stable `response_id`. Never throw bare `\Exception`.
- **Shell execution:** use `RESTAPI\Core\Command`, never bare `exec()` / `shell_exec()` / `passthru()`.
- **Sensitive values:** mark Fields `sensitive: true`. Never log secrets.

## pfSense CE only

Never reference symbols, files, or behaviors that exist only in pfSense Plus. If you cannot find it in pfSense CE source, do not use it.

## Formatting

Code must pass:

```bash
./node_modules/.bin/prettier --check ./pfSense-pkg-RESTAPI/files
phplint -vvv --no-cache
./phpdoc
```

CI enforces all three.
