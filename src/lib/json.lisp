; -*- lisp -*-
; lib/json.lisp — Serialize lizard data to JSON-like strings.
;
; Mapping:
;   number      → 42
;   string      → "quoted"
;   symbol      → "quoted"
;   #t / #f     → true / false
;   ()          → []
;   list        → [a, b, c]
;   (obj ...)   → {"k": v, ...}   (tagged association list)
;
; Objects are represented as a list tagged with the symbol 'obj
; followed by (key . value) pairs:
;   (obj (name . "Bob") (age . 30))

(import "format.lisp")   ; for ->string

; ---- escape a string for JSON ----
(define (json-escape-str s)
  (string-append "\"" s "\""))

; ---- is this a tagged object? ----
(define (json-object? x)
  (and (pair? x) (equal? (car x) 'obj)))

; ---- main serializer ----
(define (to-json x)
  (if (number? x) (number->string x)
    (if (string? x) (json-escape-str x)
      (if (equal? x #t) "true"
        (if (equal? x #f) "false"
          (if (null? x) "[]"
            (if (json-object? x)
                (json-object-to-string (cdr x))
                (if (pair? x)
                    (json-array-to-string x)
                    (if (symbol? x)
                        (json-escape-str (symbol->string x))
                        "null")))))))))

; ---- serialize a list as a JSON array ----
(define (json-array-to-string lst)
  (string-append "[" (json-join-elements lst) "]"))

(define (json-join-elements lst)
  (if (null? lst) ""
    (if (null? (cdr lst))
        (to-json (car lst))
        (string-append (to-json (car lst)) ", "
                       (json-join-elements (cdr lst))))))

; ---- serialize an object (alist of (key . value)) ----
(define (json-object-to-string pairs)
  (string-append "{" (json-join-pairs pairs) "}"))

(define (json-join-pairs pairs)
  (if (null? pairs) ""
    (let ((pair (car pairs)))
      (let ((entry (string-append
                     (json-escape-str (->string (car pair)))
                     ": "
                     (to-json (cdr pair)))))
        (if (null? (cdr pairs))
            entry
            (string-append entry ", " (json-join-pairs (cdr pairs))))))))

; ---- convenience constructors ----
(define (json-object . pairs)
  (cons 'obj pairs))

(define (json-field key val)
  (cons key val))

; ---- pretty printer (2-space indent, one field per line) ----
(define (to-json-pretty x)
  (json-pretty x 0))

(define (json-indent n)
  (repeat-str " " (* n 2)))

(define (json-pretty x depth)
  (if (json-object? x)
      (json-pretty-object (cdr x) depth)
      (if (and (pair? x) (not (json-object? x)))
          (json-pretty-array x depth)
          (to-json x))))

(define (json-pretty-object pairs depth)
  (if (null? pairs) "{}"
    (string-append
      "{\n"
      (json-pretty-pairs pairs (+ depth 1))
      "\n" (json-indent depth) "}")))

(define (json-pretty-pairs pairs depth)
  (if (null? pairs) ""
    (let ((pair (car pairs)))
      (let ((line (string-append
                    (json-indent depth)
                    (json-escape-str (->string (car pair)))
                    ": "
                    (json-pretty (cdr pair) depth))))
        (if (null? (cdr pairs))
            line
            (string-append line ",\n" (json-pretty-pairs (cdr pairs) depth)))))))

(define (json-pretty-array lst depth)
  (string-append "[" (json-join-elements lst) "]"))
