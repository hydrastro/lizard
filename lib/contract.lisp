; -*- lisp -*-
; lib/contract.lisp — higher-order contracts with blame (Findler–Felleisen).
;
; A contract is checked when a value crosses a boundary between two parties: a
; POSITIVE party (pos — the value's producer/server) and a NEGATIVE party
; (neg — the consumer/client).
;
;   * A flat (predicate) contract blames whoever produced the bad value.
;   * A function contract (-> dom rng) checks the DOMAIN with the parties
;     SWAPPED — so a bad argument blames the caller — and the RANGE with the
;     parties as-is — so a bad result blames the function.  Because rng may
;     itself be a (-> ...), this gives curried, higher-order contracts.
;
; Exceptions in lizard are values (`raise` yields an error object), so `protect`
; threads them through explicitly; a violation surfaces as an error object whose
; payload is  (contract-violation PARTY CONTRACT-NAME VALUE).

; ---- constructors --------------------------------------------------------
(define (flat pred name) (list 'flat pred name))
(define (-> dom rng)     (list 'arrow dom rng))

; ---- accessors -----------------------------------------------------------
(define (contract-flat? c) (equal? (car c) 'flat))
(define (flat-pred c)      (car (cdr c)))
(define (flat-name c)      (car (cdr (cdr c))))
(define (contract-dom c)   (car (cdr c)))
(define (contract-rng c)   (car (cdr (cdr c))))

; ---- blame ---------------------------------------------------------------
; Signal a violation as an error object that names the responsible party.
(define (blame party cname val)
  (raise (list 'contract-violation party cname val)))

; ---- attach a contract to a value ---------------------------------------
; protect : contract value positive-party negative-party -> guarded-value
(define (protect ctr val pos neg)
  (cond
    ((contract-flat? ctr)
     (if ((flat-pred ctr) val) val (blame pos (flat-name ctr) val)))
    ((equal? (car ctr) 'arrow)
     (lambda (x)
       (let ((cx (protect (contract-dom ctr) x neg pos)))   ; domain: blame swaps
         (if (error-object? cx)
             cx                                              ; bad argument
             (let ((r (val cx)))
               (if (error-object? r)
                   r                                         ; inner violation
                   (protect (contract-rng ctr) r pos neg))))))) ; range: blame as-is
    (else val)))

; ---- inspecting a violation ---------------------------------------------
(define (violation? x)         (error-object? x))
(define (blamed-party x)       (car (cdr (error-value x))))
(define (violated-contract x)  (car (cdr (cdr (error-value x)))))
