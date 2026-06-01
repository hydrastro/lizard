; 185-berlekamp-massey.lisp -- shortest linear recurrence of a sequence, and its closed form.
;
; Berlekamp-Massey discovers, from raw terms, the SHORTEST linear recurrence that
; generates a sequence (the minimal LFSR).  It is the exact dual of the linear-recurrence
; solver: this finds the recurrence, that solves it.  Composed, the pipeline turns a list
; of numbers into a closed form.  The discovered recurrence is certified by replaying it
; against every given term.  `must` raises on failure.

(import "cas/berlekamp-massey.lisp")
(import "cas/linrec.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'bm-check-failed)))

(display "Berlekamp-Massey: discovering linear recurrences") (newline) (newline)

(display "1. classic sequences") (newline)
(display "    Fibonacci 0,1,1,2,3,5,8,13 -> ") (display (bm->string (list 0 1 1 2 3 5 8 13))) (newline)
(display "    n^2 0,1,4,9,16,25         -> ") (display (bm->string (list 0 1 4 9 16 25))) (newline)
(must "Fibonacci recurrence is (1,1)"   (equal? (bm-recurrence (list 0 1 1 2 3 5 8 13)) (list 1 1)))
(must "Fibonacci order is 2"            (= (bm-order (list 0 1 1 2 3 5 8 13)) 2))
(must "Lucas has the same recurrence"   (equal? (bm-recurrence (list 2 1 3 4 7 11 18 29)) (list 1 1)))
(must "geometric 1,2,4,8,16 is (2)"     (equal? (bm-recurrence (list 1 2 4 8 16 32)) (list 2)))
(must "n^2 recurrence is (3,-3,1)"      (equal? (bm-recurrence (list 0 1 4 9 16 25)) (list 3 -3 1)))
(must "all certified"                   (and (bm-ok? (list 0 1 1 2 3 5 8 13)) (bm-ok? (list 1 2 4 8 16 32)) (bm-ok? (list 0 1 4 9 16 25))))
(newline)

(display "2. discovered recurrence reproduces and extends the data") (newline)
(define s (list 0 1 5 19 65 211 665))
(define rec (bm-recurrence s))
(display "    3^n-2^n: 0,1,5,19,... -> recurrence ") (display rec) (newline)
(must "recurrence is (5,-6)"            (equal? rec (list 5 -6)))
(must "replays the data"                (bm-ok? s))
(newline)

(display "3. composition: Berlekamp-Massey then linrec gives a closed form") (newline)
(define form (crec-solve rec (list 0 1)))
(display "    closed form a_n = ") (display (crec->string form)) (newline)
(must "closed form predicts 3^10 - 2^10" (= (eval-closed form 10) (- (expt 3 10) (expt 2 10))))
(define recn (bm-recurrence (list 0 1 2 3 4 5)))
(define formn (crec-solve recn (list 0 1)))
(display "    sequence n: BM ") (display recn) (display " -> closed form ") (display (crec->string formn)) (newline)
(must "n-sequence closed form gives 7 at n=7" (= (eval-closed formn 7) 7))
(newline)

(display "all Berlekamp-Massey checks passed.") (newline)
