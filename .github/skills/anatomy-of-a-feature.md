# Skill: Anatomy of a Feature

A worked, end-to-end example of adding a new pfSense resource exposed over the REST API. Read this once, then reuse the structure for every new resource.

We will pretend we are adding a `SystemMOTD` (message-of-the-day) resource. The same structure applies to firewall rules, DNS overrides, VPN settings, anything.

```
/api/v2/system/motd          → singular endpoint (GET, PATCH)
```

Settings-style singletons need only one Endpoint, one Model, one Dispatcher (if applying anything at runtime), and one TestCase. `many` resources additionally get a collection endpoint and may use Validators for each Field.

## Step 0: Confirm the feature exists in pfSense CE

Search https://github.com/pfsense/pfsense for the underlying functionality. If you cannot find a CE-friendly hook (function, config path, service script), stop. Do not guess. (See `platform-testing-legal.md`.)

For our example: pfSense CE stores the MOTD in the `system/motd` config path and applies it via the `system_motd` config knob — both safe, public CE behavior.

## Step 1: Pick names

| Concern          | Name                                                                    |
| ---------------- | ----------------------------------------------------------------------- |
| Model class      | `SystemMOTD`                                                            |
| Model file       | `pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Models/SystemMOTD.inc` |
| Endpoint class   | `SystemMOTDEndpoint`                                                    |
| Endpoint file    | `Endpoints/SystemMOTDEndpoint.inc`                                      |
| Endpoint URL     | `/api/v2/system/motd`                                                   |
| Dispatcher class | `SystemMOTDApplyDispatcher` (only if `apply()` does work)               |
| TestCase         | `APIModelsSystemMOTDTestCase`                                           |
| Response IDs     | `INVALID_SYSTEM_MOTD_*`, `SYSTEM_MOTD_APPLY_FAILED`, etc.               |

## Step 2: Write the Model

```php
<?php

namespace RESTAPI\Models;

require_once "RESTAPI/autoloader.inc";

use RESTAPI\Core\Model;
use RESTAPI\Dispatchers\SystemMOTDApplyDispatcher;
use RESTAPI\Fields\StringField;
use RESTAPI\Responses\ValidationError;
use RESTAPI\Validators\LengthValidator;

/**
 * Defines a Model that represents the system message-of-the-day banner.
 */
class SystemMOTD extends Model
{
  public StringField $motd;

  public function __construct(
    mixed $id = null,
    mixed $parent_id = null,
    mixed $data = [],
    mixed ...$options,
  ) {
    # Model attributes
    $this->config_path = "system";
    $this->update_strategy = "merge"; # do not overwrite siblings under system/
    $this->always_apply = true; # apply on every successful update
    $this->verbose_name = "System MOTD";

    # Fields
    $this->motd = new StringField(
      required: false,
      default: "",
      allow_empty: true,
      internal_name: "motd",
      validators: [new LengthValidator(maximum: 4096)],
      verbose_name: "MOTD",
      help_text: "The message-of-the-day banner shown on console and SSH login.",
    );

    parent::__construct($id, $parent_id, $data, ...$options);
  }

  /**
   * Reject characters that pfSense's shell login banner cannot render safely.
   */
  public function validate_motd(string $motd): string
  {
    if (preg_match('/[\x00-\x08\x0B-\x1F]/', $motd)) {
      throw new ValidationError(
        message: "Field `motd` contains unsupported control characters.",
        response_id: "INVALID_SYSTEM_MOTD_CONTROL_CHARS",
      );
    }
    return $motd;
  }

  /**
   * Persist the MOTD to disk and re-emit the login banner.
   */
  public function apply(): void
  {
    new SystemMOTDApplyDispatcher(async: $this->async)->spawn_process();
  }
}
```

Notes:

- Set Model attributes **before** Fields. `parent::__construct(...)` is the very last statement.
- Reuse `LengthValidator` rather than re-implementing length checks.
- `validate_motd()` returns the (possibly transformed) value — that's the contract.
- `apply()` delegates to a Dispatcher, never blocks the request.

## Step 3: Write the Endpoint

```php
<?php

namespace RESTAPI\Endpoints;

require_once "RESTAPI/autoloader.inc";

use RESTAPI\Core\Endpoint;

/**
 * Defines an Endpoint for interacting with the SystemMOTD Model object at /api/v2/system/motd.
 */
class SystemMOTDEndpoint extends Endpoint
{
  public function __construct()
  {
    $this->url = "/api/v2/system/motd";
    $this->model_name = "SystemMOTD";
    $this->request_method_options = ["GET", "PATCH"];
    $this->get_help_text = "Reads the current MOTD banner.";
    $this->patch_help_text = "Updates the MOTD banner.";
    parent::__construct();
  }
}
```

That is the entire Endpoint. No method overrides. No business logic. No hand-written privileges (they are derived from the URL).

## Step 4: Write the Dispatcher

Only needed because `apply()` actually does something. If your Model only mutates `$config` and pfSense reads it lazily, you can skip the Dispatcher and not implement `apply()`.

```php
<?php

namespace RESTAPI\Dispatchers;

use RESTAPI\Core\Dispatcher;

/**
 * Defines a Dispatcher that re-renders the MOTD login banner.
 */
class SystemMOTDApplyDispatcher extends Dispatcher
{
  public int $timeout = 30;

  protected function _process(mixed ...$arguments): void
  {
    # pfSense exposes a function that reads system/motd and rewrites /etc/motd
    system_motd();
  }
}
```

If pfSense did not expose a helper function, you would shell out via `RESTAPI\Core\Command`, never bare `exec()`.

## Step 5: Write the TestCase

```php
<?php

namespace RESTAPI\Tests;

use RESTAPI\Core\TestCase;
use RESTAPI\Models\SystemMOTD;

class APIModelsSystemMOTDTestCase extends TestCase
{
  public function test_default_motd_is_empty(): void
  {
    $motd = new SystemMOTD();
    $this->assert_equals("", $motd->motd->value);
  }

  public function test_reject_control_chars(): void
  {
    $this->assert_throws_response(
      response_id: "INVALID_SYSTEM_MOTD_CONTROL_CHARS",
      code: 400,
      callable: function () {
        $motd = new SystemMOTD(data: ["motd" => "Hello\x07world"]);
        $motd->validate();
      },
    );
  }

  public function test_set_and_read_motd(): void
  {
    $motd = new SystemMOTD(data: ["motd" => "Welcome to pfSense"]);
    $motd->update(apply: true);

    $reread = new SystemMOTD();
    $this->assert_equals("Welcome to pfSense", $reread->motd->value);
  }

  public function test_length_limit(): void
  {
    $this->assert_throws_response(
      response_id: "LENGTH_VALIDATOR_MAXIMUM_EXCEEDED",
      code: 400,
      callable: function () {
        $motd = new SystemMOTD(data: ["motd" => str_repeat("x", 4097)]);
        $motd->validate();
      },
    );
  }
}
```

Each `test*` method gets a clean `$config` snapshot. No teardown is needed because the tests touch only `$config`.

## Step 6: Format, lint, build, run

```bash
# Local pre-checks
./node_modules/.bin/prettier --write ./pfSense-pkg-RESTAPI/files
phplint -vvv --no-cache
./phpdoc

# Build the FreeBSD package via Vagrant (FreeBSD 14 host)
sh vagrant-build.sh

# Copy to a pfSense test VM and install
scp pfSense-pkg-RESTAPI-*.pkg admin@<pfsense-vm>:/tmp/
ssh admin@<pfsense-vm> 'pkg -C /dev/null add /tmp/pfSense-pkg-RESTAPI-*.pkg'

# Run the new tests on the pfSense VM
ssh admin@<pfsense-vm> 'pfsense-restapi runtests SystemMOTD'
```

The `pkg add` step automatically:

- Generates `/usr/local/www/api/v2/system/motd.php` from your Endpoint class.
- Adds `api-v2-system-motd-get` / `-patch` privileges.
- Regenerates the OpenAPI and GraphQL schema files.

You did not write any of those artifacts by hand. That is the framework working as intended.

## Optional extras

- **`many` resource**: add `$this->many = true;` to the Model and create a sibling Endpoint with `$this->many = true; $this->request_method_options = ['GET', 'PUT', 'DELETE'];`. The class name is the plural form (e.g. `FirewallAliasesEndpoint`).
- **Children of another Model**: set `$this->parent_model_class = 'ParentModel';` and use `$this->config_path` relative to the parent (see `DNSResolverHostOverrideAlias`).
- **Sensitive fields**: set `sensitive: true` and never log them.
- **Dynamic choices**: set `choices_callable: 'method_name'` returning an array.
- **Conditional fields**: set `conditions: ['other_field' => true]`.
- **Form page in webConfigurator**: add a sibling `Forms/SystemMOTDForm.inc` extending `RESTAPI\Core\Form`.
- **Cache-backed read-only Models** (e.g. status, package list): set `$this->cache_class = 'MyCache';` instead of `$this->internal_callable = '...';`.

## Anti-patterns to avoid

- Pulling logic into the Endpoint instead of the Model.
- Inlining validation that already exists in `Validators/`.
- Calling `filter_configure_sync()` directly from the Model — always go through a Dispatcher.
- Throwing `\Exception` instead of a `Responses/*` class with a `response_id`.
- Using `gettype($x)` checks instead of typed Field properties.
- Creating ad-hoc privilege names. They are derived from `$url` + method.

## Final checklist

- [ ] Model defined first, Fields declared before `parent::__construct(...)`.
- [ ] Endpoint stays under ~25 lines, no method overrides.
- [ ] Reused or added a `Validators/` class for each repeatable rule.
- [ ] `apply()` delegates to a Dispatcher when service-level work is needed.
- [ ] TestCase named `API<Subnamespace><ClassUnderTest>TestCase`, lives in `Tests/`, extends `RESTAPI\Core\TestCase`.
- [ ] Built and ran `pfsense-restapi runtests <keyword>` on a real pfSense VM.
- [ ] No generated artifacts committed by hand.
- [ ] Targeted the correct branch (`next_patch` / `next_minor`).
