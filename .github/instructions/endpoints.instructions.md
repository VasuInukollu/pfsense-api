---
applyTo: "pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Endpoints/**/*.inc"
---

# Endpoints — `RESTAPI\Endpoints\*`

Endpoints are **thin URL → Model adapters**. Most are 10–25 lines.

For full guidance and worked examples see:

- `.github/skills/endpoint-model-field.md`
- `docs/BUILDING_CUSTOM_ENDPOINT_CLASSES.md`

## Required shape

```php
<?php

namespace RESTAPI\Endpoints;

require_once 'RESTAPI/autoloader.inc';

use RESTAPI\Core\Endpoint;

class <Area><Resource>Endpoint extends Endpoint {
    public function __construct() {
        $this->url = '/api/v2/<area>/<resource>';
        $this->model_name = '<ModelClassName>';
        $this->request_method_options = [/* GET, POST, PATCH, PUT, DELETE */];
        # Optional: $this->many, $this->*_help_text, $this->auth_methods, ...
        parent::__construct();
    }
}
```

## Naming and URLs

- Singular endpoint: `<Area><Resource>Endpoint` → `/api/v2/<area>/<resource>`.
- Collection endpoint (`many: true`): pluralize, e.g. `FirewallAliasesEndpoint` → `/api/v2/firewall/aliases`.
- Apply endpoint: `<Area>ApplyEndpoint` → `/api/v2/<area>/apply`.
- File name = class name + `.inc`.

## Method semantics (enforced in `Core/Endpoint.inc::check_construct`)

| `many`  | Method   | Model method called |
| ------- | -------- | ------------------- |
| `false` | `GET`    | `read()`            |
| `false` | `POST`   | `create()`          |
| `false` | `PATCH`  | `update()`          |
| `false` | `DELETE` | `delete()`          |
| `true`  | `GET`    | `read_all()`        |
| `true`  | `PUT`    | `replace_all()`     |
| `true`  | `DELETE` | `delete_many()`     |

Setting `$this->many = true;` while `$model->many` is `false` throws `ENDPOINT_MANY_WITHOUT_MANY_MODEL`.

## Hard rules

- **Never** override `get()`, `post()`, `patch()`, `put()`, or `delete()`. There are zero such overrides in `Endpoints/` today.
- **Never** put business logic in the Endpoint. All behavior belongs in the Model (or in a Dispatcher invoked from the Model).
- **Never** hand-roll auth/ACL/privilege handling. Use `requires_auth`, `auth_methods`, `ignore_*` properties.
- **Never** hard-code privilege names. They are derived from `$url` + method automatically.
- **Never** edit the generated PHP files in the pfSense webroot. They are produced by `manage.php buildendpoints`.

## Optional Endpoint properties

Set when needed, leave default otherwise: `requires_auth`, `auth_methods`, `ignore_read_only`, `ignore_interfaces`, `ignore_enabled`, `ignore_acl`, `tag` (overrides default OpenAPI tag derived from URL), `*_help_text`, `deprecated`, `limit`, `offset`, `sort_by`, `sort_order`, `sort_flags`, `encode_content_handlers`, `decode_content_handlers`, `resource_link_set`. See `Core/Endpoint.inc` PHPDoc.

## Help text

Set `$this->get_help_text`, `$this->post_help_text`, `$this->patch_help_text`, `$this->put_help_text`, `$this->delete_help_text` only when the auto-generated default (built from the Model's `verbose_name`/`verbose_name_plural`) is misleading.
