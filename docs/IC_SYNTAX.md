# IC syntax — the interaction-calculus primitives

This is the complete surface syntax of the interaction-calculus core (`src/ic.c`,
`src/ic.h`), in two places it is exposed:

1. the **textual** language read by `ic_parse` (used by the demos and tests), and
2. the **Scheme** primitives `ic-normalize` / `ic-cost` / `ic-reduce` (wired in by
   `docs/ic_primitives.patch`), which read ordinary Lisp data.

The two surfaces describe the same calculus; only the concrete notation differs
(textual `(lam x …)` vs. Lisp `(lambda (x) …)`).

---

## 0. Did the syntax change? No.

Everything that parsed before Phase 14 parses **identically** and means the same
thing. Phase 14 is purely additive: it introduces first-class Σ (dependent-pair)
values, and with them three new forms — `pair`, `fst`, `snd`. No existing form is
renamed, removed, or given new behaviour. The only externally visible
consequences of the addition are:

- three new reserved words (`pair`, `fst`, `snd`) that, like `lam`/`op`/`dup`,
  can no longer be used as variable names; and
- one new shape in `ic-reduce` output: a pair value prints as `(pair a b)`.

If you never write `pair`/`fst`/`snd`, nothing about your programs or their
results changes.

---

## 1. Textual grammar (`ic_parse`)

Tokens are separated by whitespace; `(`, `)`, `{`, `}`, and `*` are
self-delimiting. Comments are not supported.

```ebnf
term   ::= NUM                                  ; exact integer, e.g. 0  -17  900000000000
         | NAME                                 ; a variable
         | '*'                                  ; the eraser, as a value
         | '(' app ')'                          ; application (see below)
         | '(' 'lam' NAME term ')'              ; abstraction   λx. body
         | '(' 'op'  OP  term term ')'          ; binary arithmetic / comparison
         | '(' 'dup' [label] NAME NAME term term ')'  ; dup x y = v in body
         | '{' [label] term term '}'            ; superposition  {a b}
         | '(' 'pair' term term ')'             ; Σ introduction   (a , b)      [Phase 14]
         | '(' 'fst'  term ')'                  ; Σ first projection   π₁ p     [Phase 14]
         | '(' 'snd'  term ')'                  ; Σ second projection  π₂ p     [Phase 14]
         | '(' 'transp' term ')'                ; transport / Id-by-observation [Phase 14b]

app    ::= term { term }                        ; left-associated: (f a b) = ((f a) b)

label  ::= ':' INT                              ; optional, e.g. :0  :1  :7
OP     ::= '+' | '-' | '*' | '/' | '%' | '=' | '<' | '>'
NUM    ::= ['-'] DIGIT { DIGIT }                ; arbitrary precision (GMP)
NAME   ::= any token that is not a reserved word, a number, or a delimiter
```

**Reserved words** (cannot be variable names): `lam` `op` `dup` `pair` `fst`
`snd` `transp`. (`sup` is *not* a textual keyword — superpositions are written
with braces, see below. It is only a keyword on the Scheme surface, §3.)

### The forms, one by one

| Form | Meaning | Example | Normal form |
|------|---------|---------|-------------|
| `NUM` | exact integer | `42` | `42` |
| `NAME` | variable (bound by `lam`/`dup`) | `x` | — |
| `*` | eraser (discards what it meets) | `*` | `*` |
| `(f a)` | application | `((lam x x) 7)` | `7` |
| `(lam x b)` | abstraction | `(lam x (op + x 1))` | a λ |
| `(op OP a b)` | arithmetic / comparison | `(op * 6 7)` | `42` |
| `{a b}` | superposition (label 0) | `{1 2}` | `{1 2}` |
| `{:L a b}` | superposition, label `L` | `{:1 1 2}` | `{1 2}` |
| `(dup x y v b)` | duplicate `v`, bind copies to `x`,`y` in `b` | `(dup x y 5 (op + x y))` | `10` |
| `(dup :L x y v b)` | same, with label `L` | `(dup :1 x y 5 (op * x y))` | `25` |
| `(pair a b)` | dependent pair (Σ intro) | `(pair 3 4)` | `(pair 3 4)` |
| `(fst p)` | first projection | `(fst (pair 3 4))` | `3` |
| `(snd p)` | second projection | `(snd (pair 3 4))` | `4` |
| `(transp v)` | transport / Id-by-observation (reflexive ⇒ identity) | `(transp 5)` | `5` |

### Arithmetic (`op`)

`+ - *` are the obvious integer operations. `/` is truncating integer division
(toward zero) and `%` the matching remainder; division or remainder **by zero
yields 0** rather than trapping. The comparisons `= < >` return `1` for true and
`0` for false. All arithmetic is arbitrary precision.

```
(op / 17 5)   => 3
(op % 17 5)   => 2
(op < 3 10)   => 1
(op = 4 5)    => 0
```

### Labels on `{…}` and `dup`

A label is an integer tag written `:N`. It controls the dual rule between
duplication and superposition, which is the operational core of the framework:

- a `dup` meeting a `{…}` of the **same** label *annihilates* — the two copies
  receive the two components (the search collapses);
- a `dup` meeting a `{…}` of a **different** label *commutes* — each side is
  duplicated and the two fan through one another.

A bare leading number inside braces is an ordinary element, **not** a label, so
`{2 3}` is the unlabelled superposition of `2` and `3`; to label, write `{:2 3 4}`.

```
(dup x y {10 20} (op + x y))      => 30     ; same label (both 0): annihilate
(dup x y {:1 10 20} (op + x y))   => {20 40} ; labels differ (dup 0 vs sup 1): commute
```

### Σ — pairs and projections (Phase 14)

`pair`/`fst`/`snd` are **first-class agents**, not an encoding: a pair is a
constructor node in the graph, and the projections are eliminators that react
with it (`fst` against a `pair` keeps the first field and erases the second;
`snd` does the reverse). Because a pair is an ordinary value, it shares and
duplicates like anything else — a pair bound once and used twice flows through a
`DUP`, which commutes past the pair and hands a copy to each use:

```
(fst (pair 3 4))                            => 3
(snd (pair (pair 1 2) 3))                   => 3
(pair (op + 1 2) (op * 5 5))                => (pair 3 25)
((lam p (op + (fst p) (snd p))) (pair 10 20)) => 30      ; pair shared via DUP
(fst {(pair 1 2) (pair 3 4)})               => {1 3}     ; projection over a superposition
```

### Transport — Id-by-observation (Phase 14b, partial)

`transp` is the transport agent. It computes by recursion on the value former it
meets — over Σ componentwise, over Π pointwise, trivially on the base — which is
the by-observation reduction of the identity type. Along a reflexivity path
transport is the identity, so `(transp v)` reduces to `v`:

```
(transp 5)                          => 5            ; base type
(transp (pair 3 4))                 => (pair 3 4)   ; Σ: transports each component
((transp (lam x (op + x 1))) 5)     => 6            ; Π: transp(λx.b) = λx.transp(b)
(transp {1 2})                      => {1 2}        ; distributes over a superposition
```

What is implemented now is this *structural, reflexive* core (and it is fuzz-
checked: wrapping any subterm in `transp` never changes its value). The full
identity type — transport along a *non-reflexive* path, the dependent second
component of Id-over-Σ, function extensionality as the Π case, and univalence as
the 𝒰 case — dispatches on the *type* former and is the remaining Phase 14b work,
validated against the cubical layer. See `docs/INET_ENGINE_PLAN.md`.

---

## 2. What a program *is* (so the syntax is not misleading)

The text above is **input notation**. After `compile`, a program is not a syntax
tree but an abstract syntax graph: agent nodes joined by wires, variables are the
wires themselves (no names survive), and a value used twice is one `DUP` node
shared by two edges rather than a duplicated subtree. `ic_dump_net` renders that
graph; see `docs/ic_graph_output.txt` and §0 of `docs/INET_ENGINE_PLAN.md`. The
grammar is how you *write* a program; the net is what *runs*.

---

## 3. Scheme primitives (`ic-normalize`, `ic-cost`, `ic-reduce`)

When `docs/ic_primitives.patch` is applied (it wires `ic` into the Scheme layer,
mirroring how `inet-normalize` is exposed today), three procedures become
available. Each takes **one quoted datum** describing a term:

| Procedure | Returns |
|-----------|---------|
| `(ic-normalize '<term>)` | the integer normal form (error if the result is not an integer) |
| `(ic-cost '<term>)` | the number of interactions taken to normalise (a fixnum) |
| `(ic-reduce '<term>)` | a string: the readback of the normal form (see §4) |

### Datum grammar (what the quote may contain)

```
<term> ::= <integer>                              ; exact integer literal
         | <symbol>                               ; a variable
         | (lambda (<symbol>) <term>)             ; abstraction (also accepts: lam)
         | (<op> <term> <term>)                   ; <op> ∈ + - * / % = < >
         | (<term> <term> ...)                    ; application (1+ arguments)
         | (sup <term> <term>)                    ; superposition, label 0
         | (sup <int> <term> <term>)              ; superposition with a label
         | (dup (<symbol> <symbol>) <term> <term>); dup x y = v in body, label 0
         | (dup <int> (<symbol> <symbol>) <term> <term>) ; with a label
         | (pair <term> <term>)                   ; Σ introduction          [Phase 14]
         | (fst <term>)                           ; Σ first projection      [Phase 14]
         | (snd <term>)                           ; Σ second projection     [Phase 14]
         | (transp <term>)                        ; transport / Id-by-obs.  [Phase 14b]
```

Notes:

- `lambda` and `lam` are both accepted for abstraction.
- Superposition uses the **symbol** `sup` here (there is no `{…}` reader in
  Scheme); the optional leading integer is the label.
- The arithmetic operators and their semantics are exactly those of §1
  (truncating `/`, `%`; division by zero → 0; comparisons → 1/0).

### Examples

```scheme
(ic-normalize '((lambda (x) (* x x)) 9))            ; => 81
(ic-normalize '(fst (pair (+ 1 2) (* 5 5))))        ; => 3
(ic-cost      '((lambda (x) (+ x x)) (+ 2 3)))      ; => a small fixnum (shared arg)
(ic-reduce    '(pair 3 4))                          ; => "(pair 3 4)"
(ic-reduce    '(sup 1 2))                           ; => "{1 2}"
(ic-normalize '(dup (a b) (pair 10 20) (+ (fst a) (snd b)))) ; => 30
```

---

## 4. Readback / output formats (`ic-reduce`, `ic_readback`)

`ic-reduce` renders the normal form to text. The shapes it can produce:

| Result | Rendering |
|--------|-----------|
| an integer | the number, e.g. `42` |
| an eraser | `*` |
| a superposition | `{a b}` (components rendered recursively) |
| a pair | `(pair a b)` (components rendered recursively) |
| an affine λ / application | `(lam body)`, `(f a)`, with bound variables as `#k` (de Bruijn level) |
| anything with residual sharing not expressible as a tree | a faithful **net dump**: `<net n0:TAG[p1 p2]; … >` |

The last row is the honest fallback: rather than guess a tree for a normal form
whose sharing a tree cannot express (the general optimal-readback problem, still
open — see `docs/INET_ENGINE_PLAN.md` §7), `ic_readback` prints the graph itself.
In a net dump, `nK` is a wire to node `K`, `#n` a number, `*` an eraser, `vK` a
wire junction, and `~` an open wire. Integer programs — including all the
arithmetic and Σ examples above — read back as plain values; the net form only
appears for genuinely shared higher-order normal forms.
