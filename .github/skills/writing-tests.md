# Skill: Writing Tests

The project ships its own lightweight test framework in `RESTAPI\Core\TestCase` because pfSense does not include PHPUnit. Tests must run on a **real pfSense instance** via `pfsense-restapi runtests`. Local PHP execution is only suitable for syntax checks (`phplint`).

## File and class layout

- Location: `pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Tests/`
- File extension: `.inc`
- File name = class name + `.inc`
- Naming convention: `API<Subnamespace><ClassUnderTest>TestCase`

| Class under test                              | Test file                                           |
| --------------------------------------------- | --------------------------------------------------- |
| `RESTAPI\Models\FirewallAlias`                | `APIModelsFirewallAliasTestCase.inc`                |
| `RESTAPI\Validators\IPAddressValidator`       | `APIValidatorsIPAddressValidatorTestCase.inc`       |
| `RESTAPI\Core\Endpoint`                       | `APICoreEndpointTestCase.inc`                       |
| `RESTAPI\Dispatchers\FirewallApplyDispatcher` | `APIDispatchersFirewallApplyDispatcherTestCase.inc` |

## Skeleton

```php
<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\FirewallAlias;

class APIModelsFirewallAliasTestCase extends TestCase
{
  /**
   * Optional — runs once before any test* method.
   */
  public function setup(): void
  {
    # e.g. disable login protection so failing auth attempts don't flood logs
  }

  /**
   * Optional — runs once after all test* methods.
   */
  public function teardown(): void
  {
    # Undo anything created outside the pfSense $config (files on disk, etc.)
  }

  public function test_does_the_thing(): void
  {
    $this->assert_is_true(true);
  }
}
```

Discovery rules (see `Core/TestCase.inc::run`):

- Every method beginning with `test` is auto-discovered.
- Between each `test*` method, the framework snapshots and restores `$config`, so config-only changes do not bleed across tests.
- External state (files, sysctls, packages) is **not** restored — clean it up yourself.
- Methods may declare `#[TestCaseRetry(retries: 3, delay: 1)]` for flaky/timing-sensitive cases (e.g. waiting for `pfctl` tables to populate after a filter reload).

## Assertion helpers

All defined in `Core/TestCase.inc`:

- `assert_equals($a, $b, $msg = '')` — strict `!==`.
- `assert_not_equals($a, $b, $msg = '')`
- `assert_throws(array $exception_classes, callable $fn, $msg = '')`
- `assert_does_not_throw(callable $fn)`
- `assert_throws_response(string $response_id, int $code, callable $fn)` — **the canonical way to check Response exceptions**.
- `assert_is_true($v, $msg = '...')`, `assert_is_false($v, $msg = '...')`
- `assert_str_contains($haystack, $needle)`, `assert_str_does_not_contain(...)`
- `assert_is_greater_than($a, $b)`, `assert_is_greater_than_or_equal(...)`
- `assert_is_less_than($a, $b)`, `assert_is_less_than_or_equal(...)`
- `assert_is_empty($v, $msg = 'Expected value to be empty.')`, `assert_is_not_empty($v, ...)`
- `assert_type($v, string $type, string $msg = '')` — `gettype()`-style.

## Patterns

### Verifying a `ValidationError`

```php
public function test_reject_prohibited_alias_names(): void {
    $this->assert_throws_response(
        response_id: 'FILTER_NAME_VALIDATOR_CANNOT_START_WITH_PKG',
        code: 400,
        callable: function () {
            $alias = new FirewallAlias(data: ['name' => 'pkg_anything', 'type' => 'host']);
            $alias->validate();
        },
    );
}
```

### Asserting `apply()` actually changed pfSense

```php
#[TestCaseRetry(retries: 3, delay: 1)]
public function test_fqdn_alias_populates_pfctl_table(): void {
    $alias = new FirewallAlias(data: [
        'name' => 'TEST_GOOGLE_DNS',
        'type' => 'host',
        'address' => ['dns.google'],
    ]);
    $alias->create(apply: true);

    # Wait up to 30s for the filter reload + DNS resolution
    foreach (range(0, 30) as $attempt) {
        $pfctl = shell_exec('pfctl -t TEST_GOOGLE_DNS -Ts');
        if ($pfctl) {
            break;
        }
        sleep(1);
    }

    $this->assert_str_contains($pfctl, '8.8.8.8');
    $this->assert_str_contains($pfctl, '8.8.4.4');

    $alias->delete(apply: true);
}
```

> Real pfctl reads are great regression tests but slow. Mark them with `TestCaseRetry` and use `shell_exec` / `Command` only for read-only diagnostics.

### Setup/teardown for shared fixtures

```php
public function setup(): void {
    $api_settings = new RESTAPISettings(login_protection: false);
    $api_settings->update();
}

public function teardown(): void {
    $api_settings = new RESTAPISettings(login_protection: true);
    $api_settings->update();
}
```

### Mocking the request method when testing Endpoints

`Endpoint::__construct` reads `$_SERVER['REQUEST_METHOD']`. Set it before instantiating:

```php
public function test_construct(): void {
    $_SERVER['REQUEST_METHOD'] = 'POST';

    $endpoint = new FirewallAliasEndpoint();
    $this->assert_equals('POST', $endpoint->request_method);
    $this->assert_is_true($endpoint->model instanceof FirewallAlias);
    $this->assert_is_not_empty($endpoint->get_privileges);
}
```

### Validating a Validator

```php
public function test_throw_response_for_non_ip_value(): void {
    $this->assert_throws_response(
        response_id: 'IP_ADDRESS_VALIDATOR_FAILED',
        code: 400,
        callable: function () {
            (new IPAddressValidator())->validate('not an IP!');
        },
    );
}
```

### Tests that depend on a pfSense package

Set `$required_packages` on the TestCase. The runner installs them with `pkg install -y` before invoking `setup()`:

```php
class APIModelsHAProxyBackendTestCase extends TestCase
{
  public array $required_packages = ["pfSense-pkg-haproxy"];
}
```

### Tests that depend on environment-specific interfaces

`TestCase::get_envs()` populates `$this->env['PFREST_WAN_IF']`, `$this->env['PFREST_LAN_IF']`, and `$this->env['PFREST_OPT1_IF']`. Use them instead of hard-coding `em0`, `em1`, etc.

## Running tests

On the pfSense host:

```bash
pfsense-restapi runtests                                      # full suite
pfsense-restapi runtests FirewallAlias                        # filter by keyword
pfsense-restapi runtests APIValidatorsIPAddressValidatorTestCase
```

CI runs the suite on every PR against pfSense 2.8.x VMs (see `.github/workflows/build.yml`).

## Common mistakes

- Forgetting to revert non-config state (files, sysctls, package installs) in `teardown()`.
- Hard-coding an interface name instead of `$this->env['PFREST_WAN_IF']`.
- Asserting on transient `pfctl` output without `TestCaseRetry`.
- Using `assert_throws` when `assert_throws_response` would also pin the `response_id`.
- Adding tests that try to mutate `$config` directly — let Models do it so the snapshot/restore lifecycle works.
- Naming a method `check_*` or `it_should_*` instead of `test_*` (it won't be discovered).

## Checklist

- [ ] File and class follow `API<Subnamespace><ClassUnderTest>TestCase` naming.
- [ ] Class extends `RESTAPI\Core\TestCase`.
- [ ] Each test method starts with `test`.
- [ ] `assert_throws_response` is used to verify `Responses/*` exceptions, with both `response_id` and `code`.
- [ ] Flaky/timing-sensitive tests are marked with `#[TestCaseRetry(...)]`.
- [ ] External state created during a test is removed in the test or `teardown()`.
- [ ] No assumption that PHPUnit is available — only `RESTAPI\Core\TestCase` helpers are used.
