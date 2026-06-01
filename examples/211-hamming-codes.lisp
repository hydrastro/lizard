; 211-hamming-codes.lisp -- binary Hamming single-error-correcting codes.
;
; The order-r Hamming code is a [2^r-1, 2^r-1-r, 3] binary code: data bits sit at the
; non-power-of-two positions, parity bits at the powers of two, and on receipt the syndrome
; read bit by bit gives the binary index of the flipped position (zero if none).  So any
; single bit error is located and corrected.  Certified by zero syndrome on clean codewords
; and recovery from a single error at every position.  `must` raises on failure.

(import "cas/hamming.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'hamming-check-failed)))
(define (databits d k) (db d k 0))
(define (db d k i) (if (>= i k) '() (cons (remainder (quotient d (expt 2 i)) 2) (db d k (+ i 1)))))
(define (all-words r) (aw r 0 (- (expt 2 (- (ham-n r) r)) 1)))
(define (aw r d hi) (cond ((> d hi) #t) ((and (hamming-clean-ok? (databits d (- (ham-n r) r)) r) (hamming-corrects-ok? (databits d (- (ham-n r) r)) r)) (aw r (+ d 1) hi)) (else #f)))

(display "Binary Hamming codes") (newline) (newline)

(display "1. the ") (display (ham-info 3)) (newline)
(define cw (hamming-encode (list 1 0 1 1) 3))
(display "    data 1011 encodes to ") (display (bits->string cw)) (newline)
(display "    flip bit 5: ") (display (bits->string (flip-bit cw 5))) (display " -> syndrome ") (display (hamming-syndrome (flip-bit cw 5) 3)) (display ", decodes to ") (display (bits->string (hamming-decode (flip-bit cw 5) 3))) (newline)
(must "clean codeword has zero syndrome"      (= (hamming-syndrome cw 3) 0))
(must "a single error at bit 5 is corrected"  (corrects-pos? (list 1 0 1 1) 3 5))
(must "every single error is corrected"       (hamming-corrects-ok? (list 1 0 1 1) 3))
(newline)

(display "2. exhaustive over all 16 messages of the [7,4] code") (newline)
(must "every message has zero clean syndrome and corrects all 7 single errors" (all-words 3))
(newline)

(display "3. the larger ") (display (ham-info 4)) (newline)
(must "[15,11] clean codeword decodes"     (hamming-clean-ok? (list 1 0 1 1 0 0 1 0 1 1 0) 4))
(must "[15,11] corrects every single error" (hamming-corrects-ok? (list 1 0 1 1 0 0 1 0 1 1 0) 4))
(must "[15,11] another message corrects all" (hamming-corrects-ok? (list 1 1 1 0 0 0 1 0 1 0 1) 4))
(newline)

(display "all Hamming-code checks passed.") (newline)
