; 190-discrete-log.lisp -- primitive roots and the discrete logarithm mod a prime.
;
; A primitive root g modulo a prime p generates the whole multiplicative group: g is
; primitive iff g^((p-1)/q) != 1 (mod p) for every prime q dividing p-1.  The discrete
; logarithm -- the x with g^x = h (mod p) -- is found by Shanks' baby-step giant-step
; method in about sqrt(p) work.  Both are certified: the log by raising g back to the
; recovered exponent, the primitive root by confirming its order is exactly p-1.
; `must` raises on failure.

(import "cas/discretelog.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'dlog-check-failed)))

(display "Primitive roots and the discrete logarithm") (newline) (newline)

(display "1. primitive roots (smallest generator), order verified") (newline)
(display "    mod 7 -> ") (display (primitive-root 7)) (display ", mod 11 -> ") (display (primitive-root 11)) (display ", mod 41 -> ") (display (primitive-root 41)) (newline)
(must "primitive root mod 7 is 3"     (= (primitive-root 7) 3))
(must "primitive root mod 11 is 2"    (= (primitive-root 11) 2))
(must "primitive root mod 13 is 2"    (= (primitive-root 13) 2))
(must "order of g mod 17 is 16"       (primitive-root-ok? 17))
(must "order of g mod 41 is 40"       (primitive-root-ok? 41))
(must "5 is not a primitive root mod 11" (not (is-primitive-root? 5 11)))
(newline)

(display "2. discrete logs, verified by g^x = h") (newline)
(display "    log_3(5) mod 7 = ") (display (dlog->string 3 5 7)) (display ",  log_2(9) mod 11 = ") (display (dlog->string 2 9 11)) (newline)
(must "log_3(5) mod 7 = 5"            (= (discrete-log 3 5 7) 5))
(must "log_2(9) mod 11 = 6"          (= (discrete-log 2 9 11) 6))
(must "log_3(1) mod 7 = 0"           (= (discrete-log 3 1 7) 0))
(must "log_5(3) mod 23 verified"     (discrete-log-ok? 5 3 23))
(must "log_6(7) mod 41 verified"     (discrete-log-ok? 6 7 41))
(newline)

(display "3. round trip at a larger prime (baby-step giant-step)") (newline)
(define g (primitive-root 1009))
(define secret 123)
(define h (mod-exp g secret 1009))
(display "    g = ") (display g) (display ", g^123 mod 1009 = ") (display h) (display ", recovered exponent = ") (display (discrete-log g h 1009)) (newline)
(must "primitive root mod 1009 certified" (primitive-root-ok? 1009))
(must "baby-step giant-step recovers 123" (= (discrete-log g h 1009) secret))
(must "and the recovery is certified"     (discrete-log-ok? g h 1009))
(newline)

(display "4. honest report when h is not a power of g") (newline)
(must "log_4(3) mod 7 has no solution" (equal? (discrete-log 4 3 7) 'none))
(must "the no-solution case is certified" (discrete-log-ok? 4 3 7))
(newline)

(display "all discrete-log checks passed.") (newline)
