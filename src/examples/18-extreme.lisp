; -*- lisp -*-
; The full advanced feature surface in one program:
;   - varargs lambdas
;   - vectors with O(1) indexed access
;   - hash tables with O(1) keyed access
;   - exception handling via (try ...)/(raise ...)
;   - gensym for hygienic-ish macros
;   - reflection (type-of)

; ---- Varargs: a `printf`-style formatter ---------------------------------
;
; (puts "x=" 42 " y=" 99) -> "x=42 y=99\n"

(define puts
  (lambda parts
    (define (loop xs)
      (cond ((null? xs) '())
            (else
              (cond ((= (type-of (car xs)) 'number)
                     (display (number->string (car xs))))
                    ((= (type-of (car xs)) 'symbol)
                     (display (symbol->string (car xs))))
                    (else (display (car xs))))
              (loop (cdr xs)))))
    (loop parts)
    (newline)))

(puts "hello " 'world ", " 2 "025")
(puts "type-of 42 is: " (type-of 42))
(newline)

; ---- Vectors: an in-place insertion sort --------------------------------

(define (insertion-sort! v)
  (define n (vector-length v))
  (define (insert i)
    (define key (vector-ref v i))
    (define (shift j)
      (cond ((< j 0) (vector-set! v 0 key))
            ((> (vector-ref v j) key)
             (vector-set! v (+ j 1) (vector-ref v j))
             (shift (- j 1)))
            (else (vector-set! v (+ j 1) key))))
    (shift (- i 1)))
  (define (loop i)
    (if (>= i n) '()
        (begin (insert i) (loop (+ i 1)))))
  (loop 1)
  '())

(define v (list->vector '(5 2 8 1 9 3 7 4 6)))
(display "before sort: ") (display v) (newline)
(insertion-sort! v)
(display "after sort:  ") (display v) (newline)
(newline)

; ---- Hash tables: word-frequency counter --------------------------------

(define (count-words! tbl words)
  (define (loop ws)
    (cond ((null? ws) '())
          (else
            (define w (car ws))
            (hash-set! tbl w (+ 1 (hash-ref tbl w 0)))
            (loop (cdr ws)))))
  (loop words)
  '())

(define freq (make-hash-table))
(count-words! freq '(the quick brown fox jumps over the lazy dog
                     the the lazy fox))

(display "word frequencies: ") (display freq) (newline)
(puts "  'the' appeared "  (hash-ref freq 'the)  " times")
(puts "  'fox' appeared "  (hash-ref freq 'fox)  " times")
(puts "  'lazy' appeared " (hash-ref freq 'lazy) " times")
(newline)

; ---- Exceptions: safe division with custom error handling ---------------

(define (safe-div a b)
  (try (lambda () (/ a b))
       (lambda (err)
         (puts "  caught error, falling back to 0")
         0)))

(puts "safe-div 10 2 = " (safe-div 10 2))
(puts "safe-div  7 0 = " (safe-div 7 0))
(puts "safe-div 99 3 = " (safe-div 99 3))
(newline)

; ---- User-raised errors with structured payloads ------------------------

(define (validate-age n)
  (cond ((< n 0)  (raise (list 'invalid-age n "must be non-negative")))
        ((> n 150) (raise (list 'invalid-age n "too old to be plausible")))
        (else n)))

(define (handle-age n)
  (try (lambda () (puts "valid age: " (validate-age n)))
       (lambda (err)
         (define payload (error-value err))
         (cond ((and (pair? payload) (= (car payload) 'invalid-age))
                (puts "rejected age " (car (cdr payload)) ": "
                      (car (cdr (cdr payload)))))
               (else (puts "unknown error"))))))

(handle-age 25)
(handle-age -5)
(handle-age 200)
(handle-age 99)
(newline)

; ---- gensym: hand-written hygiene ---------------------------------------
;
; A swap! "macro" emulated as a higher-order function. We use gensym
; to produce a temp name that can't collide with the caller's bindings.

(define tmp (gensym "swap-tmp-"))
(puts "fresh gensym: " (symbol->string tmp))
(define tmp2 (gensym "swap-tmp-"))
(puts "another:      " (symbol->string tmp2))
(puts "distinct?     " (not (= tmp tmp2)))
