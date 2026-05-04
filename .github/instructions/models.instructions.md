---
applyTo: "pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Models/**/*.inc"
---

# Models — `RESTAPI\Models\*`

Models hold the actual feature behavior of the package. Endpoints are thin adapters; everything that _does_ something belongs here.

For full guidance and worked examples see:

- `.github/skills/endpoint-model-field.md`
- `.github/skills/anatomy-of-a-feature.md`
- `docs/BUILDING_CUSTOM_MODEL_CLASSES.md`

## Constructor signature is fixed

```php
public function __construct(
    mixed $id = null,
    mixed $parent_id = null,
    mixed $data = [],
    mixed ...$options,
) {
    # 1. Set Model attributes (config_path, many, subsystem, ...)
    # 2. Define Field objects on $this
    # 3. parent::__construct(...) MUST be the last statement
    parent::__construct($id, $parent_id, $data, ...$options);
}
```

Order matters. Field objects must exist before the parent constructor runs.

## Naming

- Class: PascalCase, singular noun. `FirewallAlias`, `DNSResolverHostOverrideAlias`, `SystemHostname`.
- File: `<ClassName>.inc`.
- One class per file.

## Schema lives on Fields

Define every property as a typed `Field` (`StringField`, `IntegerField`, `BooleanField`, `ForeignModelField`, `NestedModelField`, etc.). Use Field constructor arguments for `required`, `default`, `choices`, `unique`, `sensitive`, `read_only`, `write_only`, `representation_only`, `many`, `delimiter`, `internal_name`, `internal_namespace`, `conditions`, `validators`, `verbose_name`, `help_text`. Do not validate inside the constructor — let Fields and Validators do it.

## Validation

| Need                         | Mechanism                                                              |
| ---------------------------- | ---------------------------------------------------------------------- |
| Reusable rule                | A class in `RESTAPI/Validators/` attached via `validators: [...]`.     |
| Field-specific, not reusable | `validate_<field_name>($value): <type>` returning the validated value. |
| Cross-field                  | `validate_extra(): void`.                                              |

All three throw `RESTAPI\Responses\ValidationError` with a stable, namespaced `response_id`.

## `apply()` and Dispatchers

If the Model mutates a service (filter, DNS, services, certs, HA), implement:

```php
public function apply(): void {
    (new <Area>ApplyDispatcher(async: $this->async))->spawn_process();
}
```

Set `$this->always_apply = true;` for singleton settings Models that should apply on every successful update.

Never call `filter_configure*()`, `services_*_configure()`, or other long-running pfSense functions directly from a Model — wrap them in a Dispatcher.

## Non-config-backed Models

If the Model does not store data in `$config`, set `$this->internal_callable = 'method_name';` and provide that method returning an array (single object, or list of arrays for `many` Models). Override `_create()` / `_update()` / `_delete()` only when needed.

## Hard rules

- No bare `exec()` / `shell_exec()` / `passthru()`. Use `RESTAPI\Core\Command`.
- No pfSense Plus-only functions or paths.
- Sensitive values must use `sensitive: true` on the Field.
- Errors throw `RESTAPI\Responses\*` with a stable `UPPER_SNAKE_CASE` `response_id`.
- Never edit generated files in the pfSense webroot — they are produced by `manage.php buildendpoints` from your Endpoint class.
