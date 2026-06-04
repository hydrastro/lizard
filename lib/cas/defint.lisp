; -*- lisp -*-
; lib/cas/defint.lisp -- CERTIFIED DEFINITE INTEGRALS by the Fundamental Theorem of Calculus: for a polynomial
; integrand f on [a, b], compute the antiderivative F, CERTIFY F' = f by differentiation (the arbiter), and return
; the value F(b) - F(a) together with a structured proof record (docs/CAS.md -- the proof-producing CAS: a definite
; integral as a theorem, every step certified, bottoming out at the FTC).
;
; This is the definite-integral companion to the symbolic integrators: where they decide elementarity and produce
; an antiderivative, this turns an antiderivative into a THEOREM about a number.  The proof has exactly the shape a
; human gives: INT_a^b f dx = F(b) - F(a) where F is an antiderivative of f.  The one step that could be wrong --
; "F is an antiderivative of f" -- is discharged by the differentiation arbiter (poly-deriv F must equal f exactly,
; an identity over Q), so the value is correct whenever the certificate holds.  Everything is exact rational
; arithmetic; the antiderivative of c_0 + c_1 x + ... + c_n x^n is c_0 x + c_1 x^2/2 + ... + c_n x^{n+1}/(n+1) (the
; integration constant fixed to 0), and the value F(b) - F(a) is an exact rational when a, b are rational.
;
; The proof record (the theorem with its justification) is
;     (list 'theorem (list 'definite-integral f a b) '= value
;           (list 'by 'FTC (list 'antiderivative F) (list 'certificate 'F-prime=f #t)))
; so a checker can re-verify the single nontrivial premise (F' = f) and recompute F(b) - F(a) independently.
;
; Public (f a polynomial coefficient list low->high; a, b rational endpoints):
;   dint-antideriv f          -> the antiderivative F of f with F(0) = 0 (a polynomial)
;   dint-certify f            -> #t iff (dint-antideriv f)' = f exactly (the FTC premise, by the differentiation arbiter)
;   dint-value f a b          -> the exact value F(b) - F(a) (a rational), assuming the certificate holds
;   dint-eval f a b           -> (list 'ok value) when the certificate holds, else (list 'uncertified ...)
;   dint-prove f a b          -> the full proof record (theorem + FTC justification + the F'=f certificate)
;   dint-recheck record       -> #t iff the proof record's certificate re-verifies and its value recomputes
;
; Verified: INT_0^1 x^2 dx = 1/3 (F = x^3/3, F' = x^2 certified); INT_0^1 x dx = 1/2; INT_{-1}^1 x^2 dx = 2/3;
; INT_0^2 (3x^2 + 2x + 1) dx = 8 + 4 + 2 = 14 (F = x^3 + x^2 + x); INT_a^a f = 0 for any f; the proof record of each
; re-checks (the certificate holds and the value recomputes).
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

; ----- the antiderivative F of f with F(0) = 0 -----
(define (dint-antideriv f) (cons 0 (dint-ad-go f 1)))
(define (dint-ad-go f i) (if (null? f) (quote ()) (cons (/ (car f) i) (dint-ad-go (cdr f) (+ i 1)))))

; ----- certify F' = f by the differentiation arbiter -----
(define (dint-certify f) (equal? (dint-trim (poly-deriv (dint-antideriv f))) (dint-trim f)))
(define (dint-trim p) (dint-trim-go p (dint-len p)))
(define (dint-len l) (if (null? l) 0 (+ 1 (dint-len (cdr l)))))
(define (dint-trim-go p k) (cond ((= k 0) (quote ())) ((= (dint-nth p (- k 1)) 0) (dint-trim-go p (- k 1))) (else (dint-take p k))))
(define (dint-nth l k) (if (= k 0) (car l) (dint-nth (cdr l) (- k 1))))
(define (dint-take l k) (if (= k 0) (quote ()) (cons (car l) (dint-take (cdr l) (- k 1)))))

; ----- the value F(b) - F(a) -----
(define (dint-value f a b) (- (poly-eval (dint-antideriv f) b) (poly-eval (dint-antideriv f) a)))

; ----- guarded evaluation -----
(define (dint-eval f a b) (if (dint-certify f) (list (quote ok) (dint-value f a b)) (list (quote uncertified) (quote antiderivative-check-failed))))

; ----- the full proof record -----
(define (dint-prove f a b)
  (list (quote theorem) (list (quote definite-integral) f a b) (quote =) (dint-value f a b)
        (list (quote by) (quote FTC) (list (quote antiderivative) (dint-antideriv f)) (list (quote certificate) (quote F-prime=f) (dint-certify f)))))

; ----- re-check a proof record: the certificate re-verifies and the value recomputes -----
(define (dint-recheck record) (dint-rc (car (cdr record)) (car (cdr (cdr (cdr record)))) record))
(define (dint-rc claim value record) (dint-rc-go (dint-claim-f claim) (dint-claim-a claim) (dint-claim-b claim) value))
(define (dint-claim-f claim) (car (cdr claim)))
(define (dint-claim-a claim) (car (cdr (cdr claim))))
(define (dint-claim-b claim) (car (cdr (cdr (cdr claim)))))
(define (dint-rc-go f a b value) (if (dint-certify f) (equal? (dint-value f a b) value) #f))
