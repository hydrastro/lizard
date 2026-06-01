; 195-frobenius.lisp -- the Frobenius number and numerical semigroups.
;
; With coin denominations of gcd 1, the Frobenius number is the largest amount that cannot
; be paid exactly.  It is computed from the Apery set by a Bellman-Ford relaxation modulo
; the smallest coin, which also yields the genus (count of unpayable amounts) and an exact
; representability test.  The answer is cross-checked two ways: against the closed form
; ab - a - b for two coins, and by confirming the Frobenius number is unpayable while the
; next m amounts are all payable.  `must` raises on failure.

(import "cas/frobenius.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'frobenius-check-failed)))

(display "The Frobenius number and numerical semigroups") (newline) (newline)

(display "1. two coins -- general algorithm vs the closed form ab - a - b") (newline)
(display "    coins 3,5  -> ") (display (frobenius->string (list 3 5)))  (newline)
(display "    coins 4,7  -> ") (display (frobenius->string (list 4 7)))  (newline)
(display "    coins 11,13 -> ") (display (frobenius->string (list 11 13))) (newline)
(must "Frobenius(3,5) = 7"      (= (frobenius (list 3 5)) 7))
(must "Frobenius(4,7) = 17"     (= (frobenius (list 4 7)) 17))
(must "Frobenius(11,13) = 119"  (= (frobenius (list 11 13)) 119))
(must "Apery agrees with ab-a-b for (3,5)"   (frobenius-two-ok? 3 5))
(must "Apery agrees with ab-a-b for (8,15)"  (frobenius-two-ok? 8 15))
(must "Apery agrees with ab-a-b for (17,19)" (frobenius-two-ok? 17 19))
(newline)

(display "2. the Chicken McNugget number for 6, 9, 20") (newline)
(display "    coins 6,9,20 -> ") (display (frobenius->string (list 6 9 20))) (newline)
(must "Frobenius(6,9,20) = 43"  (= (frobenius (list 6 9 20)) 43))
(must "43 is not payable"       (not (representable? 43 (list 6 9 20))))
(must "every amount 44..60 is payable"
      (and (representable? 44 (list 6 9 20)) (representable? 50 (list 6 9 20)) (representable? 60 (list 6 9 20))
           (representable? 47 (list 6 9 20)) (representable? 53 (list 6 9 20))))
(must "small unpayable amounts: 43, 37, 1 not payable"
      (and (not (representable? 43 (list 6 9 20))) (not (representable? 37 (list 6 9 20))) (not (representable? 1 (list 6 9 20)))))
(newline)

(display "3. genus and gap certificates across coin sets") (newline)
(display "    genus(6,9,20) = ") (display (genus (list 6 9 20))) (display ", genus(5,8,12) = ") (display (genus (list 5 8 12))) (newline)
(must "genus(4,7) = 9"  (= (genus (list 4 7)) 9))
(must "gap certificate holds for (6,9,20)"  (frobenius-gap-ok? (list 6 9 20)))
(must "gap certificate holds for (5,8,12)"  (frobenius-gap-ok? (list 5 8 12)))
(must "genus count matches for (6,9,20)"    (genus-count-ok? (list 6 9 20)))
(must "genus count matches for (7,11,13)"   (genus-count-ok? (list 7 11 13)))
(newline)

(display "all Frobenius checks passed.") (newline)
