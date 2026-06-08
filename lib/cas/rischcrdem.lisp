; -*- lisp -*-
; lib/cas/rischcrdem.lisp -- MULTI-PARAMETER homogeneous bookkeeping for the coupled Risch differential equation:
; the general form of the completeness layer, solving for SEVERAL homogeneous constants jointly via a linear
; system rather than the single degree-0 constant of rischcrdeh.  This is the general algorithm: when a coupled
; solve has homogeneous degrees-of-freedom at multiple degrees (each degree whose RDE coefficient vanishes leaves
; a free additive constant), the forced tail depends LINEARLY on the vector of constants, so the terminating
; choice is the solution of a linear system, found by probing and solved over Q, then certified
; (docs/TRAGER_ROADMAP.md, the summit, "multi-parameter homogeneous bookkeeping").
;
; The method.  Run the bottom-up exp banded recurrence (as in rischcrde) but with the homogeneous constants kept
; as injected parameters C = (C_0, ..., C_{p-1}), one per degree whose coefficient vanishes.  The resulting tail
; (the coefficients beyond the integrand's support) is affine in C: tail(C) = T_0 + M C, where T_0 is the tail
; with all C = 0 and column j of M is (tail with C_j = 1, others 0) minus T_0.  Setting tail(C) = 0 gives the
; linear system M C = -T_0, solved over Q by mat-solve.  Substituting the solution and re-running yields the
; candidate, which is CERTIFIED (te-crde-certify); a solution is returned only if it satisfies D y + F y = g, so
; soundness holds and the layer falls back to honest 'inconclusive when no terminating C certifies.
;
; This subsumes rischcrdeh's single-parameter fix (p = 1 is the scalar case) and handles the general
; multi-freedom situation that arises in deeper towers.  For clarity and to bound the search we cap the number
; of tracked parameters and the tail window; within those bounds the linear solve is exact.
;
; Public:
;   te-crdem-solve tower h F g  -> y | 'no-solution | 'inconclusive : the multi-parameter completed solve
;   te-crdem-integrate tower h f -> (list 'elementary y) | (list 'non-elementary ..) | (list 'deferred ..)
;
; Verified: reproduces the single-parameter results (INT (e^x e^{e^x}) = e^{e^x}); solves a constructed
; two-parameter tail system; preserves all non-elementary verdicts; every returned solution certified.
;
; Builds on rischcrde.lisp (the sound coupled solver), rischtowern.lisp, and linalg.lisp (mat-solve over Q).

(import "cas/rischcrde.lisp")
(import "cas/rischcrdeh.lisp")
(import "cas/rischtowern.lisp")
(import "cas/linalg.lisp")

(define (cm-nth l k) (if (= k 0) (car l) (cm-nth (cdr l) (- k 1))))
(define (cm-len l) (if (null? l) 0 (+ 1 (cm-len (cdr l)))))
(define (cm-append l v) (if (null? l) (list v) (cons (car l) (cm-append (cdr l) v))))
(define (cm-reverse l) (cm-rev l (quote ())))
(define (cm-rev l acc) (if (null? l) acc (cm-rev (cdr l) (cons (car l) acc))))

; ===== entry: try the single-parameter layer (which already handles one homogeneous constant + the
; certifies-immediately shortcut); if still inconclusive, attempt the multi-parameter joint solve =====
(define (te-crdem-solve tower h F g) (te-crdem-disp tower h F g (te-crdeh-solve tower h F g)))
(define (te-crdem-disp tower h F g base) (if (equal? base (quote inconclusive)) (te-crdem-try tower h F g) base))
(define (te-crdem-try tower h F g)
  (if (= h 0) (quote inconclusive) (if (equal? (te-level-type tower h) (quote exp)) (te-crdem-exp tower h F g) (quote inconclusive))))

; ----- run the exp recurrence with a vector of injected constants C (one per homogeneous degree, in order).
; The injection: as we go bottom-up, whenever a degree's coefficient is zero we consume the next constant from C
; as that degree's value (instead of the particular base-RDE solve); inner solves recurse through te-crdem-solve.
(define (te-crdem-run tower h F g C)
  (te-cm-go tower h F g (te-deriv tower (- h 1) (te-level-b tower h)) 0 (cm-bound g) (quote ()) C 0))
(define (cm-bound g) (+ 3 (cm-len g)))
(define (te-cm-go tower h F g Db n N ys C ci)
  (if (> n N) ys (te-cm-dispatch tower h F g Db n N ys C ci (te-add tower (- h 1) (te-scale-int (- h 1) n Db) (te-coeff h F 0)))))
(define (te-cm-dispatch tower h F g Db n N ys C ci coeff)
  (if (te-equal? tower (- h 1) coeff (te-zero (- h 1)))
      ; homogeneous degree: consume a constant from C
      (te-cm-step tower h F g Db n N ys C (+ ci 1) (te-cvec tower h C ci))
      (te-cm-step tower h F g Db n N ys C ci
                  (te-crdeh-solve tower (- h 1) coeff (te-sub tower (- h 1) (te-coeff h g n) (te-cm-conv tower h F ys n))))))
; the ci-th constant of C as a height-(h-1) element (a base constant); if C exhausted, zero.
(define (te-cvec tower h C ci) (if (if (< ci 0) #t (>= ci (cm-len C))) (te-zero (- h 1)) (cm-nth C ci)))
(define (te-cm-conv tower h F ys n) (te-cm-conv-go tower h F ys n 1))
(define (te-cm-conv-go tower h F ys n j)
  (if (> j n) (te-zero (- h 1)) (te-add tower (- h 1) (te-mul tower (- h 1) (te-coeff h F j) (te-cm-yget tower h ys (- n j))) (te-cm-conv-go tower h F ys n (+ j 1)))))
(define (te-cm-yget tower h ys i) (if (if (< i 0) #t (>= i (cm-len ys))) (te-zero (- h 1)) (cm-nth ys i)))
(define (te-cm-step tower h F g Db n N ys C ci yn)
  (cond ((equal? yn (quote no-solution)) ys)
        ((equal? yn (quote no-rational-solution)) ys)
        ((equal? yn (quote inconclusive)) ys)
        (else (te-cm-go tower h F g Db (+ n 1) N (cm-append ys yn) C ci))))

; ----- count the homogeneous degrees-of-freedom (how many degrees have zero coefficient up to the bound) -----
(define (te-cm-pcount tower h F g) (te-cm-pc-go tower h F g (te-deriv tower (- h 1) (te-level-b tower h)) 0 (cm-bound g) 0))
(define (te-cm-pc-go tower h F g Db n N acc)
  (if (> n N) acc
      (te-cm-pc-go tower h F g Db (+ n 1) N (if (te-equal? tower (- h 1) (te-add tower (- h 1) (te-scale-int (- h 1) n Db) (te-coeff h F 0)) (te-zero (- h 1))) (+ acc 1) acc))))

; ----- the tail vector: coefficients from g's support up to the bound, as a flat list of height-(h-1) elts -----
(define (te-cm-tailvec tower h g ys) (te-cm-tv-go tower h ys (cm-len g) (cm-bound g)))
(define (te-cm-tv-go tower h ys i top) (if (> i top) (quote ()) (cons (te-cm-yget tower h ys i) (te-cm-tv-go tower h ys (+ i 1) top))))

; ----- build and solve the linear system M C = -T_0 over Q.  We only handle the case where the tail entries are
; base-field constants (height-(h-1) elements whose value is a rational), reducing to a rational linear system;
; otherwise (richer tails) we fall back to inconclusive.  Probe: T_0 = tail(0); column j = tail(e_j) - T_0. -----
(define (te-crdem-exp tower h F g) (te-crdem-build tower h F g (te-cm-pcount tower h F g)))
(define (te-crdem-build tower h F g p)
  (if (= p 0) (quote inconclusive)
      (te-crdem-system tower h F g p (te-cm-tailvec tower h g (te-crdem-run tower h F g (te-zerovec tower h p))))))
(define (te-zerovec tower h p) (if (= p 0) (quote ()) (cons (te-zero (- h 1)) (te-zerovec tower h (- p 1)))))
; T0 = base tail; build columns by unit-probing each parameter.
(define (te-crdem-system tower h F g p T0)
  (te-crdem-solverun tower h F g p T0 (te-crdem-columns tower h F g p T0 0)))
(define (te-crdem-columns tower h F g p T0 j)
  (if (>= j p) (quote ()) (cons (te-cm-colsub tower h T0 (te-cm-tailvec tower h g (te-crdem-run tower h F g (te-unitvec tower h p j)))) (te-crdem-columns tower h F g p T0 (+ j 1)))))
(define (te-unitvec tower h p j) (te-uv-go tower h p j 0))
(define (te-uv-go tower h p j i) (if (>= i p) (quote ()) (cons (if (= i j) (te-one (- h 1)) (te-zero (- h 1))) (te-uv-go tower h p j (+ i 1)))))
; column = (tail_with_e_j - T0) as a list of rationals (extract the base value of each height-(h-1) elt)
(define (te-cm-colsub tower h tailj T0) (te-cm-cs-go tower h tailj T0))
(define (te-cm-cs-go tower h tailj T0) (if (null? tailj) (quote ()) (cons (te-baseval tower h (te-sub tower (- h 1) (car tailj) (car T0))) (te-cm-cs-go tower h (cdr tailj) (cdr T0)))))
; the base rational value of a height-(h-1) element (its degree-0...0 coefficient chain down to Q); if the elt
; has higher-degree parts we still take the base, and the certificate gate will reject if that was lossy.
(define (te-baseval tower h e) (te-bv-go (- h 1) e))
(define (te-bv-go hm e) (if (= hm 0) (te-rat-to-num e) (te-bv-go (- hm 1) (te-coeff hm e 0))))
; convert a rational (num . den) to a number when it is an integer/rational constant; we keep it as the rational
; pair's value via rat-, but mat-solve works over Lisp numbers, so reduce a rational p/q with q|p... instead we
; keep exact: represent the system entries as rationals and use a rational Gaussian elimination wrapper.
(define (te-rat-to-num e) e)

; Because mat-solve operates on Lisp numbers but our entries are rational (num.den) pairs, we solve the system
; using a small exact rational linear solver over the rat-* field instead.  Build M (list of columns) and rhs
; = -T0 (base values), solve, and map back.
(define (te-crdem-solverun tower h F g p T0 cols)
  (te-crdem-finish tower h F g (te-ratsolve tower h cols (te-cm-negbase tower h T0) p)))
(define (te-cm-negbase tower h T0) (if (null? T0) (quote ()) (cons (rat-neg (te-baseval tower h (car T0))) (te-cm-negbase tower h (cdr T0)))))

; exact rational solver: cols is a list of p columns, each a list of rationals (the rows); solve (sum_j C_j col_j)
; = rhs for C (p rationals).  We transpose to row form [row_i = (col_0[i] ... col_{p-1}[i])] and Gaussian-eliminate
; over Q with the rat-* operations.  Returns a list of p rationals or 'none.
(define (te-ratsolve tower h cols rhs p) (te-rs-build tower h cols rhs p))
(define (te-rs-build tower h cols rhs p) (te-rs-go (te-rows-of cols rhs) p 0 (quote ())))
; assemble augmented rows: number of rows = length rhs; row i = (cols[0][i] ... cols[p-1][i] | rhs[i])
(define (te-rows-of cols rhs) (te-ro-go cols rhs 0 (cm-len rhs)))
(define (te-ro-go cols rhs i m) (if (>= i m) (quote ()) (cons (cm-append (te-col-row cols i) (cm-nth rhs i)) (te-ro-go cols rhs (+ i 1) m))))
(define (te-col-row cols i) (if (null? cols) (quote ()) (cons (cm-nth (car cols) i) (te-col-row (cdr cols) i))))

; Gaussian elimination over Q on augmented rows (each row has p+1 rational entries).  Returns p rationals or 'none.
(define (te-rs-go rows p col pivots) (if (>= col p) (te-rs-back rows p) (te-rs-pivot rows p col pivots)))
(define (te-rs-pivot rows p col pivots) (te-rs-find rows p col pivots (te-find-pivot rows col)))
(define (te-find-pivot rows col) (te-fp-go rows col))
(define (te-fp-go rows col)
  (cond ((null? rows) (quote none))
        ((if (not (rat-zero? (te-rget (car rows) col))) (te-earlier-zero? (car rows) col 0) #f) (car rows))
        (else (te-fp-go (cdr rows) col))))
; a candidate pivot row for column col must have zero entries in all columns before col (so it is not the
; pivot of an earlier column already processed)
(define (te-earlier-zero? row col k)
  (cond ((>= k col) #t)
        ((rat-zero? (te-rget row k)) (te-earlier-zero? row col (+ k 1)))
        (else #f)))
(define (te-rget row k) (cm-nth row k))
(define (te-rs-find rows p col pivots pv)
  (if (equal? pv (quote none)) (te-rs-go rows p (+ col 1) pivots)
      (te-rs-go (te-eliminate rows pv col) p (+ col 1) (cm-append pivots pv))))
; normalize pivot row so entry[col]=1, then subtract the right multiple from every OTHER row, and put the
; normalized pivot row back in place of the original.  We identify the pivot row by reference to the original pv.
(define (te-eliminate rows pv col) (te-elim2-go rows pv (te-normrow pv col) col))
(define (te-normrow pv col) (te-scalerow pv (rat-inv (te-rget pv col))))
(define (te-scalerow row s) (if (null? row) (quote ()) (cons (rat-mul s (car row)) (te-scalerow (cdr row) s))))
(define (te-elim2-go rows pv npv col)
  (if (null? rows) (quote ())
      (cons (if (te-rows-eq? (car rows) pv) npv (te-rowsub (car rows) (te-scalerow npv (te-rget (car rows) col))))
            (te-elim2-go (cdr rows) pv npv col))))
(define (te-rows-eq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((rat-equal? (car a) (car b)) (te-rows-eq? (cdr a) (cdr b))) (else #f)))
(define (te-rowsub a b) (if (null? a) (quote ()) (cons (rat-sub (car a) (car b)) (te-rowsub (cdr a) (cdr b)))))
; After full Gauss-Jordan elimination each row is in RREF.  For each parameter column j, find the row whose
; entry in column j is 1 and whose entries in all OTHER parameter columns are 0; that row's augmented (last)
; entry is C_j.  If no such row exists, the value is 0 (free variable -> choose 0).
(define (te-rs-back rows p) (te-rb-go rows p 0 (quote ())))
(define (te-rb-go rows p j acc) (if (>= j p) (cm-reverse acc) (te-rb-go rows p (+ j 1) (cons (te-rb-find rows j p) acc))))
(define (te-rb-find rows j p) (te-rbf-go rows j p))
(define (te-rbf-go rows j p)
  (cond ((null? rows) (rat-zero))
        ((te-is-pivot-row? (car rows) j p) (te-rget (car rows) p))
        (else (te-rbf-go (cdr rows) j p))))
(define (te-is-pivot-row? row j p) (if (rat-equal? (te-rget row j) (rat-one)) (te-others-zero? row j p 0) #f))
(define (te-others-zero? row j p k)
  (cond ((>= k p) #t)
        ((= k j) (te-others-zero? row j p (+ k 1)))
        ((rat-zero? (te-rget row k)) (te-others-zero? row j p (+ k 1)))
        (else #f)))

; ----- substitute the solved constants, re-run, trim, certify -----
(define (te-crdem-finish tower h F g C)
  (cond ((equal? C (quote none)) (quote inconclusive))
        (else (te-crdem-verify tower h F g (te-crdem-trim tower h (te-crdem-run tower h F g (te-lift-consts tower h C)))))))
; lift the solved rationals C into height-(h-1) base-constant elements
(define (te-lift-consts tower h C) (if (null? C) (quote ()) (cons (te-lift1 tower h (car C)) (te-lift-consts tower h (cdr C)))))
(define (te-lift1 tower h r) (te-l1-go (- h 1) r))
(define (te-l1-go hm r) (if (= hm 0) r (list (te-l1-go (- hm 1) r))))
(define (te-crdem-trim tower h ys) (cm-reverse (te-crdem-dropz tower h (cm-reverse ys))))
(define (te-crdem-dropz tower h l) (cond ((null? l) (quote ())) ((te-equal? tower (- h 1) (car l) (te-zero (- h 1))) (te-crdem-dropz tower h (cdr l)) ) (else l)))
(define (te-crdem-verify tower h F g y) (if (te-crde-certify tower h F g y) y (quote inconclusive)))

; ===== integration entry =====
(define (te-crdem-integrate tower h f) (te-crdem-result (te-crdem-solve tower h (te-zero h) f)))
(define (te-crdem-result y)
  (cond ((equal? y (quote no-solution)) (list (quote non-elementary) (quote tower-rde-obstruction)))
        ((equal? y (quote inconclusive)) (list (quote deferred) (quote needs-deeper-structure)))
        (else (list (quote elementary) y))))
