; -*- lisp -*-
; String operations and string-based templating.

; Basic queries.
(display "length \"lizard\"      = ") (display (string-length "lizard")) (newline)
(display "string-append demo  = ")
(display (string-append "hello, " "world" "!"))                  (newline)
(display "substring 6..11     = ")
(display (substring "hello world" 6 11))                         (newline)
(display "string=? abc abc    = ") (display (string=? "abc" "abc")) (newline)
(display "string=? abc abd    = ") (display (string=? "abc" "abd")) (newline)
(newline)

; Conversions: numbers <-> strings.
(display "(number->string 42)         = ") (display (number->string 42)) (newline)
(display "(number->string (expt 2 80))= ")
(display (number->string (expt 2 80)))                                    (newline)
(display "(string->number \"123\")      = ") (display (string->number "123")) (newline)
(display "(string->number \"oops\")     = ") (display (string->number "oops")) (newline)
(newline)

; Symbols <-> strings.
(display "(symbol->string 'lizard) = ") (display (symbol->string 'lizard)) (newline)
(display "(string->symbol \"wiz\")   = ") (display (string->symbol "wiz")) (newline)
(newline)

; A formatted-output helper built on the string ops above.
(define (format-tagged tag value)
  (string-append "[" (symbol->string tag) "] " (number->string value)))

(display (format-tagged 'count 42))    (newline)
(display (format-tagged 'fib25 75025)) (newline)
(display (format-tagged 'fact10 3628800)) (newline)
(newline)

; A reverse-string built by recursive substring.
(define (string-reverse s)
  (define len (string-length s))
  (define (loop i acc)
    (if (< i 0) acc
        (loop (- i 1) (string-append acc (substring s i (+ i 1))))))
  (loop (- len 1) ""))

(display "reverse \"hello\" = ") (display (string-reverse "hello")) (newline)
(display "reverse \"lizard wizard\" = ") (display (string-reverse "lizard wizard")) (newline)
(newline)

; String escapes (now that the tokenizer handles them).
(display "tab between\there\n")
(display "quoted: \"with embedded quotes\"\n")
