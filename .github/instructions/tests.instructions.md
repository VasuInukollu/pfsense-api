---
applyTo: "pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Tests/**/*.inc"
---

# Tests — `RESTAPI\Tests\*`

This project does **not** use PHPUnit. Tests use the custom `RESTAPI\Core\TestCase` framework and run on a real pfSense instance via `pfsense-restapi runtests`.

For full guidance and worked examples see:

- `.github/skills/writing-tests.md`
- `docs/CONTRIBUTING.md` (Testing section)

## Naming

| Class under test                              | Test file/class                                     |
| --------------------------------------------- | --------------------------------------------------- |
| `RESTAPI\Models\FirewallAlias`                | `APIModelsFirewallAliasTestCase.inc`                |
| `RESTAPI\Validators\IPAddressValidator`       | `APIValidatorsIPAddressValidatorTestCase.inc`       |
| `RESTAPI\Core\Endpoint`                       | `APICoreEndpointTestCase.inc`                       |
| `RESTAPI\Dispatchers\FirewallApplyDispatcher` | `APIDispatchersFirewallApplyDispatcherTestCase.inc` |

Pattern: `API<Subnamespace><ClassUnderTest>TestCase`. File name matches the class name. One class per file.

## Required shape

```php
<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\<ClassUnderTest>;

class API<Subns><ClassUnderTest>TestCase extends TestCase {
    public function setup(): void { /* optional, runs once before any test* */ }
    public function teardown(): void { /* optional, runs once after all test* */ }

    public function test_<scenario>(): void {
        $this->assert_is_true(true);
    }
}
```

Discovery and lifecycle (see `Core/TestCase.inc::run`):

- Every method beginning with `test` is auto-discovered.
- `$config` is **snapshotted and restored** between each `test*` method.
- External state (files, sysctls, packages) is **not** reverted — clean it up in the test or `teardown()`.
- Mark flaky/timing-sensitive tests with `#[TestCaseRetry(retries: 3, delay: 1)]`.

## Assertion helpers

`assert_equals`, `assert_not_equals`, `assert_throws`, `assert_does_not_throw`, `assert_throws_response`, `assert_is_true`, `assert_is_false`, `assert_str_contains`, `assert_str_does_not_contain`, `assert_is_greater_than`, `assert_is_greater_than_or_equal`, `assert_is_less_than`, `assert_is_less_than_or_equal`, `assert_is_empty`, `assert_is_not_empty`, `assert_type`.

## `assert_throws_response` is canonical

Use it for any `RESTAPI\Responses\*` exception so both the HTTP code and the stable `response_id` are pinned:

```php
$this->assert_throws_response(
  response_id: "INVALID_FIREWALL_ALIAS_NAME",
  code: 400,
  callable: function () {
    $alias = new FirewallAlias(data: ["name" => "!!bad", "type" => "host"]);
    $alias->validate();
  },
);
```

## Hard rules

- Test class extends `RESTAPI\Core\TestCase`, **never** PHPUnit.
- Method names start with `test` (otherwise they are not discovered).
- For `Responses/*` errors, prefer `assert_throws_response` over `assert_throws`.
- Use `#[TestCaseRetry]` on tests that wait on async pfSense state (filter reload, DNS resolution, etc.).
- Do not hard-code interface names; use `$this->env['PFREST_WAN_IF']`, `PFREST_LAN_IF`, `PFREST_OPT1_IF`.
- If the test depends on a pfSense package, list it in `public array $required_packages = ['pfSense-pkg-...'];` so the runner installs it.
- Never run `pfsense-restapi runtests` on a production firewall.

## Running

On the pfSense host:

```bash
pfsense-restapi runtests                       # full suite
pfsense-restapi runtests <keyword>             # filter, e.g. FirewallAlias
pfsense-restapi runtests APIValidatorsIPAddressValidatorTestCase
```

CI runs the full suite on each PR against a pfSense 2.8.x VM matrix (see `.github/workflows/build.yml::check_tests`).
