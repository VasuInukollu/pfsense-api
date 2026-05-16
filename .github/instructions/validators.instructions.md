---
applyTo: "pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Validators/**/*.inc"
---

# Validators — `RESTAPI\Validators\*`

Reusable, Field-level validation classes. Attached to Fields via `validators: [new XValidator(...)]`.

For full guidance and worked examples see:

- `.github/skills/validation-command-dispatcher.md`
- `docs/BUILDING_CUSTOM_VALIDATOR_CLASSES.md`

## Required shape

```php
<?php

namespace RESTAPI\Validators;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Validator;
use RESTAPI\Responses\ValidationError;

/**
 * Defines a Validator that ensures <thing>.
 */
class <Thing>Validator extends Validator {
    public function __construct(
        public bool $allow_<option> = true,
        public array $allow_keywords = [],
    ) {}

    public function validate(mixed $value, string $field_name = ''): void {
        if (/* value is acceptable */) {
            return;
        }

        throw new ValidationError(
            message: "Field `$field_name` must be a valid <thing>, received `$value`.",
            response_id: '<THING>_VALIDATOR_FAILED',
        );
    }
}
```

## Conventions

- Class name ends in `Validator` (e.g. `IPAddressValidator`, `FilterNameValidator`).
- Use **public-promoted constructor properties** for options — that is the project convention.
- `validate()` returns `void` and **throws** `ValidationError` on failure.
- `response_id` is `UPPER_SNAKE_CASE` and **prefixed by the Validator name** so values stay globally unique (`IP_ADDRESS_VALIDATOR_FAILED`, `IP_ADDRESS_VALIDATOR_INVALID_PORT_SUFFIX`, ...).
- For optional sub-checks, label results on the Field via `$this->set_label('is_xxx')` (see `IPAddressValidator`) so downstream code can skip redundant work.

## Reuse first

Existing Validators: `EmailAddressValidator`, `FilterNameValidator`, `HexValidator`, `HostnameValidator`, `IPAddressValidator`, `LengthValidator`, `MACAddressValidator`, `NumericRangeValidator`, `RegexValidator`, `SubnetValidator`, `URLValidator`, `UniqueFromForeignModelValidator`, `X509Validator`. Compose options on existing Validators before adding a new one.

## When **not** to add a Validator

If the rule applies to exactly one Field on exactly one Model, use the Model's `validate_<field_name>(<value>): <type>` method or `validate_extra(): void` instead.

## Tests

Add a matching `Tests/APIValidators<Name>TestCase.inc` with `assert_throws_response(...)` for every failure path and `assert_does_not_throw(...)` for representative passing values. See `.github/skills/writing-tests.md` for examples.
