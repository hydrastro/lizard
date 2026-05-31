; 146-persistent-vector.lisp — the persistent vector is now a 32-way
; bit-partitioned trie with a tail (Clojure's PersistentVector): O(1) amortized
; append, O(log32 n) indexed lookup/update, and structural sharing.  This
; replaces the old flat array whose update/append copied the whole vector.
;
; Self-checking: each `must` raises on failure (non-zero exit).

(define (must l x)
  (display "  ") (display l) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pvec-check-failed)))

(display "persistent-vector: trie correctness + sharing") (newline)

; ---- build a large vector by repeated append ----
(define (build i n v) (if (< i n) (build (+ i 1) n (pvec-push v i)) v))
(define big (build 0 3000 (pvec)))           ; crosses 32 and 1024 boundaries

(must "count is 3000" (= (pvec-count big) 3000))

; every index reads back its value (navigates the trie at every depth)
(define (ok? i n v) (if (< i n) (if (= (pvec-ref v i) i) (ok? (+ i 1) n v) #f) #t))
(must "all 3000 indices correct" (ok? 0 3000 big))

; the boundaries where the trie grows a level
(must "boundary 31/32"     (and (= (pvec-ref big 31) 31)   (= (pvec-ref big 32) 32)))
(must "boundary 1023/1024" (and (= (pvec-ref big 1023) 1023) (= (pvec-ref big 1024) 1024)))
(must "last element"       (= (pvec-ref big 2999) 2999))

; ---- functional update is persistent (the original is untouched) ----
(define v1 (pvec-set big 1500 (quote changed)))
(must "update visible in copy"   (equal? (pvec-ref v1 1500) (quote changed)))
(must "original is unchanged"    (= (pvec-ref big 1500) 1500))
(must "copy keeps its neighbours" (and (= (pvec-ref v1 1499) 1499) (= (pvec-ref v1 1501) 1501)))
(must "copy keeps the count"     (= (pvec-count v1) 3000))

; many independent updates off one base all coexist (sharing the base)
(define a (pvec-set big 0 (quote A)))
(define b (pvec-set big 0 (quote B)))
(must "independent updates coexist"
      (and (equal? (pvec-ref a 0) (quote A))
           (equal? (pvec-ref b 0) (quote B))
           (= (pvec-ref big 0) 0)))

; ---- pop ----
(define (popn k v) (if (> k 0) (popn (- k 1) (pvec-pop v)) v))
(define shrunk (popn 1985 big))              ; 3000 -> 1015
(must "pop count 1015"      (= (pvec-count shrunk) 1015))
(must "pop preserves head"  (= (pvec-ref shrunk 0) 0))
(must "pop preserves tail"  (= (pvec-ref shrunk 1014) 1014))

; ---- conversions ----
(must "pvec literal -> list"  (equal? (pvec->list (pvec 10 20 30)) (list 10 20 30)))
(must "list -> pvec -> list"  (equal? (pvec->list (list->pvec (list 1 2 3 4 5))) (list 1 2 3 4 5)))
(must "empty vector"          (= (pvec-count (pvec)) 0))
(must "push onto empty"       (= (pvec-ref (pvec-push (pvec) 99) 0) 99))

(display "persistent-vector: all checks passed") (newline)
