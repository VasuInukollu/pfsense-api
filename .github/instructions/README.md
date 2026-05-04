---
applyTo: ".github/instructions/**/*.md"
---

# About `.github/instructions/`

This directory contains **GitHub Copilot path-scoped instructions**. Each `*.instructions.md` file declares an `applyTo` glob in its YAML frontmatter; Copilot automatically loads the matching file when you edit a file under that path.

## Files

| File                          | `applyTo` glob                                             | Purpose                                                   |
| ----------------------------- | ---------------------------------------------------------- | --------------------------------------------------------- |
| `php.instructions.md`         | `pfSense-pkg-RESTAPI/files/usr/local/pkg/RESTAPI/**/*.inc` | Global PHP rules for all package source files.            |
| `models.instructions.md`      | `.../RESTAPI/Models/**/*.inc`                              | Model class authoring rules.                              |
| `endpoints.instructions.md`   | `.../RESTAPI/Endpoints/**/*.inc`                           | Endpoint class authoring rules.                           |
| `validators.instructions.md`  | `.../RESTAPI/Validators/**/*.inc`                          | Reusable Validator authoring rules.                       |
| `dispatchers.instructions.md` | `.../RESTAPI/Dispatchers/**/*.inc`                         | Background Dispatcher authoring rules.                    |
| `tests.instructions.md`       | `.../RESTAPI/Tests/**/*.inc`                               | TestCase authoring rules (custom framework, not PHPUnit). |

## How to extend

1. Copy an existing `*.instructions.md` and edit the `applyTo` glob.
2. Keep each file focused on one path scope. Don't duplicate `AGENTS.md` — link to it.
3. Keep the rules short and prescriptive. Long-form examples belong in `.github/skills/` or `docs/`.
4. After editing, verify the `applyTo` glob actually matches the file you expect — Copilot only loads the file when the glob matches.

## Authoritative sources

These files are guard-rails. The full picture is:

- `AGENTS.md` (root) — architecture and rules.
- `.github/skills/` — task playbooks with concrete examples.
- `docs/CONTRIBUTING.md` and `docs/BUILDING_CUSTOM_*.md` — human reference docs.
- `https://pfrest.org/php-docs/` — generated PHPDoc.
