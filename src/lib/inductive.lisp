; -*- lisp -*-
; lib/inductive.lisp — Inductive families (Track K).
;
; Demonstrates DEPENDENT TYPES at the library level: types indexed
; by values. The index (a length, a bound) constrains which
; operations are valid and what their result types are.
;
;   Vec A n   — vectors of length n   (length tracked in the type)
;   Fin n     — the finite type {0,...,n-1}  (bounded indices)
;
; The classic payoff: vnth : Vec A n -> Fin n -> A is TOTAL —
; indexing can never go out of bounds, because Fin n only contains
; valid indices.

; ============================================================
;  Vec A n  — length-indexed vectors
; ============================================================
; Representation: (vec n elements). The invariant n = length elements
; is maintained by every constructor.

(define (vnil) (list 'vec 0 '()))

(define (vcons x v)
  (list 'vec (+ 1 (vec-len v)) (cons x (vec-elems v))))

(define (vec? x) (and (pair? x) (equal? (car x) 'vec)))
(define (vec-len v) (car (cdr v)))
(define (vec-elems v) (car (cdr (cdr v))))

; Build a Vec from a plain list (length inferred).
(define (list->vec lst)
  (list 'vec (length lst) lst))

(define (vec->list v) (vec-elems v))

; head : Vec A (n+1) -> A   (only valid on non-empty vectors)
(define (vhead v)
  (if (= (vec-len v) 0)
      (begin (display "  TYPE ERROR: vhead of empty Vec") (newline) 'error)
      (car (vec-elems v))))

; tail : Vec A (n+1) -> Vec A n
(define (vtail v)
  (if (= (vec-len v) 0)
      (begin (display "  TYPE ERROR: vtail of empty Vec") (newline) (vnil))
      (list 'vec (- (vec-len v) 1) (cdr (vec-elems v)))))

; append : Vec A m -> Vec A n -> Vec A (m+n)
(define (vappend a b)
  (list 'vec (+ (vec-len a) (vec-len b))
        (append (vec-elems a) (vec-elems b))))

; map : (A -> B) -> Vec A n -> Vec B n   (length preserved)
(define (vmap f v)
  (list 'vec (vec-len v) (map f (vec-elems v))))

; zip : Vec A n -> Vec B n -> Vec (A,B) n   (requires EQUAL lengths)
(define (vzip a b)
  (if (not (= (vec-len a) (vec-len b)))
      (begin (display "  TYPE ERROR: vzip length mismatch") (newline) (vnil))
      (list 'vec (vec-len a)
            (zip-lists (vec-elems a) (vec-elems b)))))

(define (zip-lists a b)
  (if (or (null? a) (null? b)) '()
    (cons (list (car a) (car b))
          (zip-lists (cdr a) (cdr b)))))

; ============================================================
;  Fin n  — the finite type with exactly n inhabitants
; ============================================================
; Representation: (fin bound index) with 0 <= index < bound.
; Constructors: fzero : Fin (n+1);  fsuc : Fin n -> Fin (n+1)

(define (fzero bound)
  (if (<= bound 0)
      (begin (display "  TYPE ERROR: Fin 0 is uninhabited") (newline) 'error)
      (list 'fin bound 0)))

(define (fsuc f)
  (list 'fin (+ 1 (fin-bound f)) (+ 1 (fin-index f))))

(define (fin? x) (and (pair? x) (equal? (car x) 'fin)))
(define (fin-bound f) (car (cdr f)))
(define (fin-index f) (car (cdr (cdr f))))

; Build a Fin n from an integer (checks the bound).
(define (make-fin bound i)
  (if (and (>= i 0) (< i bound))
      (list 'fin bound i)
      (begin (display "  TYPE ERROR: ") (display i)
             (display " not in Fin ") (display bound) (newline)
             'error)))

; ============================================================
;  THE PAYOFF: total, safe indexing
; ============================================================
; nth : Vec A n -> Fin n -> A
; Because the Fin's bound matches the Vec's length, this is total.

(define (vnth v f)
  (if (not (= (vec-len v) (fin-bound f)))
      (begin (display "  TYPE ERROR: Fin bound /= Vec length") (newline) 'error)
      (list-nth (vec-elems v) (fin-index f))))

(define (list-nth lst i)
  (if (= i 0) (car lst) (list-nth (cdr lst) (- i 1))))

; ============================================================
;  DEPENDENT ELIMINATOR (induction on Vec)
; ============================================================
; vec-rec : base (for vnil) + step (for vcons) -> result
; Folds a vector while respecting its inductive structure.

(define (vec-rec base step v)
  (if (= (vec-len v) 0)
      base
      (step (vhead v) (vec-rec base step (vtail v)))))

; Example uses of the eliminator:
(define (vsum v) (vec-rec 0 (lambda (x acc) (+ x acc)) v))
(define (vall-len v) (vec-rec 0 (lambda (x acc) (+ 1 acc)) v))
