; -*- lisp -*-
; ============================================================
;  EXAMPLE 81 — lizard v5 complete cheat sheet
; ============================================================

(display "╔═══════════════════════════════════════╗") (newline)
(display "║  lizard v5 — Complete Cheat Sheet      ║") (newline)
(display "╚═══════════════════════════════════════╝") (newline)
(newline)

; ─── ARITHMETIC ───
(display "Arithmetic: + - * / % abs expt gcd lcm") (newline)
(display "  (+ 1 2 3) = ") (display (+ 1 2 3)) (newline)
(display "  (expt 2 10) = ") (display (expt 2 10)) (newline)
(display "  (gcd 12 8) = ") (display (gcd 12 8)) (newline)
(newline)

; ─── PREDICATES ───
(display "Predicates: zero? positive? negative? even? odd?") (newline)
(display "  (zero? 0) = ") (display (zero? 0)) (newline)
(display "  (even? 42) = ") (display (even? 42)) (newline)
(display "  (odd? 7) = ") (display (odd? 7)) (newline)
(display "  (positive? 5) = ") (display (positive? 5)) (newline)
(display "  (negative? -3) = ") (display (negative? -3)) (newline)
(newline)

; ─── COMPARISON ───
(display "Comparison: = < > <= >= min max") (newline)
(display "  (min 3 7) = ") (display (min 3 7)) (newline)
(display "  (max 3 7) = ") (display (max 3 7)) (newline)
(newline)

; ─── LISTS ───
(display "Lists: cons car cdr list length append reverse") (newline)
(display "  map filter fold-left for-each apply") (newline)
(display "  assoc member list-ref iota list?") (newline)
(display "  (map (lambda (x) (* x x)) '(1 2 3)) = ")
(display (map (lambda (x) (* x x)) '(1 2 3))) (newline)
(display "  (filter even? (iota 10 1)) = ")
(display (filter even? (iota 10 1))) (newline)
(display "  (fold-left + 0 '(1 2 3 4 5)) = ")
(display (fold-left + 0 '(1 2 3 4 5))) (newline)
(newline)

; ─── STRINGS ───
(display "Strings: string-length string-append substring") (newline)
(display "  string-ref string-split string-join") (newline)
(display "  string-upcase string-downcase string-reverse") (newline)
(display "  string-contains? string->list list->string") (newline)
(display "  char->integer integer->char") (newline)
(display "  (string-split \"a:b:c\" \":\") = ")
(display (string-split "a:b:c" ":")) (newline)
(display "  (string-join '(\"x\" \"y\" \"z\") \"-\") = ")
(display (string-join '("x" "y" "z") "-")) (newline)
(display "  (string-reverse \"hello\") = ")
(display (string-reverse "hello")) (newline)
(display "  (char->integer \"A\") = ") (display (char->integer "A")) (newline)
(display "  (integer->char 65) = ") (display (integer->char 65)) (newline)
(display "  (string->list \"hi\") = ") (display (string->list "hi")) (newline)
(display "  (list->string '(\"h\" \"i\")) = ") (display (list->string '("h" "i"))) (newline)
(newline)

; ─── PERSISTENT DATA ───
(display "Persistent: pvec pvec-ref pvec-set pvec-push") (newline)
(display "  phash-map phash-get phash-set phash-keys") (newline)
(define v (pvec 1 2 3))
(display "  (pvec 1 2 3) = ") (display v) (newline)
(display "  (pvec-set v 1 99) = ") (display (pvec-set v 1 99)) (newline)
(define m (phash-map 'a 1 'b 2))
(display "  (phash-get m 'a) = ") (display (phash-get m 'a)) (newline)
(newline)

; ─── ATOMS ───
(display "Atoms: atom deref swap! reset! atom?") (newline)
(define c (atom 0))
(swap! c (lambda (n) (+ n 1)))
(display "  (deref (atom 0) after swap!) = ") (display (deref c)) (newline)
(newline)

; ─── VM ───
(display "VM: vm-eval disassemble vm-time time-eval profile") (newline)
(display "  (vm-eval '(+ 2 3)) = ") (display (vm-eval '(+ 2 3))) (newline)
(newline)

; ─── MODULES ───
(display "Modules: import load module-loaded? add-module-path!") (newline)
(newline)

; ─── GC ───
(display "GC: gc gc-stats") (newline)
(newline)

; ─── KERNEL ───
(display "Kernel: kernel-infer kernel-check kernel-reduce") (newline)
(display "  kernel-equal? kernel-define kernel-lookup") (newline)
(display "  kernel-hole kernel-solve kernel-zonk kernel-unify") (newline)
(display "  begin-proof proof-state tactic-intro tactic-exact") (newline)
(display "  tactic-refl tactic-assumption qed") (newline)
(newline)

; ─── SYNTAX ───
(display "Syntax: datum->syntax syntax->datum syntax-e syntax? syntax-source") (newline)
(newline)

; ─── EXCEPTIONS ───
(display "Exceptions: raise try guard error-location") (newline)
(newline)

; ─── LAZY ───
(display "Lazy: delay force promise?") (newline)
(newline)

(display "Total: 500+ primitives across 10 subsystems.") (newline)
(display "Type 'kernel-infer', 'vm-eval', 'gc', 'profile' to explore!") (newline)
