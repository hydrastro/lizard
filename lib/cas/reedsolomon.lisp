; -*- lisp -*-
; lib/cas/reedsolomon.lisp -- Reed-Solomon error-correcting codes over F_p.
;
; A message of k symbols is the coefficient list of a polynomial m of degree < k; its
; codeword is the evaluations (m(x_1), ..., m(x_n)) at n distinct points of F_p.  This is a
; maximum-distance-separable [n, k] code with minimum distance n - k + 1, correcting up to
; t = floor((n - k) / 2) symbol errors.
;
; Decoding uses the Berlekamp-Welch method.  If the received word r differs from a codeword
; in <= t places, there is an error-locator polynomial E of degree <= t (vanishing at the
; error points) and a polynomial N of degree < t + k with N(x_i) = r_i E(x_i) for all i.
; Those relations are LINEAR in the unknown coefficients of E (taken monic of degree t) and
; N, so a Gaussian elimination over F_p recovers them, and the message is the exact
; quotient m = N / E.  The whole scheme is gated by the round trip: encode a message,
; corrupt up to t positions, and decoding must return the original message.  Builds on
; ffactor.lisp (finite-field polynomial arithmetic).

(import "cas/ffactor.lisp")

; ---------- evaluation and encoding ----------
(define (peval-at c x p) (if (null? c) 0 (imod (+ (car c) (* x (peval-at (cdr c) x p))) p)))
(define (rs-points n) (rs-pts 0 n))
(define (rs-pts i n) (if (>= i n) '() (cons i (rs-pts (+ i 1) n))))
(define (rs-encode msg xs p) (map (lambda (x) (peval-at msg x p)) xs))
(define (rs-t n k) (quotient (- n k) 2))
(define (rs-distance n k) (+ (- n k) 1))

; ---------- corrupting a codeword (for tests) ----------
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))
(define (rs-corrupt cw errs p)
  (if (null? errs) cw
      (rs-corrupt (set-nth cw (car (car errs)) (imod (+ (nthc cw (car (car errs))) (cdr (car errs))) p)) (cdr errs) p)))

; ---------- Gaussian elimination over F_p (augmented rows; particular solution) ----------
(define (map2 f a b) (if (or (null? a) (null? b)) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (scale-row row c p) (map (lambda (x) (imod (* x c) p)) row))
(define (axpy dst src factor p) (map2 (lambda (x y) (imod (- x (* factor y)) p)) dst src))   ; dst - factor*src
(define (get-row rows i) (nthc rows i))
(define (set-row rows i v) (set-nth rows i v))
(define (swap-rows rows i j) (set-row (set-row rows i (get-row rows j)) j (get-row rows i)))
(define (find-nz rows r c p n) (cond ((>= r n) -1) ((= (imod (nthc (get-row rows r) c) p) 0) (find-nz rows (+ r 1) c p n)) (else r)))
(define (elim-col rows r c p) (ec-go rows r c p 0 (length rows)))
(define (ec-go rows r c p k n)
  (cond ((>= k n) rows)
        ((= k r) (ec-go rows r c p (+ k 1) n))
        (else (ec-go (set-row rows k (axpy (get-row rows k) (get-row rows r) (nthc (get-row rows k) c) p)) r c p (+ k 1) n))))
(define (rref rows r c ncols p)
  (if (or (>= c ncols) (>= r (length rows))) rows
      (let ((piv (find-nz rows r c p (length rows))))
        (if (< piv 0) (rref rows r (+ c 1) ncols p)
            (let ((rows1 (swap-rows rows r piv)))
              (let ((rows2 (set-row rows1 r (scale-row (get-row rows1 r) (mod-inverse (nthc (get-row rows1 r) c) p) p))))
                (rref (elim-col rows2 r c p) (+ r 1) (+ c 1) ncols p)))))))
(define (leading-col row ncols) (lc-go row 0 ncols))
(define (lc-go row j ncols) (cond ((>= j ncols) -1) ((= (nthc row j) 0) (lc-go row (+ j 1) ncols)) (else j)))
(define (build-sol rows ncols sol p)
  (cond ((null? rows) sol)
        ((equal? sol 'none) 'none)
        (else (let ((lcv (leading-col (car rows) ncols)))
                (if (< lcv 0)
                    (if (= (imod (nthc (car rows) ncols) p) 0) (build-sol (cdr rows) ncols sol p) 'none)
                    (build-sol (cdr rows) ncols (set-nth sol lcv (imod (nthc (car rows) ncols) p)) p))))))
(define (solve-mod rows p)
  (let ((ncols (- (length (car rows)) 1)))
    (build-sol (rref rows 0 0 ncols p) ncols (zeros ncols) p)))

; ---------- Berlekamp-Welch decode ----------
(define (powers x p hi) (pw x p 0 hi 1))            ; (x^0 x^1 ... x^{hi-1})
(define (pw x p i hi acc) (if (>= i hi) '() (cons acc (pw x p (+ i 1) hi (imod (* acc x) p)))))
(define (bw-row x r e k p)
  (append (powers x p (+ e k))                       ; N coefficients: x^0..x^{e+k-1}
          (append (map (lambda (v) (imod (- 0 (* r v)) p)) (powers x p e))   ; E coeffs: -r*x^0..-r*x^{e-1}
                  (list (imod (* r (imod (expt x e) p)) p)))))                ; RHS: r*x^e
(define (bw-system xs rs e k p) (map2 (lambda (x r) (bw-row x r e k p)) xs rs))
(define (take-n l n) (if (or (= n 0) (null? l)) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (or (= n 0) (null? l)) l (drop-n (cdr l) (- n 1))))
(define (rs-decode r xs k p)
  (let ((n (length xs)) (e (rs-t (length xs) k)))
    (let ((sol (solve-mod (bw-system xs r e k p) p)))
      (if (equal? sol 'none) 'fail
          (let ((Npoly (trim (take-n sol (+ e k)))) (Elow (take-n (drop-n sol (+ e k)) e)))
            (let ((Epoly (trim (pnorm (append Elow (list 1)) p))))
              (let ((dm (pdivmod Npoly Epoly p)))
                (if (null? (trim (cdr dm))) (pad (trim (car dm)) k) 'fail))))))))
(define (pad l k) (if (>= (length l) k) (take-n l k) (append l (zeros (- k (length l))))))

; ---------- certificates ----------
(define (msg-equal? a b k p) (equal? (pad (trim (pnorm a p)) k) (pad (trim (pnorm b p)) k)))
(define (rs-roundtrip-ok? msg xs k p errs)
  (msg-equal? (rs-decode (rs-corrupt (rs-encode msg xs p) errs p) xs k p) msg k p))
(define (rs-clean-ok? msg xs k p) (msg-equal? (rs-decode (rs-encode msg xs p) xs k p) msg k p))

; ---------- display ----------
(define (rs-info n k p) (string-append "[" (number->string n) "," (number->string k) "] RS over F_" (number->string p) ", distance " (number->string (rs-distance n k)) ", corrects " (number->string (rs-t n k))))
