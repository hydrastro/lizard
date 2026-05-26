; -*- lisp -*-
; ============================================================
;  Pi-BETA REDUCTION + ALPHA-EQUIVALENCE
; ============================================================
;
; The engine now supports lambda-calculus computation at the TT
; layer: `(Lambda 'x body)` introduces a function, `(@ f a)` is
; application, and the reducer fires
;
;    (@ (Lambda 'x body) a)  -->  body[a/x]
;
; via capture-avoiding substitution. Equal? sees through both
; pi-beta and alpha-renaming of binders.

; ------------------------------------------------------------
; 1. Identity function
; ------------------------------------------------------------

(define id (Lambda 'x 'x))
(display "id = ") (display id) (newline)
(display "(@ id 'a) = ") (display (reduce (@ id 'a))) (newline)
(display "(@ id (refl 'q)) = ") (display (reduce (@ id (refl 'q)))) (newline)
(newline)

; ------------------------------------------------------------
; 2. Constant function and composition
; ------------------------------------------------------------

(define K (Lambda 'x (Lambda 'y 'x)))    ; K x y = x
(display "K = ") (display K) (newline)
(display "((K 'a) 'b) = ")
(display (reduce (@ (@ K 'a) 'b))) (newline)

; Function composition: (compose f g) = \x. f (g x)
(define compose
  (Lambda 'f (Lambda 'g (Lambda 'x (@ 'f (@ 'g 'x))))))
(display "compose = ") (display compose) (newline)

; (compose id id) should equal id (up to alpha)
(display "(compose id id) 'q = ")
(display (reduce (@ (@ (@ compose id) id) 'q))) (newline)
(newline)

; ------------------------------------------------------------
; 3. Alpha-equivalence in action
; ------------------------------------------------------------

(display "=== Alpha-equivalence ===") (newline)
(display "(Lambda x x) = (Lambda y y)?: ")
(display (equal? (Lambda 'x 'x) (Lambda 'y 'y))) (newline)
(display "(Pi x A x) = (Pi y A y)?:     ")
(display (equal? (Pi 'x 'A 'x) (Pi 'y 'A 'y))) (newline)
(display "Nested: (Pi x (Pi y x)) = (Pi a (Pi b a))?: ")
(display (equal? (Pi 'x 'A (Pi 'y 'B 'x))
                 (Pi 'a 'A (Pi 'b 'B 'a))))
(newline)

; Free variables are NOT alpha-renamed
(display "(Pi x A y) ≠ (Pi x A z): ")
(display (equal? (Pi 'x 'A 'y) (Pi 'x 'A 'z))) (newline)
(newline)

; ------------------------------------------------------------
; 4. Capture avoidance — the famous subtle case
; ------------------------------------------------------------

(display "=== Capture avoidance ===") (newline)
; (Lambda 'y (Lambda 'x 'y)) applied to 'x:
;   naive: substitute 'x for 'y in (Lambda 'x 'y) gives (Lambda 'x 'x)
;          — CAPTURED! the result identity is wrong
;   correct: alpha-rename inner 'x first, then substitute
;          gives (Lambda 'x_N 'x) — the inner binder is now fresh
(display "input:    (@ (Lambda 'y (Lambda 'x 'y)) 'x)") (newline)
(display "naive bad: would be (Lambda 'x 'x) -- captured") (newline)
(display "got:      ")
(display (reduce (@ (Lambda 'y (Lambda 'x 'y)) 'x))) (newline)
(display "  ↑ the body is the free 'x, the binder was renamed fresh") (newline)
(newline)

; ------------------------------------------------------------
; 5. Mixing beta with identity-type rules
; ------------------------------------------------------------

(display "=== Beta + identity rules interacting ===") (newline)

; A Lambda that returns a refl proof
(define refl-of (Lambda 'x (refl 'x)))
(display "refl-of = ") (display refl-of) (newline)
(display "(@ refl-of 'a) = ")
(display (reduce (@ refl-of 'a))) (newline)

; A Lambda whose body has identity manipulations.
; sym-of-sym should cancel after beta substitutes the arg.
(define double-sym (Lambda 'p (Id-sym (Id-sym 'p))))
(display "double-sym = ") (display double-sym) (newline)
(display "(@ double-sym (refl 'a)) = ")
(display (reduce (@ double-sym (refl 'a)))) (newline)
(display "  ↑ beta + sym-involutive both fire") (newline)

; The composition trans(p, sym(p)) cancels to refl
; even when applied to an unknown argument
(define triv-loop (Lambda 'p (Id-trans 'p (Id-sym 'p))))
(display "triv-loop = ") (display triv-loop) (newline)
(display "(@ triv-loop (refl 'q)) = ")
(display (reduce (@ triv-loop (refl 'q)))) (newline)
(display "  ↑ beta + trans-inverse + sym-refl cascade") (newline)
(newline)

; ------------------------------------------------------------
; 6. Church-encoded booleans, just to see them work
; ------------------------------------------------------------

(define true_  (Lambda 't (Lambda 'f 't)))   ; pick first
(define false_ (Lambda 't (Lambda 'f 'f)))   ; pick second
(define if_   (Lambda 'b (Lambda 't (Lambda 'f (@ (@ 'b 't) 'f)))))

(display "Church booleans:") (newline)
(display "  (if true a b)  = ")
(display (reduce (@ (@ (@ if_ true_)  'a) 'b))) (newline)
(display "  (if false a b) = ")
(display (reduce (@ (@ (@ if_ false_) 'a) 'b))) (newline)
(newline)

; ------------------------------------------------------------
; 7. equal? sees through everything
; ------------------------------------------------------------

(display "=== Equality across reductions ===") (newline)
(display "id applied to refl x ≡ refl x: ")
(display (equal? (@ id (refl 'x)) (refl 'x))) (newline)
(display "K a b ≡ a: ")
(display (equal? (@ (@ K 'a) 'b) 'a)) (newline)
(display "double-sym (refl q) ≡ refl q: ")
(display (equal? (@ double-sym (refl 'q)) (refl 'q))) (newline)
(display "triv-loop (refl q) ≡ refl q: ")
(display (equal? (@ triv-loop (refl 'q)) (refl 'q))) (newline)
