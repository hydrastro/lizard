; -*- lisp -*-
; lib/syntax-rules.lisp — A syntax-rules macro engine with ellipsis.
;
; This is the headline Track R feature: pattern-based macros with
; `...` (ellipsis) for matching and generating repeated forms.
;
;   pattern variables : symbols (bound to matched sub-forms)
;   literals          : symbols in the literal list (must match exactly)
;   _                 : wildcard
;   (p ...)           : ellipsis — p matches zero or more forms
;
; A macro is (rules literals (pattern template) ...). Expansion finds
; the first matching pattern and instantiates its template.

(import "match.lisp")    ; for take, drop

; ============================================================
;  PATTERN MATCHING (produces a bindings alist or 'fail)
; ============================================================
; Binding forms:
;   (var . value)                 normal pattern variable
;   (var ellipsis v1 v2 ...)      ellipsis variable (a sequence)

(define (ellipsis-tok? x) (equal? x '...))

(define (sr-match pat form lits)
  (sr-m pat form lits '()))

(define (sr-m pat form lits binds)
  (if (equal? binds 'fail) 'fail
    (if (symbol? pat)
        (if (member pat lits)
            (if (equal? pat form) binds 'fail)        ; literal
            (if (equal? pat '_) binds                  ; wildcard
                (cons (cons pat form) binds)))         ; pattern variable
      (if (null? pat)
          (if (null? form) binds 'fail)
        (if (pair? pat)
            (if (and (pair? (cdr pat)) (ellipsis-tok? (car (cdr pat))))
                (sr-match-ellipsis (car pat) (cdr (cdr pat)) form lits binds)
                (if (pair? form)
                    (sr-m (cdr pat) (cdr form) lits
                          (sr-m (car pat) (car form) lits binds))
                    'fail))
          (if (equal? pat form) binds 'fail))))))      ; literal datum

; Match (sub ... . rest) against form.
(define (sr-match-ellipsis sub rest form lits binds)
  (let ((n (- (length form) (pattern-min-length rest))))
    (if (< n 0) 'fail
      (let ((matched (take n form))
            (remaining (drop n form)))
        (let ((seqs (match-ellipsis-items sub matched lits)))
          (if (equal? seqs 'fail) 'fail
            (sr-m rest remaining lits
                  (merge-ellipsis-binds sub seqs binds))))))))

; Match `sub` against each item; return a list of binding-sets or 'fail.
(define (match-ellipsis-items sub items lits)
  (if (null? items) '()
    (let ((b (sr-match sub (car items) lits)))
      (if (equal? b 'fail) 'fail
        (let ((rest (match-ellipsis-items sub (cdr items) lits)))
          (if (equal? rest 'fail) 'fail
            (cons b rest)))))))

; For each pattern var in `sub`, collect its value across all
; binding-sets into one ellipsis binding.
(define (merge-ellipsis-binds sub binding-sets base)
  (append
    (map (lambda (v)
           (cons v (cons 'ellipsis
                         (map (lambda (bs) (cdr (assoc v bs))) binding-sets))))
         (pattern-vars sub))
    base))

; ---- pattern variable collection ----
(define (pattern-vars pat)
  (if (symbol? pat)
      (if (or (ellipsis-tok? pat) (equal? pat '_)) '() (list pat))
    (if (pair? pat)
        (sr-union (pattern-vars (car pat)) (pattern-vars (cdr pat)))
        '())))

(define (sr-union a b)
  (if (null? a) b
    (if (member (car a) b)
        (sr-union (cdr a) b)
        (cons (car a) (sr-union (cdr a) b)))))

; ---- minimum forms a pattern list consumes (ellipsis counts as 0) ----
(define (pattern-min-length pat)
  (if (pair? pat)
      (if (and (pair? (cdr pat)) (ellipsis-tok? (car (cdr pat))))
          (pattern-min-length (cdr (cdr pat)))
          (+ 1 (pattern-min-length (cdr pat))))
      0))

; ============================================================
;  TEMPLATE EXPANSION
; ============================================================

(define (sr-expand template binds)
  (if (symbol? template)
      (let ((b (assoc template binds)))
        (if b
            (if (ellipsis-binding? b) b (cdr b))   ; normal var → its value
            template))
    (if (pair? template)
        (if (and (pair? (cdr template)) (ellipsis-tok? (car (cdr template))))
            (append (expand-ellipsis (car template) binds)
                    (sr-expand (cdr (cdr template)) binds))
            (cons (sr-expand (car template) binds)
                  (sr-expand (cdr template) binds)))
        template)))

(define (ellipsis-binding? b)
  (and (pair? (cdr b)) (equal? (car (cdr b)) 'ellipsis)))

(define (ellipsis-values b) (cdr (cdr b)))

; Expand (sub ...) by iterating over the ellipsis variables in sub.
(define (expand-ellipsis sub binds)
  (let ((evars (filter (lambda (v)
                         (let ((b (assoc v binds)))
                           (and b (ellipsis-binding? b))))
                       (pattern-vars sub))))
    (if (null? evars) '()
      (let ((lists (map (lambda (v) (ellipsis-values (assoc v binds))) evars)))
        (expand-ell-iter sub evars lists (length (car lists)) 0 binds)))))

(define (expand-ell-iter sub evars lists n i binds)
  (if (>= i n) '()
    (cons (sr-expand sub (bind-ith evars lists i binds))
          (expand-ell-iter sub evars lists n (+ i 1) binds))))

; Bind each ellipsis var to its i-th value, on top of base binds.
(define (bind-ith evars lists i base)
  (if (null? evars) base
    (cons (cons (car evars) (nth i (car lists)))
          (bind-ith (cdr evars) (cdr lists) i base))))

(define (nth i lst)
  (if (= i 0) (car lst) (nth (- i 1) (cdr lst))))

; ============================================================
;  MACRO INTERFACE
; ============================================================
; A macro: (make-macro literals rules) where rules is a list of
; (pattern template).

(define (make-macro literals rules)
  (list 'macro literals rules))

(define (macro? x) (and (pair? x) (equal? (car x) 'macro)))
(define (macro-literals m) (car (cdr m)))
(define (macro-rules m) (car (cdr (cdr m))))

; Expand one use of a macro against a form.
(define (macro-apply m form)
  (try-rules (macro-rules m) (macro-literals m) form))

(define (try-rules rules lits form)
  (if (null? rules)
      (begin (display "  macro: no matching rule for ") (display form) (newline)
             form)
    (let ((rule (car rules)))
      (let ((pat (car rule))
            (tmpl (car (cdr rule))))
        (let ((binds (sr-match pat form lits)))
          (if (equal? binds 'fail)
              (try-rules (cdr rules) lits form)
              (sr-expand tmpl binds)))))))
