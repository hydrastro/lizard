; -*- lisp -*-
; ============================================================
;  EXAMPLE 108 — A regex engine in lizard
; ============================================================

(import "regex.lisp")

(display "=== LITERAL STRINGS ===") (newline)
(display "  /abc/ matches \"abc\": ")
(display (regex-match (rx-string "abc") "abc")) (newline)
(display "  /abc/ matches \"abd\": ")
(display (regex-match (rx-string "abc") "abd")) (newline)
(newline)

(display "=== ALTERNATION (cat|dog) ===") (newline)
(define cat-or-dog (rx-alt (rx-string "cat") (rx-string "dog")))
(display "  matches \"cat\": ") (display (regex-match cat-or-dog "cat")) (newline)
(display "  matches \"dog\": ") (display (regex-match cat-or-dog "dog")) (newline)
(display "  matches \"cow\": ") (display (regex-match cat-or-dog "cow")) (newline)
(newline)

(display "=== KLEENE STAR a* ===") (newline)
(define a-star (rx-star (rx-lit "a")))
(display "  /a*/ matches \"\": ") (display (regex-match a-star "")) (newline)
(display "  /a*/ matches \"aaa\": ") (display (regex-match a-star "aaa")) (newline)
(display "  /a*/ matches \"aab\": ") (display (regex-match a-star "aab")) (newline)
(newline)

(display "=== PLUS a+ ===") (newline)
(define a-plus (rx-plus (rx-lit "a")))
(display "  /a+/ matches \"\": ") (display (regex-match a-plus "")) (newline)
(display "  /a+/ matches \"aaa\": ") (display (regex-match a-plus "aaa")) (newline)
(newline)

(display "=== OPTIONAL colou?r ===") (newline)
(define colour (rx-seq (rx-string "colo") (rx-opt (rx-lit "u")) (rx-string "r")))
(display "  matches \"color\": ") (display (regex-match colour "color")) (newline)
(display "  matches \"colour\": ") (display (regex-match colour "colour")) (newline)
(display "  matches \"colur\": ") (display (regex-match colour "colur")) (newline)
(newline)

(display "=== CHARACTER SETS ===") (newline)
(define vowel (rx-set "aeiou"))
(display "  /[aeiou]/ matches \"e\": ") (display (regex-match vowel "e")) (newline)
(display "  /[aeiou]/ matches \"z\": ") (display (regex-match vowel "z")) (newline)
(newline)

(display "=== COMPLEX: /[ab]*c/ ===") (newline)
(define pat (rx-seq (rx-star (rx-set "ab")) (rx-lit "c")))
(display "  matches \"ababc\": ") (display (regex-match pat "ababc")) (newline)
(display "  matches \"c\": ") (display (regex-match pat "c")) (newline)
(display "  matches \"abx\": ") (display (regex-match pat "abx")) (newline)
(newline)

(display "=== SEARCH (match anywhere) ===") (newline)
(define needle (rx-string "cat"))
(display "  search \"cat\" in \"the cat sat\": ")
(display (regex-search needle "the cat sat")) (newline)
(display "  search \"dog\" in \"the cat sat\": ")
(display (regex-search needle "the dog sat")) (newline)
(newline)

(display "=== ANY + STAR: /a.*z/ ===") (newline)
(define a-any-z (rx-seq (rx-lit "a") (rx-star 'any) (rx-lit "z")))
(display "  matches \"abcz\": ") (display (regex-match a-any-z "abcz")) (newline)
(display "  matches \"az\": ") (display (regex-match a-any-z "az")) (newline)
(display "  matches \"abc\": ") (display (regex-match a-any-z "abc")) (newline)
(newline)

(display "=== End — a backtracking regex engine ===") (newline)
