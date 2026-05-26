; -*- lisp -*-
; Tower of Hanoi — prints every move.
;
; (hanoi n FROM TO VIA) = solve with n disks moving from FROM to TO,
; using VIA as the spare peg.

(define (hanoi n from to via)
  (if (= n 0)
      '()  ; nil — keeps script output clean by suppressing => done
      (begin
        (hanoi (- n 1) from via to)
        (display "move disk ") (display n)
        (display " from ") (display from)
        (display " to ")     (display to)
        (newline)
        (hanoi (- n 1) via to from))))

(display "=== 3 disks (7 moves) ===") (newline)
(hanoi 3 'A 'C 'B)
(newline)

(display "=== 4 disks (15 moves) ===") (newline)
(hanoi 4 'A 'C 'B)
(newline)

(display "=== 5 disks (31 moves) ===") (newline)
(hanoi 5 'A 'C 'B)
