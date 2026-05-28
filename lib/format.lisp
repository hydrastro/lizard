; -*- lisp -*-
; lib/format.lisp — String formatting and table rendering.

; ---- build a string of n copies of a 1-char string ----
(define (repeat-str ch n)
  (if (<= n 0) ""
    (string-append ch (repeat-str ch (- n 1)))))

; ---- convert any common value to a display string ----
(define (->string x)
  (if (string? x) x
    (if (number? x) (number->string x)
      (if (symbol? x) (symbol->string x)
        (if (null? x) "()"
          (if (equal? x #t) "#t"
            (if (equal? x #f) "#f"
              "?")))))))

; ---- padding ----
(define (pad-left s width)
  (let ((str (->string s)))
    (let ((len (string-length str)))
      (if (>= len width) str
        (string-append (repeat-str " " (- width len)) str)))))

(define (pad-right s width)
  (let ((str (->string s)))
    (let ((len (string-length str)))
      (if (>= len width) str
        (string-append str (repeat-str " " (- width len)))))))

(define (pad-center s width)
  (let ((str (->string s)))
    (let ((len (string-length str)))
      (if (>= len width) str
        (let ((total (- width len)))
          (let ((left (quotient total 2)))
            (string-append (repeat-str " " left)
                           str
                           (repeat-str " " (- total left)))))))))

; ---- template substitution: replace {} with successive args ----
; (format-template "Hello {}, you are {}" (list "Bob" 30))
(define (format-template tmpl args)
  (let ((parts (string-split tmpl "{}")))
    (interleave-args parts args)))

(define (interleave-args parts args)
  (if (null? parts) ""
    (if (null? (cdr parts))
        (car parts)
        (string-append
          (car parts)
          (if (null? args) "" (->string (car args)))
          (interleave-args (cdr parts)
                           (if (null? args) '() (cdr args)))))))

; ---- table rendering ----
; rows is a list of lists (cells). Each column is padded to its max width.

(define (column-widths rows)
  (if (null? rows) '()
    (fold-left
      (lambda (widths row)
        (max-widths widths (map (lambda (c) (string-length (->string c))) row)))
      (map (lambda (c) (string-length (->string c))) (car rows))
      (cdr rows))))

(define (max-widths a b)
  (if (or (null? a) (null? b)) '()
    (cons (max (car a) (car b))
          (max-widths (cdr a) (cdr b)))))

(define (render-row row widths)
  (string-append
    "| "
    (string-join
      (render-cells row widths)
      " | ")
    " |"))

(define (render-cells row widths)
  (if (or (null? row) (null? widths)) '()
    (cons (pad-right (car row) (car widths))
          (render-cells (cdr row) (cdr widths)))))

(define (render-table rows)
  (let ((widths (column-widths rows)))
    (for-each (lambda (row)
                (display (render-row row widths))
                (newline))
              rows)))

; ---- number formatting ----
(define (format-int n width)
  (pad-left (number->string n) width))
