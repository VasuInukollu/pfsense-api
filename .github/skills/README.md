# Skills for Contributors and AI Agents

This directory contains task-oriented playbooks for contributing to **pfSense-pkg-RESTAPI**. Each skill answers one question: _"how do I do X the way the rest of the codebase does it?"_

Skills complement, but never replace:

- **`AGENTS.md`** (repo root) — the rules and architectural overview.
- **`.github/instructions/`** — GitHub Copilot path-scoped instructions that auto-apply when you edit specific directories.
- **`docs/CONTRIBUTING.md`** + **`docs/BUILDING_CUSTOM_*.md`** — the human-facing reference docs published at https://pfrest.org.
- **`https://pfrest.org/php-docs/`** — generated PHPDoc for every class.

## Skill index

| File                               | Use it when...                                                                                                     |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `anatomy-of-a-feature.md`          | You're adding a brand-new resource end-to-end (Model → Endpoint → Validators → Dispatcher → Tests). Start here.    |
| `endpoint-model-field.md`          | You need a quick reminder of the Endpoint/Model/Field rules and the most common patterns.                          |
| `validation-command-dispatcher.md` | You're writing reusable validation, running shell commands, or implementing background apply behavior.             |
| `writing-tests.md`                 | You're adding or updating `Tests/*TestCase.inc` files.                                                             |
| `platform-testing-legal.md`        | You need a refresher on pfSense CE-only constraints, the FreeBSD build environment, and how the test runner works. |

## Routing your task

| If your task is...                                              | Start with                                                                                          |
| --------------------------------------------------------------- | --------------------------------------------------------------------------------------------------- |
| Add a new pfSense resource exposed over the API                 | `anatomy-of-a-feature.md`                                                                           |
| Add or change a Field, Endpoint, or Model attribute             | `endpoint-model-field.md`                                                                           |
| Reuse or add a `Validators/` class                              | `validation-command-dispatcher.md`                                                                  |
| Replace a `shell_exec`/`exec` call                              | `validation-command-dispatcher.md`                                                                  |
| Reload pfSense services after changes                           | `validation-command-dispatcher.md` (Dispatcher section)                                             |
| Write/expand `Tests/` coverage                                  | `writing-tests.md`                                                                                  |
| Confirm something is allowed (CE vs Plus, runtime requirements) | `platform-testing-legal.md`                                                                         |
| Tweak OpenAPI/GraphQL/HATEOAS output                            | `endpoint-model-field.md` (schema is auto-generated from Field/Endpoint metadata; do not hand-edit) |

## Quick AI prompt

When asking an AI assistant to make changes here, paste this preamble:

```text
Follow AGENTS.md and .github/skills strictly. Project rules:
- Endpoint stays thin: only $url, $model_name, $request_method_options, $many, optional auth/help.
- Never override Endpoint::get/post/patch/put/delete.
- All schema/types/defaults/choices/constraints live on Field objects in the Model constructor.
- Reusable validation goes in Validators/; model-specific validation uses validate_<field>() / validate_extra().
- Shell calls use RESTAPI\Core\Command, never bare exec/shell_exec/passthru.
- Long-running apply work is invoked from Model::apply() via (new XApplyDispatcher(async: $this->async))->spawn_process().
- Errors throw a Responses/* class with a stable UPPER_SNAKE_CASE response_id.
- Sensitive Fields use sensitive: true and never go through logs.
- Tests live in pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/Tests/, extend RESTAPI\Core\TestCase, file & class named API<Subns><Class>TestCase.
- pfSense CE only. No pfSense Plus references.
- All PHP files end in .inc, start with <?php + namespace + require_once 'RESTAPI/autoloader.inc';
- Comments use # for inline notes, /** ... */ PHPDoc on public symbols.
- Format with Prettier (`@prettier/plugin-php`) before submitting.
```
