# Strictness Contract

Lizard deliberately keeps its C build hostile to accidental looseness.
Compiler warnings are treated as build failures because this project is meant
for embedders, future concurrency, and proof/type-theory experiments where
undefined behavior and ambiguous APIs are especially costly.

## Non-negotiable rules

- Do not remove warning flags for convenience.
- Do not add local `#pragma` warning suppressions to land a refactor.
- Do not silence a warning by hiding code behind unused variables or dummy casts
  unless the cast is required and documented by the C interface.
- Avoid aggregate-return APIs; use out-parameters for structs.
- Keep declarations before statements for C89 compatibility.
- Keep public prototypes inside include guards.
- Keep non-static functions declared in headers included by their `.c` files.
- Keep file splits behavior-preserving unless the change is deliberately tested
  as a semantic change.

## Expected validation loop

```sh
nix develop
make clean
make -j
make test
make examples
make smoke
./scripts/clean.sh --check
```

For the strictest local check, use:

```sh
make strict
```

`make strict` is intentionally not a softer path. It is the normal strict
build/test/example/smoke/lint chain bundled into one target.

## If a warning appears

Treat it as a design signal. Preferred fixes:

- change API shape, for example struct out-parameters instead of struct returns;
- split declarations into the right internal header;
- remove unused helpers after moving code;
- make ownership explicit rather than casting away `const`;
- make fallback behavior explicit rather than relying on silent defaults.

The rule is simple: if Lizard cannot satisfy its own warning policy, the code
is not ready to merge.

## Test harness strictness note

`TEST_ASSERT_EQ` intentionally casts its operands to `long`; callers must not
pass function calls returning enum types directly when `-Wbad-function-cast` is
enabled.  Store enum-returning function results in a local variable first, then
assert on that variable.

## Shared report writers

Text and JSON escaping for tooling reports must go through `report_writer.c`.
Do not duplicate escaping logic in individual report modules; duplicated
escaping tends to drift and creates subtle editor/tooling bugs.

## Report schema constants

Tooling-visible report type names and versions must be centralized in
`report_schema.c` / `report_schema.h`. Do not reintroduce hardcoded
`lizard-...` report names in individual report writers unless the schema
registry test is updated intentionally.


## Report/tooling strictness

Tooling report formats are part of the public debugging surface.  Schema names,
versions, and capabilities must be centralized in the report schema registry;
do not duplicate string literals or silently change schema versions in writer
modules.  Any new report schema must declare whether it supports stable text,
JSON, and the stable-v1 contract.
