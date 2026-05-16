# Skill: Validators, Command, and Dispatchers

Use this skill when your change needs to validate input, run shell commands, or perform work that takes longer than a typical HTTP request.

## 1. Validators

Reusable validation lives in `RESTAPI/Validators/`. Each Validator extends `RESTAPI\Core\Validator` and implements one method:

```php
public function validate(mixed $value, string $field_name = ''): void;
```

It throws `RESTAPI\Responses\ValidationError` with a stable `response_id` on failure and returns silently on success.

### Reuse before adding

Existing Validators in `Validators/`:

`EmailAddressValidator`, `FilterNameValidator`, `HexValidator`, `HostnameValidator`, `IPAddressValidator`, `LengthValidator`, `MACAddressValidator`, `NumericRangeValidator`, `RegexValidator`, `SubnetValidator`, `URLValidator`, `UniqueFromForeignModelValidator`, `X509Validator`.

### Attaching a Validator to a Field

```php
$this->ha_sync_hosts = new StringField(
  default: [],
  allow_empty: true,
  many: true,
  delimiter: " ",
  validators: [new IPAddressValidator(allow_fqdn: true)],
  verbose_name: "HA sync hosts",
  help_text: "Set a list of IP addresses or hostnames to sync API settings to.",
);
```

### Authoring a new Validator

```php
<?php

namespace RESTAPI\Validators;

require_once "RESTAPI/autoloader.inc";

use RESTAPI\Core\Validator;
use RESTAPI\Responses\ValidationError;

/**
 * Defines a Validator that ensures a value matches a port number or service name.
 */
class PortOrServiceValidator extends Validator
{
  public function __construct(public bool $allow_service_names = true) {}

  public function validate(mixed $value, string $field_name = ""): void
  {
    if (is_port($value)) {
      return;
    }
    if ($this->allow_service_names and getservbyname($value, "tcp") !== false) {
      return;
    }

    throw new ValidationError(
      message: "Field `$field_name` must be a valid port or service name, received `$value`.",
      response_id: "PORT_OR_SERVICE_VALIDATOR_FAILED",
    );
  }
}
```

Notes:

- Constructor parameters are public-promoted properties — that's the project convention.
- `response_id` is `UPPER_SNAKE_CASE` and prefixed by the Validator name so values stay unique.
- Validators may emit informative `set_label('is_xxx')` calls on the Field to skip redundant downstream checks (see `IPAddressValidator`).

### Model-specific (non-reusable) validation

Use `validate_<field_name>($value): <type>` returning the validated value, or `validate_extra(): void` for cross-field rules. Both throw `ValidationError`.

```php
public function validate_address(string $address): string {
    $type = $this->type->value;
    if ($type === 'host' and !is_ipaddr($address) and !is_fqdn($address)) {
        throw new ValidationError(
            message: "Host alias 'address' value '$address' is not a valid IP, FQDN, or alias.",
            response_id: 'INVALID_HOST_ALIAS_ADDRESS',
        );
    }
    return $address;
}
```

## 2. `RESTAPI\Core\Command`

Use this for **all** shell execution in feature code. It captures `output` and `result_code` and routes stderr to stdout by default.

```php
use RESTAPI\Core\Command;

$cmd = new Command("/sbin/pfctl -sr");
if ($cmd->result_code !== 0) {
  throw new ServerError(
    message: "pfctl failed: $cmd->output",
    response_id: "PFCTL_RULESET_READ_FAILED",
  );
}

# Strip excessive whitespace
$cmd = new Command("/sbin/ifconfig em0", trim_whitespace: true);

# Custom redirect (e.g. to background a process)
$cmd = new Command(
  "nohup /usr/local/bin/something",
  redirect: '>/dev/null & echo $!',
);
```

Avoid bare `exec()`, `shell_exec()`, and `passthru()` in new code. The only intentional uses today are inside `Core/Command.inc` and the test runner.

## 3. Dispatchers

Anything that can take more than ~1 second — service reloads, filter rebuilds, certificate issuance, HA sync, etc. — must run through a Dispatcher so the API stays responsive.

### Anatomy of a Dispatcher

```php
<?php

namespace RESTAPI\Dispatchers;

use RESTAPI\Core\Dispatcher;

/**
 * Defines a Dispatcher for applying changes to the firewall filter configuration.
 */
class FirewallApplyDispatcher extends Dispatcher
{
  protected function _process(mixed ...$arguments): void
  {
    if ($this->async) {
      filter_configure();
    } else {
      filter_configure_sync();
    }
  }
}
```

Optional class properties:

| Property             | Default | Purpose                                                                                                    |
| -------------------- | ------- | ---------------------------------------------------------------------------------------------------------- |
| `$timeout`           | `300`   | Max seconds before the spawned process is killed.                                                          |
| `$max_queue`         | `10`    | Max queued processes; exceeding throws `ServiceUnavailableError` (`DISPATCHER_TOO_MANY_QUEUED_PROCESSES`). |
| `$schedule`          | `''`    | 5-field cron string. Setting this registers a `CronJob` automatically.                                     |
| `$required_packages` | `[]`    | pfSense package full names (`pfSense-pkg-...`). Missing packages throw `FailedDependencyError`.            |
| `$package_includes`  | `[]`    | PHP includes pulled in before `_process()` runs.                                                           |

### Triggering from a Model

```php
public function apply(): void {
    (new FirewallApplyDispatcher(async: $this->async))->spawn_process();
}
```

- `async: $this->async` defers to whatever the client requested (`async` is a control parameter).
- `spawn_process()` returns the spawned PID and writes a PID file under `/tmp/`.
- For Models that should always trigger `apply()` after a write (settings/singletons), set `$this->always_apply = true;` in the constructor.

### Manual invocation (debugging)

On the pfSense host:

```bash
pfsense-restapi notifydispatcher FirewallApplyDispatcher
```

`manage.php notifydispatcher` runs the dispatcher synchronously, waits for any earlier instance to finish, and writes a `/tmp/<DispatcherName>.lock` file while running.

### Scheduled Dispatchers

Setting `$schedule = '*/5 * * * *';` causes the package install routine (`schedule_dispatchers()` in `manage.php`) to create a corresponding `CronJob`. Cache classes (which extend `Dispatcher`) follow the same pattern.

## 4. End-to-end example

```php
# Models/SystemHostname.inc — set the hostname, then trigger a Dispatcher
public function apply(): void {
    (new SystemHostnameApplyDispatcher(async: $this->async))->spawn_process();
}
```

```php
# Dispatchers/SystemHostnameApplyDispatcher.inc — restart the relevant pfSense services
class SystemHostnameApplyDispatcher extends Dispatcher
{
  protected function _process(mixed ...$arguments): void
  {
    system_hostname_configure();
    system_resolvconf_generate();
    services_dnsmasq_configure();
    services_unbound_configure();
    filter_configure();
  }
}
```

If any of those operations failed before, your only outward sign was a request that hung for 30+ seconds. With the Dispatcher pattern the request returns immediately, and clients can poll a corresponding `*Apply` Model for status (see `FirewallApply`).

## Checklist

- [ ] Reused an existing Validator if one fit.
- [ ] New Validators throw `ValidationError` with a stable, namespaced `response_id`.
- [ ] All shell execution uses `RESTAPI\Core\Command`.
- [ ] Long-running work lives in `Dispatchers/` and is triggered from `Model::apply()` via `spawn_process()`.
- [ ] `apply()` is invoked because the Model was changed via `create()`/`update()`/`delete()` (or `always_apply: true` is set for singletons).
- [ ] No bare `exec()`/`shell_exec()`/`passthru()` introduced.
