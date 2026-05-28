; -*- lisp -*-
; ============================================================
;  EXAMPLE 107 — A logic engine (Prolog in lizard)
; ============================================================

(import "match.lisp")
(import "logic.lisp")

(display "╔══════════════════════════════════════════════════╗") (newline)
(display "║  Unification + backtracking — Prolog in lizard.  ║") (newline)
(display "╚══════════════════════════════════════════════════╝") (newline)
(newline)

; --- a family database ---
(define family
  (list
    (db-fact '(parent tom bob))
    (db-fact '(parent tom liz))
    (db-fact '(parent bob ann))
    (db-fact '(parent bob pat))
    (db-fact '(parent pat jim))
    ; grandparent(X,Z) :- parent(X,Y), parent(Y,Z)
    (db-rule '(grandparent ?X ?Z)
             '((parent ?X ?Y) (parent ?Y ?Z)))
    ; ancestor(X,Y) :- parent(X,Y)
    (db-rule '(ancestor ?X ?Y) '((parent ?X ?Y)))
    ; ancestor(X,Z) :- parent(X,Y), ancestor(Y,Z)
    (db-rule '(ancestor ?X ?Z)
             '((parent ?X ?Y) (ancestor ?Y ?Z)))))

(display "=== DIRECT FACTS ===") (newline)
(display "  Is tom a parent of bob? ")
(display (provable? family '(parent tom bob))) (newline)
(display "  Is tom a parent of jim? ")
(display (provable? family '(parent tom jim))) (newline)
(newline)

(display "=== QUERIES WITH VARIABLES ===") (newline)
(display "  Who are tom's children? ")
(display (query-var family '(parent tom ?who) '?who)) (newline)
(display "  Who are bob's children? ")
(display (query-var family '(parent bob ?who) '?who)) (newline)
(newline)

(display "=== RULES: grandparent ===") (newline)
(display "  tom's grandchildren: ")
(display (query-var family '(grandparent tom ?gc) '?gc)) (newline)
(display "  Who is ann's grandparent? ")
(display (query-var family '(grandparent ?gp ann) '?gp)) (newline)
(newline)

(display "=== RECURSIVE RULES: ancestor ===") (newline)
(display "  All of tom's descendants: ")
(display (query-var family '(ancestor tom ?d) '?d)) (newline)
(display "  Is tom an ancestor of jim? ")
(display (provable? family '(ancestor tom jim))) (newline)
(newline)

(display "=== COUNTING SOLUTIONS ===") (newline)
(display "  Number of parent facts matching (parent ?x ?y): ")
(display (solution-count family '(parent ?x ?y))) (newline)
(display "  Number of tom's descendants: ")
(display (solution-count family '(ancestor tom ?d))) (newline)
(newline)

(display "=== LIST RELATIONS ===") (newline)
; append relation: append([], L, L).  append([H|T], L, [H|R]) :- append(T, L, R).
; (lists encoded as (cons H T) / nil)
(define list-db
  (list
    (db-fact '(append nil ?L ?L))
    (db-rule '(append (cons ?H ?T) ?L (cons ?H ?R))
             '((append ?T ?L ?R)))))
(display "  append (cons a (cons b nil)) (cons c nil) ?R:") (newline)
(display "    ")
(display (query-var list-db
          '(append (cons a (cons b nil)) (cons c nil) ?R) '?R)) (newline)
(newline)

(display "=== End — a logic language, in 200 lines of lizard ===") (newline)
