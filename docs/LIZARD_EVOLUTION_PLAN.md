# Lizard Evolution Plan

This is the practical roadmap for turning Lizard from a strong research prototype into a production-grade Lisp and proof-oriented language platform.

## Principle

Lizard should have a small trusted kernel and a large language-level tower. Keep C responsible for runtime, representation, parsing, expansion, evaluation/compilation, memory management, and embedding. Move derived language features into Lizard libraries whenever possible.

## Phase 0: repository and build hygiene

Status: started.

- Keep `nix develop && make && make test` as the primary workflow.
- Remove generated artefacts from source archives.
- Remove non-Lizard dynsys/TPCAS/Dear ImGui leftovers from the Lizard repository.
- Commit `flake.lock` for reproducibility.
- Add `make coverage`, `make profile`, `make MODE=asan test`, and `make MODE=debug`.
- Keep examples runnable from a fresh clone.

Exit criteria:

```sh
nix develop
make clean
make
make test
make examples
git status --short
```

## Phase 1: runtime/context object

Goal: make Lizard embeddable, testable, and eventually concurrent.

Introduce opaque public types:

```c
typedef struct lizard_runtime lizard_runtime_t;
typedef struct lizard_context lizard_context_t;
typedef struct lizard_value lizard_value_t;
```

Move global state into runtime/context objects:

- heap / allocator state
- global environment
- continuation state
- macro gensym counters
- source interner
- type-theory reduction flags
- diagnostics
- module registry

Exit criteria:

- Two independent Lizard contexts can exist in one process.
- Running code in one context cannot mutate another context.
- Tests no longer depend on process-global interpreter state.

## Phase 2: stable C API

Goal: make Lizard usable as an extension language.

Initial public API:

```c
lizard_runtime_t *lizard_runtime_new(const lizard_config_t *cfg);
void lizard_runtime_free(lizard_runtime_t *rt);
lizard_context_t *lizard_context_new(lizard_runtime_t *rt);
void lizard_context_free(lizard_context_t *ctx);
lizard_status_t lizard_eval_string(lizard_context_t *ctx, const char *source, lizard_value_t **out);
const char *lizard_last_error(lizard_context_t *ctx);
```

Do not expose AST internals from the stable header. Keep those in private headers.

## Phase 3: reader / syntax / expander / compiler separation

Goal: make macros, modules, source locations, debugging, and compilation clean.

Pipeline:

```text
source -> tokens -> datum -> syntax object -> expanded core syntax -> IR -> evaluator/VM
```

Syntax objects should carry:

- datum
- lexical context
- source span
- scopes / marks
- properties useful for macro debugging

Exit criteria:

- Macro expansion does not rely on reparsing ad-hoc AST nodes.
- Every syntax error can report file, line, column, and form context.

## Phase 4: hygienic macro system with ellipsis

Goal: language-oriented programming at production scale.

Implement:

- `syntax-rules` with ellipsis
- syntax objects with lexical context
- controlled escape hatches for unhygienic macros
- macro expansion tracing
- macro stepper output

Exit criteria:

- `when`, `unless`, `cond`, `case`, `let`, `let*`, `letrec`, `match`, and module forms can be written as macros.

## Phase 5: module and package system

Goal: prevent global namespace soup.

Module forms:

```scheme
(module name
  (export x y)
  ...)

(import name)
(import (only name x))
(import (rename name (x z)))
```

Package structure:

```text
lib/lizard/core/*.lisp
lib/lizard/match/*.lisp
lib/lizard/tt/*.lisp
lib/lizard/cubical/*.lisp
```

Exit criteria:

- The prelude is a module.
- Type-theory and cubical functionality live in modules, not the global default namespace.

## Phase 6: garbage collector

Goal: make long-running Lisp programs viable.

Start with a simple precise mark-sweep collector using explicit roots:

- current context
- global/module environments
- evaluator/trampoline frames
- temporary C roots
- interned symbols
- GMP-backed numbers

Then move toward generational collection.

Exit criteria:

- Recursive and macro-heavy programs do not grow memory without bound after collections.
- The C API exposes root guards for embedders.

## Phase 7: compiler and VM

Goal: stop being only a tree-walking interpreter.

Compiler pipeline:

```text
expanded core syntax -> ANF IR -> bytecode -> VM
```

Add:

- bytecode compiler
- bytecode disassembler
- source maps
- profiler hooks
- coverage counters
- later: native/LLVM backend

Exit criteria:

- REPL can evaluate through interpreter or VM.
- Tests pass in both modes.
- Bytecode disassembly is stable enough for debugging.

## Phase 8: debugger, profiler, coverage

Goal: production developer tooling.

Debugger:

- breakpoints
- stack inspection
- source spans
- local variables
- macro-expanded view
- restart integration

Profiler:

- sampling profiler if host supports it
- evaluator/VM counters everywhere else
- allocation profiler after GC exists

Coverage:

- per-form execution counters
- branch counters for `if`, `cond`, `match`
- coverage report in text first, HTML later

## Phase 9: immutable persistent data and concurrency

Goal: application-scale data/concurrency.

Data structures:

- persistent vector
- persistent hash map / hash array mapped trie
- persistent set
- persistent queue

Concurrency:

- immutable values share freely
- atoms for independent state
- refs + STM for coordinated state
- agents/tasks/futures for asynchronous work
- channels/actors later

Exit criteria:

- Concurrent programs can share Lizard values without copying or locks when immutable.
- Mutable state is explicit.

## Phase 10: proof assistant and dependent type layer

Goal: honest proof-oriented Lizard.

Minimum pieces:

- terms
- universes
- dependent function types
- dependent pair types
- identity/path types
- inductive types
- normalization/equality
- elaborator
- holes/metavariables
- proof state
- tactics or proof scripts

Even if the project is experimental, keep a soundness document listing trusted assumptions and known unsound features.

## Phase 11: CCHM cubical layer

Goal: a cubical computation layer inspired by CCHM.

Core pieces:

- interval terms
- face lattice
- cofibrations
- systems
- partial elements
- Kan composition
- transport
- homogeneous composition
- Glue types
- `ua` computation rule

Exit criteria:

- Examples document exactly which CCHM rules are implemented.
- Unsupported rules fail explicitly rather than silently reducing incorrectly.

## Non-goals for now

- Claiming full proof-assistant trust before the kernel is isolated.
- Claiming full CCHM before all interval/face/composition laws are represented and tested.
- Adding native code generation before a bytecode compiler and source maps exist.
