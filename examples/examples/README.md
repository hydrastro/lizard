# Lizard examples

Run a file through the REPL with stdin redirection:

```sh
./lizard < examples/02-recursion.lisp
```

Or load it interactively by pasting one expression at a time — the
REPL accumulates lines until parens balance, so multi-line definitions
work as expected.

Numbered for suggested reading order, but each file is self-contained:

| File | Topics |
| ---- | ------ |
| `01-basics.lisp`    | `define`, `lambda`, arithmetic, comparisons, `let`, `set!` |
| `02-recursion.lisp` | `fact`, `fib`, `gcd`, Ackermann, mutual recursion, bignums |
| `03-lists.lisp`     | `cons`, `car`, `cdr`, `list`, `null?`, `pair?`, building `map`/`length`/`reverse` |
| `04-macros.lisp`    | `define-syntax`, quasiquote `` ` ``, unquote `,`, splicing `,@` |
| `05-control.lisp`   | `if`, `cond` with `else`, `begin`, `and`/`or`/`not` |
