; -*- lisp -*-
; lib/cas/hamming.lisp -- binary Hamming single-error-correcting codes.
;
; The Hamming code of order r is a [2^r - 1, 2^r - 1 - r] binary code of minimum distance 3,
; so it corrects any single bit error.  Positions 1..n are numbered, the r parity positions
; are the powers of two, and the parity bit at position 2^i is the XOR of all data bits at
; positions whose i-th bit is set -- equivalently, parity check i covers exactly those
; positions.  On receipt the syndrome is read off bit by bit: syndrome bit i is parity check
; i of the received word, and the integer they form is precisely the position of the flipped
; bit (zero meaning no error), so decoding flips that one position and reads the data back.
;
; Two facts certify it: a clean codeword has zero syndrome (it satisfies every parity
; check), and a single error in any of the n positions is located and corrected, so the
; original message is recovered.  Self-contained over GF(2).

(define (xor a b) (remainder (+ a b) 2))
(define (bit j i) (remainder (quotient j (expt 2 i)) 2))
(define (pow2? j) (p2 j 1))
(define (p2 j k) (cond ((= k j) #t) ((> k j) #f) (else (p2 j (* k 2)))))
(define (nthc l i) (if (= i 0) (car l) (nthc (cdr l) (- i 1))))
(define (nth1 l pos) (nthc l (- pos 1)))
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))
(define (set-nth1 l pos v) (set-nth l (- pos 1) v))
(define (ham-n r) (- (expt 2 r) 1))

; ---------- encoding ----------
(define (place-data data n pos acc)
  (if (> pos n) (reverse acc)
      (if (pow2? pos) (place-data data n (+ pos 1) (cons 0 acc))
          (place-data (cdr data) n (+ pos 1) (cons (car data) acc)))))
(define (parity-i cw i n pos acc) (if (> pos n) acc (parity-i cw i n (+ pos 1) (if (= (bit pos i) 1) (xor acc (nth1 cw pos)) acc))))
(define (set-parities cw i r n) (if (>= i r) cw (set-parities (set-nth1 cw (expt 2 i) (parity-i cw i n 1 0)) (+ i 1) r n)))
(define (hamming-encode data r) (set-parities (place-data data (ham-n r) 1 '()) 0 r (ham-n r)))

; ---------- decoding ----------
(define (synbit cw i n pos acc) (if (> pos n) acc (synbit cw i n (+ pos 1) (if (= (nth1 cw pos) 1) (xor acc (bit pos i)) acc))))
(define (syndrome cw r n i acc) (if (>= i r) acc (syndrome cw r n (+ i 1) (+ acc (* (synbit cw i n 1 0) (expt 2 i))))))
(define (hamming-syndrome cw r) (syndrome cw r (ham-n r) 0 0))
(define (hamming-correct cw r) (let ((s (hamming-syndrome cw r))) (if (= s 0) cw (set-nth1 cw s (xor (nth1 cw s) 1)))))
(define (extract-data cw n pos acc) (if (> pos n) (reverse acc) (if (pow2? pos) (extract-data cw n (+ pos 1) acc) (extract-data cw n (+ pos 1) (cons (nth1 cw pos) acc)))))
(define (hamming-decode cw r) (extract-data (hamming-correct cw r) (ham-n r) 1 '()))

; ---------- corrupting (for tests) ----------
(define (flip-bit cw pos) (set-nth1 cw pos (xor (nth1 cw pos) 1)))

; ---------- certificates ----------
(define (hamming-clean-ok? data r)
  (and (= (hamming-syndrome (hamming-encode data r) r) 0) (equal? (hamming-decode (hamming-encode data r) r) data)))
(define (corrects-pos? data r pos) (equal? (hamming-decode (flip-bit (hamming-encode data r) pos) r) data))
(define (corrects-all data r) (ca data r 1 (ham-n r)))
(define (ca data r pos n) (cond ((> pos n) #t) ((corrects-pos? data r pos) (ca data r (+ pos 1) n)) (else #f)))
(define (hamming-corrects-ok? data r) (corrects-all data r))

; ---------- display ----------
(define (bits->string b) (if (null? b) "" (string-append (number->string (car b)) (bits->string (cdr b)))))
(define (ham-info r) (string-append "[" (number->string (ham-n r)) "," (number->string (- (ham-n r) r)) ",3] Hamming code"))
