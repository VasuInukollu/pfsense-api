# Skill: Endpoint + Model + Field

Use this skill whenever you create or modify an API resource. The repository is **declarative and metadata-driven** — Endpoints are routers, Models hold the behavior, and Fields hold the schema. OpenAPI, GraphQL, privileges, and HATEOAS links are all generated from these classes, so any deviation breaks more than just the touched file.

## TL;DR

1. **Endpoint**: ~10–25 lines. Sets `$url`, `$model_name`, `$request_method_options`, optional `$many`, optional help text. Calls `parent::__construct()` last.
2. **Model**: defines Fields in the constructor, attaches reusable Validators, optionally implements `validate_<field>()`/`validate_extra()`/`apply()`. `parent::__construct(...)` is the last statement.
3. **Field**: declarative — set `required`, `default`, `choices`, `many`, `unique`, `sensitive`, `validators`, `verbose_name`, `help_text` etc. Do not validate manually if a Validator exists.

## Endpoint patterns

### Singular resource (one canonical object)

```php
<?php

namespace RESTAPI\Endpoints;

require_once "RESTAPI/autoloader.inc";

use RESTAPI\Core\Endpoint;

class SystemHostnameEndpoint extends Endpoint
{
  public function __construct()
  {
    $this->url = "/api/v2/system/hostname";
    $this->model_name = "SystemHostname";
    $this->request_method_options = ["GET", "PATCH"];
    $this->get_help_text = "Reads the current system hostname.";
    $this->patch_help_text = "Updates the system hostname.";
    parent::__construct();
  }
}
```

### One item of a `many` Model

```php
class FirewallAliasEndpoint extends Endpoint
{
  public function __construct()
  {
    $this->url = "/api/v2/firewall/alias";
    $this->model_name = "FirewallAlias";
    $this->request_method_options = ["GET", "POST", "PATCH", "DELETE"];
    parent::__construct();
  }
}
```

### Collection of a `many` Model

```php
class FirewallAliasesEndpoint extends Endpoint
{
  public function __construct()
  {
    $this->url = "/api/v2/firewall/aliases";
    $this->model_name = "FirewallAlias";
    $this->many = true;
    $this->request_method_options = ["GET", "PUT", "DELETE"];
    parent::__construct();
  }
}
```

### Apply endpoint (status + trigger)

```php
class FirewallApplyEndpoint extends Endpoint
{
  public function __construct()
  {
    $this->url = "/api/v2/firewall/apply";
    $this->model_name = "FirewallApply";
    $this->request_method_options = ["GET", "POST"];
    $this->get_help_text = "Read pending firewall change status.";
    $this->post_help_text = "Apply pending firewall changes.";
    parent::__construct();
  }
}
```

### Method → Model mapping (enforced by `Core/Endpoint.inc::check_construct`)

| `many`  | Method   | Model method called |
| ------- | -------- | ------------------- |
| `false` | `GET`    | `read()`            |
| `false` | `POST`   | `create()`          |
| `false` | `PATCH`  | `update()`          |
| `false` | `DELETE` | `delete()`          |
| `true`  | `GET`    | `read_all()`        |
| `true`  | `PUT`    | `replace_all()`     |
| `true`  | `DELETE` | `delete_many()`     |

> **Do not override** `get()`, `post()`, `patch()`, `put()`, or `delete()`. There are zero such overrides in `Endpoints/` today — keep it that way.

### Optional Endpoint properties (use only when needed)

`requires_auth`, `auth_methods`, `ignore_read_only`, `ignore_interfaces`, `ignore_enabled`, `ignore_acl`, `tag` (overrides default OpenAPI tag derived from URL), `*_help_text`, `deprecated`, `limit`, `offset`, `sort_by`, `sort_order`, `sort_flags`, `encode_content_handlers`, `decode_content_handlers`, `resource_link_set`. See `Core/Endpoint.inc` PHPDoc for full semantics.

## Model patterns

### Config-backed `many` Model

```php
<?php

namespace RESTAPI\Models;

require_once "RESTAPI/autoloader.inc";

use RESTAPI\Core\Model;
use RESTAPI\Dispatchers\DNSResolverApplyDispatcher;
use RESTAPI\Fields\StringField;
use RESTAPI\Validators\HostnameValidator;

class DNSResolverHostOverrideAlias extends Model
{
  public StringField $host;
  public StringField $domain;
  public StringField $descr;

  public function __construct(
    mixed $id = null,
    mixed $parent_id = null,
    mixed $data = [],
    mixed ...$options,
  ) {
    $this->parent_model_class = "DNSResolverHostOverride";
    $this->config_path = "aliases/item"; # relative to the parent Model's config_path
    $this->subsystem = "unbound";
    $this->many = true;
    $this->sort_order = SORT_ASC;
    $this->sort_by = ["host"];
    $this->unique_together_fields = ["host", "domain"];

    $this->host = new StringField(
      required: true,
      allow_empty: true,
      maximum_length: 255,
      validators: [new HostnameValidator()],
      verbose_name: "Host",
      help_text: "The hostname portion of the host override alias.",
    );
    $this->domain = new StringField(
      required: true,
      maximum_length: 255,
      validators: [new HostnameValidator()],
      verbose_name: "Domain",
      help_text: "The domain portion of the host override alias.",
    );
    $this->descr = new StringField(
      default: "",
      allow_empty: true,
      internal_name: "description", # stored in pfSense XML as `description`
      verbose_name: "Description",
      help_text: "A detailed description for this host override alias.",
    );

    parent::__construct($id, $parent_id, $data, ...$options);
  }

  public function apply(): void
  {
    new DNSResolverApplyDispatcher(async: $this->async)->spawn_process();
  }
}
```

### Singleton settings Model

```php
class SystemHostname extends Model
{
  public StringField $hostname;
  public StringField $domain;

  public function __construct(
    mixed $id = null,
    mixed $parent_id = null,
    mixed $data = [],
    mixed ...$options,
  ) {
    $this->config_path = "system";
    $this->update_strategy = "merge"; # do not overwrite siblings under system/
    $this->always_apply = true; # call apply() on every successful update
    $this->hostname = new StringField(
      required: true,
      allow_empty: true,
      validators: [
        new HostnameValidator(allow_hostname: true, allow_domain: false),
      ],
      verbose_name: "Hostname",
      help_text: "The hostname portion of the FQDN to assign to this system.",
    );
    $this->domain = new StringField(
      required: true,
      allow_empty: true,
      validators: [
        new HostnameValidator(allow_hostname: false, allow_domain: true),
      ],
      verbose_name: "Domain",
      help_text: "The domain portion of the FQDN to assign to this system.",
    );

    parent::__construct($id, $parent_id, $data, ...$options);
  }

  public function apply(): void
  {
    new SystemHostnameApplyDispatcher(async: $this->async)->spawn_process();
  }
}
```

### Non-config-backed Model (read from runtime, write via `_create`)

Use `internal_callable` to supply data that does not live in `$config`. Override `_create()`/`_update()`/`_delete()` only when the data isn't backed by `$config`.

```php
class CommandPrompt extends Model {
    public StringField $command;
    public StringField $output;
    public IntegerField $result_code;

    public function __construct(mixed $id = null, mixed $parent_id = null, mixed $data = [], mixed ...$options) {
        $this->command = new StringField(required: true, write_only: true, ...);
        $this->output = new StringField(allow_null: true, read_only: true, ...);
        $this->result_code = new IntegerField(allow_null: true, read_only: true, ...);
        parent::__construct($id, $parent_id, $data, ...$options);
    }

    public function _create(): void {
        $cmd = new Command($this->command->value);
        $this->output->value = $cmd->output;
        $this->result_code->value = $cmd->result_code;
    }
}
```

### Model-specific validation

Reach for these only after confirming no `Validators/` class fits.

```php
public function validate_name(string $name): string {
    if (!is_validaliasname($name)) {
        throw new ValidationError(
            message: "Invalid firewall alias name '$name'.",
            response_id: 'INVALID_FIREWALL_ALIAS_NAME',
        );
    }
    return $name;   # always return the (possibly transformed) value
}

public function validate_extra(): void {
    # Cross-field validation goes here. Throw ValidationError with a stable response_id.
}
```

## Field patterns

| You want...                              | Use this                                                                                       |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------- |
| Plain string with length cap             | `new StringField(required: true, maximum_length: 31, ...)`                                     |
| Bounded integer                          | `new IntegerField(default: 3600, minimum: 300, maximum: 86400, ...)`                           |
| Boolean stored as `enabled`/`disabled`   | `new BooleanField(default: true, indicates_true: 'enabled', indicates_false: 'disabled', ...)` |
| Choice list with verbose labels          | `new StringField(choices: ['LOG_INFO' => 'Info', 'LOG_WARNING' => 'Warning'], ...)`            |
| Choice list populated dynamically        | `new StringField(choices_callable: 'method_on_this_model', ...)`                               |
| Many values stored space-separated       | `new StringField(many: true, delimiter: ' ', allow_empty: true, default: [], ...)`             |
| Reference to another Model's field       | `new ForeignModelField(model_name: 'RoutingGateway', model_field: 'name', ...)`                |
| Nested Model objects (children)          | `new NestedModelField(model_class: 'TrafficShaperLimiterQueue', many: true, ...)`              |
| Internal name differs from API name      | `new StringField(..., internal_name: 'description')`                                           |
| Field nested under an internal namespace | `new StringField(..., internal_namespace: 'advanced')`                                         |
| Conditionally relevant field             | `new StringField(..., conditions: ['enabled' => true])`                                        |
| Sensitive value (passwords, keys)        | `new StringField(..., sensitive: true)`                                                        |
| Read-only computed value                 | `new StringField(..., read_only: true)`                                                        |
| Write-only input not echoed back         | `new StringField(..., write_only: true)`                                                       |
| Model-only field never written to config | `new StringField(..., representation_only: true)`                                              |

> Always set `verbose_name` and `help_text` — both feed the OpenAPI/GraphQL/Form output.

## Common mistakes (caught in review)

- Adding business logic inside an Endpoint subclass.
- Defining Fields after `parent::__construct(...)` instead of before.
- Throwing a generic `Exception` instead of a `Responses/*` class with a `response_id`.
- Forgetting `apply()` on a Model that mutates a service that requires reload.
- Using `exec()` / `shell_exec()` / `passthru()` instead of `RESTAPI\Core\Command`.
- Hard-coding privilege names. They auto-generate from `$url` + method.
- Editing the generated PHP files in the pfSense webroot. They are produced by `manage.php buildendpoints` at install time.

## Checklist

- [ ] Endpoint contains only attribute assignments and `parent::__construct();`.
- [ ] `$request_method_options` matches `$many` (per the table above).
- [ ] Every Field has `verbose_name` and `help_text`.
- [ ] Reusable rules use a `Validators/` class via `validators: [...]`.
- [ ] Sensitive Fields use `sensitive: true`.
- [ ] If the Model affects a service, `apply()` invokes the corresponding Dispatcher with `async: $this->async`.
- [ ] Errors throw a `Responses/*` class with a stable `UPPER_SNAKE_CASE` `response_id`.
- [ ] Tests added/updated under `Tests/`.
