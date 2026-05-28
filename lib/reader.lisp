; -*- lisp -*-
; lib/reader.lisp — A reader with #lang dialects (Track R → 100%).
;
; Completes the Phase 3 "reader -> expander" separation: a reader
; written in lizard that tokenizes source text, parses it into
; s-expressions, and supports `#lang <dialect>` directives that
; select a post-processing transform (a language dialect).

(import "match.lisp")

; ============================================================
;  TOKENIZER
; ============================================================
; Surround parens with spaces, then split on whitespace.

(define (pad-parens str)
  (apply-str-append
    (map (lambda (c)
           (if (equal? c "(") " ( "
             (if (equal? c ")") " ) " c)))
         (string->list str))))

(define (apply-str-append parts)
  (if (null? parts) ""
    (string-append (car parts) (apply-str-append (cdr parts)))))

(define (tokenize str)
  (filter (lambda (tok) (not (string=? tok "")))
          (string-split (pad-parens str) " ")))

; ============================================================
;  PARSER  (tokens -> s-expression)
; ============================================================
; Returns (cons form remaining-tokens).

(define (token->atom tok)
  (let ((n (string->number tok)))
    (if n n (string->symbol tok))))

(define (parse-tokens tokens)
  (if (null? tokens)
      (cons '() '())
    (let ((tok (car tokens)))
      (if (string=? tok "(")
          (parse-list (cdr tokens) '())
        (cons (token->atom tok) (cdr tokens))))))

(define (parse-list tokens acc)
  (if (null? tokens)
      (cons (reverse acc) '())             ; unbalanced — tolerate
    (if (string=? (car tokens) ")")
        (cons (reverse acc) (cdr tokens))
      (let ((r (parse-tokens tokens)))
        (parse-list (cdr r) (cons (car r) acc))))))

; Read a single s-expression from a string.
(define (read-str str)
  (car (parse-tokens (tokenize str))))

; Read all top-level forms from a string.
(define (read-all str)
  (read-forms (tokenize str)))

(define (read-forms tokens)
  (if (null? tokens) '()
    (let ((r (parse-tokens tokens)))
      (cons (car r) (read-forms (cdr r))))))

; ============================================================
;  #lang DIALECTS
; ============================================================
; A program may begin with  (#lang NAME).  The reader strips it and
; applies the dialect's transform to the remaining forms.
;
; A dialect is (list 'dialect name transform), where transform maps
; a list of forms to a list of forms.

(define (make-dialect name transform)
  (list 'dialect name transform))

(define (dialect-name d) (car (cdr d)))
(define (dialect-transform d) (car (cdr (cdr d))))

; A registry of dialects (alist name -> dialect).
(define (dialect-registry . ds)
  (map (lambda (d) (cons (dialect-name d) d)) ds))

(define (registry-lookup reg name)
  (let ((b (assoc name reg))) (if b (cdr b) #f)))

; Read a program, honoring a leading (#lang NAME) directive.
(define (read-program str registry)
  (let ((forms (read-all str)))
    (if (and (pair? forms)
             (pair? (car forms))
             (equal? (car (car forms)) (string->symbol "#lang")))
        (let ((name (car (cdr (car forms))))
              (body (cdr forms)))
          (let ((d (registry-lookup registry name)))
            (if d
                ((dialect-transform d) body)
                body)))
        forms)))

; ============================================================
;  SAMPLE DIALECTS
; ============================================================

; "identity" dialect: forms unchanged.
(define identity-dialect
  (make-dialect 'plain (lambda (forms) forms)))

; "infix" dialect: rewrite (a OP b) where OP is + - * / into prefix.
; (a + b) => (+ a b).  Recurses into sub-forms.
(define (infix->prefix form)
  (if (pair? form)
      (if (and (= (length form) 3)
               (member (car (cdr form)) '(+ - * /)))
          (list (car (cdr form))
                (infix->prefix (car form))
                (infix->prefix (car (cdr (cdr form)))))
          (map infix->prefix form))
      form))

(define infix-dialect
  (make-dialect 'infix
    (lambda (forms) (map infix->prefix forms))))

; "uppercase-keywords" dialect: leave as-is (placeholder for custom
; reader behavior). Shows the extension point.
(define quote-all-dialect
  (make-dialect 'quoted
    (lambda (forms) (map (lambda (f) (list 'quote f)) forms))))
