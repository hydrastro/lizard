; -*- lisp -*-
; lib/record.lisp — A record/struct system built on association lists.
;
; A record is a tagged alist:
;   (record (field1 . value1) (field2 . value2) ...)
;
; This gives named-field data with functional (immutable) update.

; ---- construction ----

; (make-record (list (cons 'name "Bob") (cons 'age 30)))
(define (make-record fields)
  (cons 'record fields))

; (record 'name "Bob" 'age 30) — variadic constructor
(define (record . kvs)
  (make-record (pairs-from-list kvs)))

(define (pairs-from-list kvs)
  (if (null? kvs) '()
    (if (null? (cdr kvs)) '()
      (cons (cons (car kvs) (car (cdr kvs)))
            (pairs-from-list (cdr (cdr kvs)))))))

; ---- predicate ----
(define (record? x)
  (and (pair? x) (equal? (car x) 'record)))

; ---- accessors ----
(define (record-fields rec)
  (cdr rec))

(define (record-get rec field)
  (let ((pair (assoc field (cdr rec))))
    (if pair (cdr pair) #f)))

(define (record-has? rec field)
  (if (assoc field (cdr rec)) #t #f))

(define (record-keys rec)
  (map car (cdr rec)))

(define (record-values rec)
  (map cdr (cdr rec)))

; ---- functional update (returns a new record) ----
(define (record-set rec field val)
  (make-record
    (cons (cons field val)
          (filter (lambda (p) (not (equal? (car p) field)))
                  (cdr rec)))))

(define (record-update rec field f)
  (record-set rec field (f (record-get rec field))))

(define (record-remove rec field)
  (make-record
    (filter (lambda (p) (not (equal? (car p) field)))
            (cdr rec))))

; ---- merge two records (right wins on conflicts) ----
(define (record-merge a b)
  (fold-left (lambda (acc pair)
               (record-set acc (car pair) (cdr pair)))
             a
             (cdr b)))

; ---- map over values ----
(define (record-map-values rec f)
  (make-record
    (map (lambda (pair) (cons (car pair) (f (cdr pair))))
         (cdr rec))))

; ---- count fields ----
(define (record-size rec)
  (length (cdr rec)))

; ---- define-record-type style: generate constructor + accessors ----
; Returns a list (constructor-fn accessor-fns...) — usage shown in
; examples. Because lizard is eager, this returns helper functions
; rather than installing global names.

(define (record-type field-names)
  ; constructor: takes values in field order
  (cons
    (lambda (values)
      (make-record (zip-fields field-names values)))
    ; accessor for a named field
    (lambda (field) (lambda (rec) (record-get rec field)))))

(define (zip-fields names values)
  (if (or (null? names) (null? values)) '()
    (cons (cons (car names) (car values))
          (zip-fields (cdr names) (cdr values)))))
