; -*- lisp -*-
; lib/regex.lisp — A regular expression engine in lizard.
;
; Patterns are s-expressions (an AST):
;   (lit "a")        literal character
;   any              any single character
;   (seq p ...)      concatenation
;   (alt p1 p2)      alternation (p1 | p2)
;   (star p)         zero or more  (p*)
;   (plus p)         one or more   (p+)
;   (opt p)          optional      (p?)
;   (set "abc")      character class — match any one of the chars
;
; Matching uses continuation-passing backtracking: re-m tries to
; match p at the front of `chars`, then calls k on what remains.
; k returns #t/#f; the whole match succeeds if some path lets k
; succeed at end-of-input.

; ============================================================
;  CORE MATCHER
; ============================================================

(define (re-m p chars k)
  (if (equal? p 'any)
      (if (null? chars) #f (k (cdr chars)))
    (if (pair? p)
        (re-m-compound p chars k)
        ; bare atom: treat as literal symbol/char — rarely used
        (if (and (not (null? chars)) (equal? (car chars) p))
            (k (cdr chars))
            #f))))

(define (re-m-compound p chars k)
  (let ((tag (car p)))
    (if (equal? tag 'lit)
        (if (and (not (null? chars)) (equal? (car chars) (car (cdr p))))
            (k (cdr chars))
            #f)
      (if (equal? tag 'seq)
          (re-m-seq (cdr p) chars k)
        (if (equal? tag 'alt)
            (if (re-m (car (cdr p)) chars k)
                #t
                (re-m (car (cdr (cdr p))) chars k))
          (if (equal? tag 'star)
              (re-m-star (car (cdr p)) chars k)
            (if (equal? tag 'plus)
                (re-m (car (cdr p)) chars
                      (lambda (rest) (re-m-star (car (cdr p)) rest k)))
              (if (equal? tag 'opt)
                  (if (re-m (car (cdr p)) chars k) #t (k chars))
                (if (equal? tag 'set)
                    (re-m-set (car (cdr p)) chars k)
                  #f)))))))))

; Concatenation: match each sub-pattern in order.
(define (re-m-seq ps chars k)
  (if (null? ps)
      (k chars)
      (re-m (car ps) chars
            (lambda (rest) (re-m-seq (cdr ps) rest k)))))

; Kleene star: greedily try to match more, backtracking as needed.
(define (re-m-star p chars k)
  (if (re-m p chars
           (lambda (rest)
             (if (equal? rest chars)
                 #f                       ; no progress — avoid infinite loop
                 (re-m-star p rest k))))
      #t
      (k chars)))                          ; zero repetitions

; Character set: match if the current char is in the set string.
(define (re-m-set setstr chars k)
  (if (null? chars) #f
    (if (char-in-set? (car chars) setstr)
        (k (cdr chars))
        #f)))

(define (char-in-set? ch setstr)
  (string-contains? setstr ch))

; ============================================================
;  TOP-LEVEL
; ============================================================

; Full match: does the pattern match the ENTIRE string?
(define (regex-match pattern str)
  (re-m pattern (string->list str)
        (lambda (rest) (null? rest))))

; Search: does the pattern match somewhere (any prefix of some suffix)?
(define (regex-search pattern str)
  (define (try chars)
    (if (re-m pattern chars (lambda (rest) #t))
        #t
        (if (null? chars) #f (try (cdr chars)))))
  (try (string->list str)))

; ============================================================
;  CONVENIENCE CONSTRUCTORS
; ============================================================

(define (rx-lit s) (list 'lit s))
(define (rx-seq . ps) (cons 'seq ps))
(define (rx-alt a b) (list 'alt a b))
(define (rx-star p) (list 'star p))
(define (rx-plus p) (list 'plus p))
(define (rx-opt p) (list 'opt p))
(define (rx-set s) (list 'set s))

; Build a (seq (lit ...) ...) from a literal string.
(define (rx-string str)
  (cons 'seq (map (lambda (c) (list 'lit c)) (string->list str))))
