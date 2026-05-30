; -*- lisp -*-
; lib/parser.lisp — Parser combinators (Evolution Plan Phase 3).
;
; A parser is a function: input-list -> result.
; The result is either:
;   (ok value rest)    a 3-element list: tag 'ok, value, remaining input
;   (fail)             a 1-element list: tag 'fail
;
; Parsers consume from the front of a list (of tokens or chars).

; ---- result constructors / predicates ----
(define (p-ok value rest) (list 'ok value rest))
(define (p-failure) (list 'fail))
(define (p-ok? r) (equal? (car r) 'ok))
(define (p-value r) (car (cdr r)))
(define (p-rest r) (car (cdr (cdr r))))

; ---- run a parser on an input list ----
(define (parse p input) (p input))

; ============================================================
;  PRIMITIVE PARSERS
; ============================================================

; Match any single item (fails on empty input).
(define (p-any)
  (lambda (input)
    (if (null? input) (p-failure)
      (p-ok (car input) (cdr input)))))

; Match an item equal to x.
(define (p-lit x)
  (lambda (input)
    (if (null? input) (p-failure)
      (if (equal? (car input) x)
          (p-ok (car input) (cdr input))
          (p-failure)))))

; Match an item satisfying pred.
(define (p-sat pred)
  (lambda (input)
    (if (null? input) (p-failure)
      (if (pred (car input))
          (p-ok (car input) (cdr input))
          (p-failure)))))

; Always succeed with a value, consuming nothing.
(define (p-return value)
  (lambda (input) (p-ok value input)))

; Always fail.
(define (p-fail-always)
  (lambda (input) (p-failure)))

; ============================================================
;  COMBINATORS
; ============================================================

; Transform the result of a parser.
(define (p-map f p)
  (lambda (input)
    (let ((r (p input)))
      (if (p-ok? r)
          (p-ok (f (p-value r)) (p-rest r))
          r))))

; Sequence two parsers, combining results with f.
(define (p-then2 f p1 p2)
  (lambda (input)
    (let ((r1 (p1 input)))
      (if (p-ok? r1)
          (let ((r2 (p2 (p-rest r1))))
            (if (p-ok? r2)
                (p-ok (f (p-value r1) (p-value r2)) (p-rest r2))
                (p-failure)))
          (p-failure)))))

; Sequence two parsers, keeping both values as a pair.
(define (p-seq p1 p2)
  (p-then2 (lambda (a b) (cons a b)) p1 p2))

; Sequence, keep only the left result.
(define (p-left p1 p2)
  (p-then2 (lambda (a b) a) p1 p2))

; Sequence, keep only the right result.
(define (p-right p1 p2)
  (p-then2 (lambda (a b) b) p1 p2))

; Try p1; if it fails, try p2.
(define (p-or p1 p2)
  (lambda (input)
    (let ((r1 (p1 input)))
      (if (p-ok? r1) r1 (p2 input)))))

; Zero or more repetitions; collects results into a list.
(define (p-many p)
  (lambda (input)
    (define (go acc inp)
      (let ((r (p inp)))
        (if (p-ok? r)
            (go (cons (p-value r) acc) (p-rest r))
            (p-ok (reverse acc) inp))))
    (go '() input)))

; One or more repetitions.
(define (p-many1 p)
  (p-then2 (lambda (first rest) (cons first rest))
           p
           (p-many p)))

; ============================================================
;  CONVENIENCE
; ============================================================

; Parse and return just the value, or #f on failure / leftover input.
(define (parse-all p input)
  (let ((r (p input)))
    (if (and (p-ok? r) (null? (p-rest r)))
        (p-value r)
        #f)))

; A digit parser (over numeric tokens).
(define (p-digit)
  (p-sat number?))

; Parse a sequence of digits into a list.
(define (p-number)
  (p-many1 (p-digit)))
