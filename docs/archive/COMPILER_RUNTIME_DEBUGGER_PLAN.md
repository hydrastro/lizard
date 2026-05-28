# Compiler, Runtime, Debugger, Profiler, Coverage Plan

## Compiler

The compiler should start with bytecode, not LLVM/native code.

Pipeline:

```text
expanded syntax -> core syntax -> ANF IR -> bytecode -> VM
```

First bytecodes:

```text
CONST, LOAD_GLOBAL, LOAD_LOCAL, STORE_GLOBAL, CLOSURE, CALL, TAIL_CALL,
RETURN, JUMP, JUMP_IF_FALSE, POP, CONS, CAR, CDR
```

The bytecode format must carry source spans so debugger and coverage tooling work from day one.

## Runtime

Move toward:

```c
lizard_runtime_t   // allocator, intern table, module registry, GC state
lizard_context_t   // current env, stack/frames, diagnostics, options
lizard_vm_t        // bytecode execution state
```

A runtime can own many contexts. A context belongs to one runtime.

## Debugger

Minimum debugger support:

- backtrace of evaluator/VM frames
- current source span
- local bindings
- breakpoint table
- step in / step over / continue
- macro-expanded view of the current form

The debugger should be a Lizard library over a small C substrate, not a giant C subsystem.

## Profiler

Two layers:

1. Host profiler integration: `perf`, `gprof`, callgrind.
2. Language profiler: function/form counters, time, allocation counts, macro expansion time.

Start with counters. Sampling can come later.

## Coverage

Coverage needs source spans. Once every expanded form has a source location, inject counters during expansion or compilation.

Report shape:

```text
file.lisp:line:column  executed N times  form
```

HTML can wait. Plain text first.
