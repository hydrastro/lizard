; -*- lisp -*-
; ============================================================
;  EXAMPLE 97 — Formatting, JSON, and records
; ============================================================

(import "match.lisp")
(import "format.lisp")
(import "json.lisp")
(import "record.lisp")

(display "=== STRING FORMATTING ===") (newline)
(display "  pad-left 'hi' to 8:   [") (display (pad-left "hi" 8)) (display "]") (newline)
(display "  pad-right 'hi' to 8:  [") (display (pad-right "hi" 8)) (display "]") (newline)
(display "  pad-center 'hi' to 8: [") (display (pad-center "hi" 8)) (display "]") (newline)
(newline)

(display "  template: ")
(display (format-template "Hello {}, you are {} years old" (list "Bob" 30)))
(newline)
(newline)

(display "=== TABLE RENDERING ===") (newline)
(render-table
  (list
    (list "Name" "Age" "City")
    (list "Alice" 30 "Milano")
    (list "Bob" 25 "Roma")
    (list "Charlie" 35 "Napoli")))
(newline)

(display "=== RECORDS ===") (newline)
(define person (record 'name "Alice" 'age 30 'city "Milano"))
(display "  name: ") (display (record-get person 'name)) (newline)
(display "  age:  ") (display (record-get person 'age)) (newline)
(display "  keys: ") (display (record-keys person)) (newline)

(define older (record-update person 'age (lambda (a) (+ a 1))))
(display "  after birthday, age: ") (display (record-get older 'age)) (newline)
(display "  original age unchanged: ") (display (record-get person 'age)) (newline)

(define with-email (record-set person 'email "alice@example.com"))
(display "  added email: ") (display (record-get with-email 'email)) (newline)
(display "  size: ") (display (record-size with-email)) (newline)
(newline)

(display "=== JSON SERIALIZATION ===") (newline)
(display "  number: ") (display (to-json 42)) (newline)
(display "  string: ") (display (to-json "hello")) (newline)
(display "  bool:   ") (display (to-json #t)) (newline)
(display "  array:  ") (display (to-json (list 1 2 3))) (newline)
(display "  nested: ") (display (to-json (list 1 (list 2 3) 4))) (newline)
(newline)

(display "  object:") (newline)
(define obj (json-object (json-field 'name "Alice")
                         (json-field 'age 30)
                         (json-field 'active #t)))
(display "    compact: ") (display (to-json obj)) (newline)
(display "    pretty:") (newline)
(display (to-json-pretty obj)) (newline)
(newline)

(display "=== RECORD → JSON pipeline ===") (newline)
; Convert a record to a JSON object
(define (record->json rec)
  (cons 'obj (record-fields rec)))
(display "  person as JSON: ")
(display (to-json (record->json person))) (newline)
(newline)

(display "=== nested JSON document ===") (newline)
(define doc
  (json-object
    (json-field 'title "lizard")
    (json-field 'version 5)
    (json-field 'tracks (list "R" "C" "K" "Q"))
    (json-field 'meta (json-object
                        (json-field 'lines 29000)
                        (json-field 'tested #t)))))
(display (to-json-pretty doc)) (newline)
(newline)

(display "=== End ===") (newline)
