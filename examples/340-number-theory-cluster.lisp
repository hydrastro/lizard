; A cluster of classical number theory: the Mobius function and Dirichlet convolution (with Mobius inversion),
; perfect and amicable numbers, the Frobenius (Chicken McNugget) number for several coprime denominations, and
; the Stern-Brocot / Farey mediant structure -- each exact and arithmetically checked (docs/CAS.md).
(import "cas/numthy2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Classical number theory: Mobius, Dirichlet convolution, perfect/amicable, Frobenius, Stern-Brocot.") (newline) (newline)

(display "the Mobius function mu(n) and Mobius inversion (mu * 1)(n) = [n = 1]:") (newline)
(chk "mu(1)=1, mu(6)=1, mu(12)=0, mu(30)=-1" (if (= (moebius 1) 1) (if (= (moebius 6) 1) (if (= (moebius 12) 0) (= (moebius 30) -1) #f) #f) #f))
(chk "Mobius inversion: (mu*1)(1)=1, (mu*1)(12)=0, (mu*1)(100)=0" (if (= (dirichlet moebius n2-one 1) 1) (if (= (dirichlet moebius n2-one 12) 0) (= (dirichlet moebius n2-one 100) 0) #f) #f))

(display "perfect numbers (sigma(n) = 2n) and amicable pairs (each is the other's aliquot sum):") (newline)
(chk "6, 28, 496 are perfect; 12 is not" (if (perfect? 6) (if (perfect? 28) (if (perfect? 496) (if (perfect? 12) #f #t) #f) #f) #f))
(chk "(220, 284) are amicable" (amicable? 220 284))

(display "the Frobenius number -- the largest amount not payable in the given coprime denominations:") (newline)
(chk "frobenius2(3,5) = 7 (Sylvester: ab-a-b)" (= (frobenius2 3 5) 7))
(chk "frobenius(6,9,20) = 43 -- the Chicken McNugget number" (= (frobenius-list (list 6 9 20)) 43))
(chk "frobenius(5,8,9) = 12" (= (frobenius-list (list 5 8 9)) 12))

(display "the Stern-Brocot mediant and Farey adjacency (unimodular |bc - ad| = 1):") (newline)
(chk "mediant of 1/2 and 1/1 is 2/3" (let ((m (sb-mediant 1 2 1 1))) (if (= (car m) 2) (= (cdr m) 3) #f)))
(chk "1/3 and 1/2 are Farey neighbours; 1/3 and 2/3 are not" (if (farey-neighbours? 1 3 1 2) (if (farey-neighbours? 1 3 2 3) #f #t) #f))

(newline)
(display "Each result is exact: the Mobius and divisor functions are read from the factorization, the Frobenius") (newline)
(display "number from an exact Apery-set shortest-path over residues, the Farey adjacency from the unimodular") (newline)
(display "determinant -- classical number theory, computed and checked over the exact integers.") (newline)
