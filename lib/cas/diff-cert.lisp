; -*- lisp -*-
; lib/cas/diff-cert.lisp — a proof-CARRYING differentiator.
;
; The companion lib/cas-proof.lisp attaches an INFORMAL justification (a chain
; of named rules bottoming out at ZFC) to each step.  docs/CAS.md names the next
; move: "state each [rule] as a kernel proposition ... a checked proof the kernel
; accepts."  That is what this module does — a derivative now emits a CERTIFICATE
; the trusted kernel type-checks.
;
; Design — a typed derivative JUDGMENT, not equational rewriting.  We work over
; an abstract commutative ring R and postulate one relation
;
;     Der : (R -> R) -> (R -> R) -> Sort 0          "g is the derivative of f"
;
; The differentiation rules are postulated CONSTRUCTORS of that judgment:
;
;     der_id    :                      Der (\x. x)        (\x. 1)
;     der_const : (c : R) ->           Der (\x. c)        (\x. 0)
;     der_add   : Der f f' -> Der g g' -> Der (\x. f x + g x) (\x. f' x + g' x)
;     der_mul   : Der f f' -> Der g g' -> Der (\x. f x * g x)
;                                            (\x. f' x * g x + f x * g' x)
;
; These four axioms ARE the "cited rules" of CAS.md, now stated as kernel
; propositions.  Elementary functions (sin, cos, exp, ln) add one base rule
; each (e.g. der_sin : Der sin cos), and the chain rule der_comp composes
; derivatives — together these differentiate the usual elementary fragment.
; A derivative's certificate is simply a nested application of them, and the
; kernel checks it: beta-reduction lines the dependent types up, so no fragile
; sym/trans/cong equational plumbing is needed.  Because the proof term must
; literally inhabit  Der (\x.e) (\x.e'),  a wrong derivative cannot be
; certified — the kernel rejects it (see examples/139-cas-certificates.lisp).
;
; Note: kernel-assume is GLOBAL kernel state, so importing this module postulates
; the ring + the four rules into the one shared proof context (the Lisp helpers
; below are ordinary module bindings and are namespaced as usual).

; ---- kernel term constructors (build terms as data; never hand-count parens) --
(define (kpi v ty body) (list 'Pi (list v ty) body))
(define (klam v ty body) (list 'lam (list v ty) body))
(define (kapp f x) (list 'app f x))
(define (k2 f x y) (kapp (kapp f x) y))
(define RR (kpi 'x 'R 'R))                ; the function type  R -> R
(define (Der f g) (k2 'Der f g))          ; the judgment        Der f g
(define (fn body) (klam 'x 'R body))      ; a function          \x. body

; The telescope every binary rule shares, written exactly once:
;   Pi f f1 g g1, Der f f1 -> Der g g1 -> Der resF resG
(define (der-rule-type resF resG)
  (kpi 'f RR (kpi 'f1 RR (kpi 'g RR (kpi 'g1 RR
    (kpi 'pf (Der 'f 'f1) (kpi 'pg (Der 'g 'g1) (Der resF resG))))))))

; ---- the signature: a commutative ring carrying the derivative judgment -------
; (names dodge reserved kernel symbols: I is the cubical interval; zero/succ are
;  the Nat constructors; hence zeroR / oneR for the ring's 0 and 1.)
(kernel-assume 'R '(Sort 0))
(kernel-assume 'zeroR 'R)
(kernel-assume 'oneR 'R)
(kernel-assume 'a 'R)                      ; a generic constant, for examples
(kernel-assume 'add (kpi 'u 'R (kpi 'v 'R 'R)))
(kernel-assume 'mul (kpi 'u 'R (kpi 'v 'R 'R)))
(kernel-assume 'Der (kpi 'f RR (kpi 'g RR '(Sort 0))))
(kernel-assume 'der_id    (Der (fn 'x) (fn 'oneR)))
(kernel-assume 'der_const (kpi 'c 'R (Der (fn 'c) (fn 'zeroR))))
(kernel-assume 'der_add
  (der-rule-type (fn (k2 'add (kapp 'f 'x) (kapp 'g 'x)))
                 (fn (k2 'add (kapp 'f1 'x) (kapp 'g1 'x)))))
(kernel-assume 'der_mul
  (der-rule-type (fn (k2 'mul (kapp 'f 'x) (kapp 'g 'x)))
                 (fn (k2 'add (k2 'mul (kapp 'f1 'x) (kapp 'g 'x))
                              (k2 'mul (kapp 'f 'x) (kapp 'g1 'x))))))

; Elementary functions and their derivative rules.  Each base rule is just a
; derivative judgment between two named functions; the chain rule der_comp
; composes derivatives:
;     der_comp : Der f f' -> Der g g' -> Der (\x. f (g x)) (\x. f'(g x) * g' x)
(kernel-assume 'sin RR)
(kernel-assume 'cos RR)
(kernel-assume 'exp RR)
(kernel-assume 'ln RR)
(kernel-assume 'neg RR)
(kernel-assume 'recip RR)
(kernel-assume 'der_sin (Der 'sin 'cos))
(kernel-assume 'der_cos (Der 'cos (fn (kapp 'neg (kapp 'sin 'x)))))
(kernel-assume 'der_exp (Der 'exp 'exp))
(kernel-assume 'der_ln  (Der 'ln 'recip))
(kernel-assume 'der_comp
  (der-rule-type (fn (kapp 'f (kapp 'g 'x)))
                 (fn (k2 'mul (kapp 'f1 (kapp 'g 'x)) (kapp 'g1 'x)))))

; Linearity through negation: d/dx (- g x) = - g' x.  A unary rule (neg is the
; ring's additive inverse), letting higher derivatives of sin and cos close up
; (cos' = -sin introduces neg, so the second derivative needs this).
;     der_neg_lin : Der g g' -> Der (\x. neg (g x)) (\x. neg (g' x))
(kernel-assume 'der_neg_lin
  (kpi 'g RR (kpi 'g1 RR (kpi 'pg (Der 'g 'g1)
    (Der (fn (kapp 'neg (kapp 'g 'x))) (fn (kapp 'neg (kapp 'g1 'x))))))))

; ---- the differentiator: emits (list derivative-term kernel-proof) ------------
(define (mentions-x? e)
  (cond ((symbol? e) (equal? e 'x))
        ((pair? e) (or (mentions-x? (car e)) (mentions-x? (cdr e))))
        (else #f)))
(define (k6 op f f1 g g1) (kapp (kapp (kapp (kapp op f) f1) g) g1))

; per-elementary-function tables for the chain rule:
;   frule    : the base derivative rule  (der_sin, ...)
;   ffn      : the outer derivative FUNCTION f'  (for der_comp's f' argument)
;   fapplied : f' applied to arg, already beta-reduced (for a readable result)
(define (frule s)
  (cond ((equal? s 'sin) 'der_sin) ((equal? s 'cos) 'der_cos)
        ((equal? s 'exp) 'der_exp) ((equal? s 'ln) 'der_ln) (else 'der_id)))
(define (ffn s)
  (cond ((equal? s 'sin) 'cos)
        ((equal? s 'cos) (fn (kapp 'neg (kapp 'sin 'x))))
        ((equal? s 'exp) 'exp)
        ((equal? s 'ln) 'recip)
        (else (fn 'x))))
(define (fapplied s arg)
  (cond ((equal? s 'sin) (kapp 'cos arg))
        ((equal? s 'cos) (kapp 'neg (kapp 'sin arg)))
        ((equal? s 'exp) (kapp 'exp arg))
        ((equal? s 'ln) (kapp 'recip arg))
        (else arg)))

; diff e  ->  (list e' proof)   where   proof : Der (\x. e) (\x. e')
;   e is a ring term in the bound variable x (built with add/mul over x and
;   constants).  Recursion mirrors the four rules exactly.
(define (diff e)
  (cond
    ((equal? e 'x)            (list 'oneR 'der_id))            ; d/dx x = 1
    ((not (mentions-x? e))    (list 'zeroR (kapp 'der_const e))) ; d/dx c = 0
    (else                                  ; e = (app HEAD tail)
     (let ((head (car (cdr e))))           ; HEAD: a fn symbol (unary) or (app OP a)
       (if (symbol? head)
           ; unary application f(arg): chain rule  (f∘g)' = f'(g)·g'
           (let ((arg (car (cdr (cdr e)))))
             (let ((d (diff arg)))
               (let ((a1 (car d)) (pa (car (cdr d))))
                 (if (equal? head 'neg)
                     ; linearity:  (neg (g x))' = neg (g' x)
                     (list (kapp 'neg a1)
                           (kapp (kapp (kapp 'der_neg_lin (fn arg)) (fn a1)) pa))
                     ; chain rule:  (f (g x))' = f'(g x) * g' x
                     (list (k2 'mul (fapplied head arg) a1)
                           (kapp (kapp (k6 'der_comp head (ffn head)
                                            (fn arg) (fn a1))
                                       (frule head)) pa))))))
           ; binary application (a OP b), OP in {add, mul}
           (let ((op (car (cdr head)))
                 (ea (car (cdr (cdr head))))
                 (eb (car (cdr (cdr e)))))
             (let ((da (diff ea)) (db (diff eb)))
               (let ((a1 (car da)) (pa (car (cdr da)))
                     (b1 (car db)) (pb (car (cdr db))))
                 (let ((fa (fn ea)) (fa1 (fn a1)) (fb (fn eb)) (fb1 (fn b1)))
                   (if (equal? op 'add)
                       ; sum rule:   (a+b)' = a' + b'
                       (list (k2 'add a1 b1)
                             (kapp (kapp (k6 'der_add fa fa1 fb fb1) pa) pb))
                       ; product rule: (a*b)' = a'*b + a*b'
                       (list (k2 'add (k2 'mul a1 eb) (k2 'mul ea b1))
                             (kapp (kapp (k6 'der_mul fa fa1 fb fb1) pa) pb))))))))))))

; certify e -> #t iff the kernel accepts the emitted proof as inhabiting
;               Der (\x. e) (\x. e').
(define (certify e)
  (let ((d (diff e)))
    (kernel-check (car (cdr d)) (Der (fn e) (fn (car d))))))

; The derivative term alone (the e' the differentiator produced).
(define (derivative e) (car (diff e)))

; ---- a friendly surface: (* x x), (+ a b), x, constants -> ring term ----------
(define (cas e)
  (cond ((equal? e 'x) 'x)
        ((symbol? e) e)                               ; a constant: zeroR, oneR, a
        ((equal? (car e) '+)
         (k2 'add (cas (car (cdr e))) (cas (car (cdr (cdr e))))))
        ((equal? (car e) '*)
         (k2 'mul (cas (car (cdr e))) (cas (car (cdr (cdr e))))))
        ((equal? (car e) 'sin) (kapp 'sin (cas (car (cdr e)))))
        ((equal? (car e) 'cos) (kapp 'cos (cas (car (cdr e)))))
        ((equal? (car e) 'exp) (kapp 'exp (cas (car (cdr e)))))
        ((equal? (car e) 'ln)  (kapp 'ln  (cas (car (cdr e)))))
        (else e)))                                    ; already a kernel term
