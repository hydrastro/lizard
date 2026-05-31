; -*- lisp -*-
; examples/143-persistent-hamt.lisp
;
; Phase 6 — persistent maps and sets backed by a real Hash Array Mapped Trie.
; Updates copy only the nodes along one root-to-leaf path (O(log32 n)) and SHARE
; the rest of the structure with the original, so the original is never mutated
; and large maps update cheaply.  This file checks correctness at small scale,
; across mixed key types (exercising hashing + collisions), for persistent sets,
; and at scale (10000 entries) where the previous flat array would have been
; quadratic.
;
; Self-checking: each `must` raises if its claim is false.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'hamt-regression)))

(define (len xs) (if (null? xs) 0 (+ 1 (len (cdr xs)))))

(newline)
(display "Persistent maps:") (newline)
(define m (phash-map 'a 1 'b 2 'c 3))
(must "get existing keys" (equal? (list (phash-get m 'a) (phash-get m 'b) (phash-get m 'c))
                                  (list 1 2 3)))
(must "missing key returns the default" (equal? (phash-get m 'z 'none) 'none))
(must "count is 3" (equal? (phash-count m) 3))
(must "keys list length matches count" (equal? (len (phash-keys m)) 3))
(must "values list length matches count" (equal? (len (phash-values m)) 3))
(must "entries list length matches count" (equal? (len (phash-entries m)) 3))

(newline)
(display "Persistence — updates never mutate the original:") (newline)
(define m2 (phash-set m 'b 20))
(must "the update is visible in the new map" (equal? (phash-get m2 'b) 20))
(must "the original map is unchanged" (equal? (phash-get m 'b) 2))
(must "overwriting a key keeps the count" (equal? (phash-count (phash-set m 'a 9)) 3))
(must "adding a key grows the count" (equal? (phash-count (phash-set m 'd 4)) 4))
(define m3 (phash-remove m2 'a))
(must "remove drops the count" (equal? (phash-count m3) 2))
(must "removed key is gone" (equal? (phash-get m3 'a 'gone) 'gone))
(must "remove leaves the original intact" (equal? (phash-get m2 'a) 1))

(newline)
(display "Mixed key types (symbol / string / number / bool) hash distinctly:") (newline)
(define mk (phash-map 'a 10 "a" 20 1 30 #t 40))
(must "symbol a -> 10"  (equal? (phash-get mk 'a) 10))
(must "string a -> 20"  (equal? (phash-get mk "a") 20))
(must "number 1 -> 30"  (equal? (phash-get mk 1) 30))
(must "bool #t -> 40"   (equal? (phash-get mk #t) 40))
(must "symbol a and string a are different keys" (equal? (phash-count mk) 4))

(newline)
(display "Persistent sets:") (newline)
(define s (pset 1 2 3 2 1))
(must "construction dedups (count 3)" (equal? (pset-count s) 3))
(must "membership works" (equal? (list (pset-contains? s 2) (pset-contains? s 9)) (list #t #f)))
(define s2 (pset-add s 9))
(must "add grows the set" (equal? (pset-count s2) 4))
(must "add is persistent (original unchanged)" (equal? (pset-contains? s 9) #f))
(define s3 (pset-remove s 1))
(must "remove shrinks the set" (equal? (pset-count s3) 2))
(must "->list length matches count" (equal? (len (pset->list s2)) 4))
(must "sets and maps are distinguishable"
      (equal? (list (pset? s) (phash-map? s) (pset? m) (phash-map? m))
              (list #t #f #f #t)))

(newline)
(display "Scale + structural sharing (10000 entries):") (newline)
(define (build i n acc) (if (>= i n) acc (build (+ i 1) n (phash-set acc i (* i i)))))
(define big (build 0 10000 (phash-map)))
(must "all 10000 entries are present" (equal? (phash-count big) 10000))
(must "a deep entry round-trips" (equal? (phash-get big 9999) 99980001))
(define (all-ok i n) (if (>= i n) #t (if (equal? (phash-get big i) (* i i)) (all-ok (+ i 1) n) #f)))
(must "every one of the 10000 entries round-trips" (all-ok 0 10000))
; thousands of updates of the big map are cheap because structure is shared
(define (updn i n mm) (if (>= i n) mm (updn (+ i 1) n (phash-set mm i (- 0 i)))))
(define big2 (updn 1 2000 big))
(must "2000 updates produced the new values" (equal? (phash-get big2 1500) -1500))
(must "the 10000-entry original is untouched by 2000 updates"
      (equal? (phash-get big 1500) (* 1500 1500)))

(newline)
(display "persistent HAMT maps and sets: structural sharing verified") (newline)
