# Lizard

Lizard Lisp wizard; a Scheme interpreter library written in C89.

```
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
```

## Layout

```
.
├── include/
│   └── lizard.h         Public API header.
├── src/                 Implementation + private headers.
│   ├── lizard.c         Evaluator (eval / apply / expand_macros / expand_quasiquote).
│   ├── parser.c         S-expression parser, tokens -> AST.
│   ├── tokenizer.c      Source -> token stream.
│   ├── primitives.c     Built-in procedures and `lizard_install_primitives`.
│   ├── env.c            Environment frames.
│   ├── mem.c            Bump-arena heap; GMP allocator integration.
│   ├── printer.c        AST debug printer + Scheme-style value printer.
│   ├── repl.c           Interactive REPL + script driver.
│   ├── errors.h         Error-code enum (X-macro list).
│   ├── en.h / lang.h    English message strings (i18n scaffolding).
│   └── …
├── tests/
│   ├── test_harness.h   TEST_ASSERT / TEST_ASSERT_EQ / TEST_ASSERT_STR.
│   ├── test_helpers.{h,c}  Façade for C unit tests.
│   ├── *_test.c         C-level tests linked against liblizard.a.
│   └── *.lisp + *.expected  Golden-output end-to-end tests.
├── examples/
│   ├── 01-basics.lisp … 06-scripting.lisp
│   └── prelude.lisp     User-level stdlib (map, filter, fold-left, range, …).
├── Makefile             Top-level build + dispatch to tests/tests.mk.
└── flake.nix            Nix flake (devShell + package).
```

## Dependencies

- [`hydrastro/ds`](https://github.com/hydrastro/ds) — intrusive data structures.
- [`gmp`](https://gmplib.org) — arbitrary precision integers.

Lizard's source uses its own typedefs (`lz_list_t`, `lz_list_node_t`)
anchored on the underlying struct tags `linked_list` and `list_node`,
so it builds against **either** the older ds (which exposes `list_t`)
**or** newer ds revisions (which expose `ds_list_t`) — no source edits
needed when ds is upgraded.

## Build

### With Nix (recommended)

```sh
nix develop
make            # build/liblizard.{a,so} and build/lizard
```

That's the whole flow — `nix develop` injects ds and gmp into the
compiler/linker search paths via Nix's standard wrappers, so `make`
needs no extra flags.

To upgrade the pinned ds revision (e.g. if you previously got
`unknown type name 'ds_list_node_t'` because `flake.lock` pinned an
older release):

```sh
nix flake update    # bumps ds in flake.lock
make clean && make
```

### Without Nix

```sh
make CPPFLAGS=-I/path/to/ds/include \
     LDFLAGS=-L/path/to/ds/lib
```

### Other Make targets

```sh
make test       # build + run all C and golden-output tests
make examples   # run every examples/*.lisp through the REPL
make install    # PREFIX=/usr/local by default
make clean      # remove build/
make distclean  # also remove gmon.out, vgcore.*, *~, etc. (calls scripts/clean.sh)
```

## Run

```sh
# Interactive REPL
./build/lizard

# Run a script
./build/lizard < examples/06-scripting.lisp

# Or from inside a script via the load primitive
echo '(load "examples/prelude.lisp") (display (sum (range 1 101))) (newline)' | ./build/lizard
```

## Testing

```sh
make test         # all tests, exits non-zero if any fail
```

Two flavours of tests:

- **C unit tests** (`tests/*_test.c`) — link against `liblizard.a` via the
  `tests/test_helpers.h` façade and exercise the public API with
  `TEST_ASSERT*` macros.
- **Golden-output tests** (`tests/*.lisp` + `tests/*.expected`) — run a
  `.lisp` file through the REPL and diff against an expected output
  file. Useful for catching changes in user-visible behaviour
  (error messages, value printing, script output).

## Contributing

- Format with `clang-format` (use the project's `.clang-format` if present).
- Run `make test` before submitting.
- Run under Valgrind when touching memory paths:
  ```sh
  valgrind --leak-check=full ./build/lizard < tests/scripting.lisp
  ```
