; -*- lisp -*-
; ============================================================
;  EXAMPLE 68 — Phase 2: Four tracks
; ============================================================

; ============================================================
;  Track R: Syntax objects
; ============================================================
(display "=== Track R: Syntax objects ===") (newline)

(define stx (datum->syntax #f '(+ 1 2)))
(display "  (syntax? stx): ") (display (syntax? stx)) (newline)
(display "  (syntax-e stx): ") (display (syntax-e stx)) (newline)
(display "  (syntax? 42): ") (display (syntax? 42)) (newline)
(display "  (syntax->datum of non-syntax): ") (display (syntax->datum 42)) (newline)
(newline)

; ============================================================
;  Track C: Persistent data structures
; ============================================================
(display "=== Track C: Persistent vectors ===") (newline)

(define v (pvec 10 20 30 40 50))
(display "  v: ") (display v) (newline)
(display "  (pvec-ref v 2): ") (display (pvec-ref v 2)) (newline)
(display "  (pvec-count v): ") (display (pvec-count v)) (newline)

(define v2 (pvec-set v 2 99))
(display "  v2 = (pvec-set v 2 99): ") (display v2) (newline)
(display "  v unchanged: ") (display v) (newline)

(define v3 (pvec-push v 60))
(display "  v3 = (pvec-push v 60): ") (display v3) (newline)
(display "  (pvec->list v3): ") (display (pvec->list v3)) (newline)
(newline)

(display "=== Track C: Persistent hash maps ===") (newline)

(define m (phash-map 'name "Alice" 'age 30 'city "Monza"))
(display "  m: ") (display m) (newline)
(display "  (phash-get m 'name): ") (display (phash-get m 'name)) (newline)
(display "  (phash-get m 'age): ") (display (phash-get m 'age)) (newline)
(display "  (phash-get m 'missing 'default): ") (display (phash-get m 'missing 'default)) (newline)
(display "  (phash-count m): ") (display (phash-count m)) (newline)
(display "  (phash-keys m): ") (display (phash-keys m)) (newline)

(define m2 (phash-set m 'age 31))
(display "  m2 = (phash-set m 'age 31):") (newline)
(display "    (phash-get m2 'age): ") (display (phash-get m2 'age)) (newline)
(display "    (phash-get m 'age): ") (display (phash-get m 'age)) (newline)
(display "    ↑ original unchanged") (newline)
(newline)

; ============================================================
;  Track K: Kernel type checker
; ============================================================
(display "=== Track K: Kernel type checker ===") (newline)

(display "  (kernel-infer 0): ") (display (kernel-infer 0)) (newline)
(display "  (kernel-infer 3): ") (display (kernel-infer 3)) (newline)
(display "  (kernel-infer 'Nat): ") (display (kernel-infer 'Nat)) (newline)
(display "  (kernel-check 0 'Nat): ") (display (kernel-check 0 'Nat)) (newline)
(display "  (kernel-check 'Nat (Sort 0)): ") (display (kernel-check 'Nat '(Sort 0))) (newline)
(display "  (kernel-infer (refl 0)): ") (display (kernel-infer '(refl 0))) (newline)
(newline)

(display "=== End of Phase 2 example ===") (newline)
