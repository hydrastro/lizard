; -*- lisp -*-
; lib/cas/rsbm.lisp -- Reed-Solomon decoding by syndromes + Berlekamp-Massey.
;
; This is the classical "spectral" Reed-Solomon decoder, complementary to the Berlekamp-Welch
; decoder in reedsolomon.lisp.  Fix a primitive element alpha of F_p (so its powers run over
; all nonzero residues, n = p - 1).  The generator polynomial g(x) = prod_{j=1}^{2t}(x -
; alpha^j) defines an [n, k] code with k = n - 2t: a message m (k coefficients) is encoded as
; c = m * g, whose spectrum vanishes at alpha^1..alpha^{2t}.  Hence a clean codeword has zero
; syndromes S_j = r(alpha^j).
;
; Given a received word with <= t errors, the syndromes obey a linear recurrence whose
; shortest generator -- found by the Berlekamp-Massey LFSR-synthesis algorithm over F_p -- is
; the error-locator polynomial Lambda.  A Chien search evaluates Lambda at alpha^{-i} to find
; the error positions, and a small Vandermonde system Sum_l Y_l (alpha^{i_l})^j = S_j recovers
; the error magnitudes (sidestepping Forney's index-convention pitfalls).  Subtracting the
; error vector and dividing by g recovers the message exactly.
;
; Certified two ways: the round trip (corrupt up to t positions, recover the message) and the
; structural identity that the locator's roots are exactly the injected error positions.
; Builds on ffactor.lisp (polynomial arithmetic) and discretelog.lisp (primitive-root).

(import "cas/ffactor.lisp")
(import "cas/discretelog.lisp")

; ---------- safe coefficient access (0 past the end), local list helpers ----------
(define (c-at l i) (cond ((null? l) 0) ((= i 0) (car l)) (else (c-at (cdr l) (- i 1)))))
(define (map2 f a b) (if (or (null? a) (null? b)) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (padto l n) (if (>= (length l) n) l (append l (zeros (- n (length l))))))
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))

; ---------- code parameters / generator ----------
(define (rsbm-n p) (- p 1))
(define (rsbm-k p t) (- (- p 1) (* 2 t)))
(define (rsbm-distance t) (+ (* 2 t) 1))
(define (gen-poly a t p) (gp-go a 1 (* 2 t) p (list 1)))
(define (gp-go a j hi p acc) (if (> j hi) acc (gp-go a (+ j 1) hi p (pmul acc (list (imod (- 0 (mod-exp a j p)) p) 1) p))))

; ---------- encode / corrupt ----------
(define (rsbm-encode msg a t p) (padto (pnorm (pmul (pnorm msg p) (gen-poly a t p) p) p) (rsbm-n p)))
(define (rsbm-corrupt cw errs p)
  (if (null? errs) cw
      (rsbm-corrupt (set-nth cw (car (car errs)) (imod (+ (c-at cw (car (car errs))) (cdr (car errs))) p)) (cdr errs) p)))

; ---------- syndromes ----------
(define (poly-eval-pt c x p) (pe-go c x p 0 1))         ; Sum c_i x^i, low->high
(define (pe-go c x p acc xi) (if (null? c) acc (pe-go (cdr c) x p (imod (+ acc (* (car c) xi)) p) (imod (* xi x) p))))
(define (syndromes r a t p) (syn-go r a 1 (* 2 t) p))
(define (syn-go r a j hi p) (if (> j hi) '() (cons (poly-eval-pt r (mod-exp a j p) p) (syn-go r a (+ j 1) hi p))))
(define (all-zero? s) (cond ((null? s) #t) ((= (car s) 0) (all-zero? (cdr s))) (else #f)))

; ---------- Berlekamp-Massey over F_p (LFSR synthesis) ----------
; sequence s (0-indexed: s_0=S_1, ...); returns (Lambda . L), Lambda monic-at-0 (const term 1).
(define (pscale poly c p) (map (lambda (z) (imod (* z c) p)) poly))
(define (shiftm poly m) (append (zeros m) poly))
(define (psub a b p) (let ((n (max (length a) (length b)))) (map2 (lambda (x y) (imod (- x y) p)) (padto a n) (padto b n))))
(define (discrepancy C s n L p) (d-go C s n 1 L (c-at s n) p))
(define (d-go C s n i L acc p) (if (> i L) (imod acc p) (d-go C s n (+ i 1) L (+ acc (* (c-at C i) (c-at s (- n i)))) p)))
(define (bm s p) (bm-go s p (length s) 0 (list 1) (list 1) 0 1 1))
(define (bm-go s p N n C B L m b)
  (if (>= n N) (cons (trim C) L)
      (let ((d (discrepancy C s n L p)))
        (if (= d 0)
            (bm-go s p N (+ n 1) C B L (+ m 1) b)
            (let ((Cnew (psub C (pscale (shiftm B m) (imod (* d (mod-inverse b p)) p) p) p)))
              (if (<= (* 2 L) n)
                  (bm-go s p N (+ n 1) Cnew C (+ (- n L) 1) 1 d)
                  (bm-go s p N (+ n 1) Cnew B L (+ m 1) b)))))))

; ---------- Chien search: error positions i where Lambda(alpha^{-i}) = 0 ----------
(define (chien Lam a n p) (ch-go Lam (mod-inverse a p) 0 n p))
(define (ch-go Lam ainv i n p)
  (cond ((>= i n) '())
        ((= (poly-eval-pt Lam (mod-exp ainv i p) p) 0) (cons i (ch-go Lam ainv (+ i 1) n p)))
        (else (ch-go Lam ainv (+ i 1) n p))))

; ---------- magnitudes: small Vandermonde solve over F_p ----------
(define (scale-row row c p) (map (lambda (x) (imod (* x c) p)) row))
(define (axpy dst src f p) (map2 (lambda (x y) (imod (- x (* f y)) p)) dst src))
(define (get-row rows i) (c-at rows i))
(define (set-row rows i v) (set-nth rows i v))
(define (swap-rows rows i j) (set-row (set-row rows i (get-row rows j)) j (get-row rows i)))
(define (find-nz rows r c p n) (cond ((>= r n) -1) ((= (imod (c-at (get-row rows r) c) p) 0) (find-nz rows (+ r 1) c p n)) (else r)))
(define (elim-col rows r c p) (ec-go rows r c p 0 (length rows)))
(define (ec-go rows r c p k n)
  (cond ((>= k n) rows) ((= k r) (ec-go rows r c p (+ k 1) n))
        (else (ec-go (set-row rows k (axpy (get-row rows k) (get-row rows r) (c-at (get-row rows k) c) p)) r c p (+ k 1) n))))
(define (rref rows r c ncols p)
  (if (or (>= c ncols) (>= r (length rows))) rows
      (let ((piv (find-nz rows r c p (length rows))))
        (if (< piv 0) (rref rows r (+ c 1) ncols p)
            (let ((r1 (swap-rows rows r piv)))
              (let ((r2 (set-row r1 r (scale-row (get-row r1 r) (mod-inverse (c-at (get-row r1 r) c) p) p))))
                (rref (elim-col r2 r c p) (+ r 1) (+ c 1) ncols p)))))))
(define (leading-col row ncols) (lc-go row 0 ncols))
(define (lc-go row j ncols) (cond ((>= j ncols) -1) ((= (c-at row j) 0) (lc-go row (+ j 1) ncols)) (else j)))
(define (build-sol rows ncols sol p)
  (cond ((null? rows) sol) ((equal? sol 'none) 'none)
        (else (let ((lcv (leading-col (car rows) ncols)))
                (if (< lcv 0) (if (= (imod (c-at (car rows) ncols) p) 0) (build-sol (cdr rows) ncols sol p) 'none)
                    (build-sol (cdr rows) ncols (set-nth sol lcv (imod (c-at (car rows) ncols) p)) p))))))
(define (solve-aug rows p) (let ((ncols (- (length (car rows)) 1))) (build-sol (rref rows 0 0 ncols p) ncols (zeros ncols) p)))
(define (mag-rows positions a nu p syn) (mr-go positions a 1 nu p syn))
(define (mr-go positions a j nu p syn)
  (if (> j nu) '()
      (cons (append (map (lambda (i) (mod-exp (mod-exp a i p) j p)) positions) (list (c-at syn (- j 1))))
            (mr-go positions a (+ j 1) nu p syn))))

; ---------- assemble + decode ----------
(define (apply-errs r positions Y p) (ae-go r positions Y p))
(define (ae-go r positions Y p)
  (if (null? positions) r
      (ae-go (set-nth r (car positions) (imod (- (c-at r (car positions)) (car Y)) p)) (cdr positions) (cdr Y) p)))
(define (recover c a t p) (let ((dm (pdivmod (trim c) (gen-poly a t p) p))) (if (null? (trim (cdr dm))) (trim (car dm)) 'fail)))
(define (rsbm-decode r a t p)
  (let ((n (rsbm-n p)) (s (syndromes (padto r (rsbm-n p)) a t p)))
    (if (all-zero? s) (recover (padto r n) a t p)
        (let ((bmres (bm s p)))
          (let ((Lam (car bmres)) (nu (cdr bmres)))
            (let ((positions (chien Lam a n p)))
              (if (or (= (length positions) 0) (not (= (length positions) nu))) 'fail
                  (let ((Y (solve-aug (mag-rows positions a nu p s) p)))
                    (if (equal? Y 'none) 'fail (recover (apply-errs (padto r n) positions Y p) a t p))))))))))

; ---------- certificates ----------
(define (msg-eq? a b p) (equal? (trim (pnorm a p)) (trim (pnorm b p))))
(define (rsbm-roundtrip-ok? msg a t p errs)
  (msg-eq? (rsbm-decode (rsbm-corrupt (rsbm-encode msg a t p) errs p) a t p) msg p))
(define (rsbm-clean-ok? msg a t p) (msg-eq? (rsbm-decode (rsbm-encode msg a t p) a t p) msg p))
(define (positions-of errs) (map car errs))
(define (sortl l) (if (null? l) l (insert (car l) (sortl (cdr l)))))
(define (insert x l) (cond ((null? l) (list x)) ((<= x (car l)) (cons x l)) (else (cons (car l) (insert x (cdr l))))))
; the locator's roots (Chien positions) equal the injected error positions
(define (rsbm-locator-ok? msg a t p errs)
  (let ((cw (rsbm-corrupt (rsbm-encode msg a t p) errs p)))
    (let ((s (syndromes cw a t p)))
      (equal? (sortl (chien (car (bm s p)) a (rsbm-n p) p)) (sortl (positions-of errs))))))

; ---------- display ----------
(define (rsbm-spectrum-zero? msg a t p)            ; structural: clean codeword vanishes at alpha^1..alpha^2t
  (all-zero? (syndromes (rsbm-encode msg a t p) a t p)))
(define (rsbm-info p t) (string-append "[" (number->string (rsbm-n p)) "," (number->string (rsbm-k p t)) "] BCH-form RS over F_" (number->string p) ", corrects " (number->string t)))
