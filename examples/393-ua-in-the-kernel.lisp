; -*- lisp -*-
; ua in the TRUSTED kernel.  Univalence's typing now lives in lizard's audited kernel (kernel.c): the kernel has
; new term formers KT_EQUIV (Equiv A B, the type of equivalences) and KT_UA (ua e), with typing rules
;   (Equiv A B) : Sort(max la lb)          when A : Sort(la), B : Sort(lb)
;   (ua e)      : (Id (Sort n) A B)         when e : (Equiv A B), A,B : Sort(n)
; so an equivalence yields an IDENTITY BETWEEN THE TWO TYPES IN THE UNIVERSE -- univalence, checked by the trusted
; kt_infer, with the discriminating rejections that make it sound.
;
; SCOPE, STATED HONESTLY: this brings the TYPING of univalence into the audited kernel (formation + the ua rule +
; soundness rejections), verified against kt_infer.  It does NOT add full COMPUTATIONAL univalence -- transport
; across ua reducing through Glue/Kan composition -- which would require the entire Glue/comp machinery in the
; kernel and is a separate, larger undertaking.  What is claimed: ua is now a kernel-typed construct, sound.
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote ua-kernel-regression))))
(kernel-assume (quote A) (quote (Sort 0)))
(kernel-assume (quote B) (quote (Sort 0)))
(kernel-assume (quote e) (quote (Equiv A B)))

(display "ua in the trusted kernel: an equivalence is an identity in the universe, checked by kt_infer.") (newline) (newline)

(must "the kernel forms (Equiv A B) at (Sort 0)" (kernel-check (quote (Equiv A B)) (quote (Sort 0))))
(must "the kernel types (ua e) as an identity in the universe -- (ua e) : (Id (Sort 0) A B)"
  (kernel-check (quote (ua e)) (quote (Id (Sort 0) A B))))

; soundness: ua of a non-equivalence is rejected
(kernel-assume (quote notequiv) (quote A))
(must "the kernel REJECTS (ua notequiv) where notequiv : A is not an equivalence"
  (not (kernel-check (quote (ua notequiv)) (quote (Id (Sort 0) A B)))))
; soundness: ua produces the CORRECT identity, not a wrong one
(must "the kernel REJECTS (ua e) : (Id (Sort 0) A A) -- e is an A~B equivalence, not A~A"
  (not (kernel-check (quote (ua e)) (quote (Id (Sort 0) A A)))))

(newline)
(display "Univalence's typing is now in the audited kernel: (Equiv A B) forms, (ua e) yields an identity in the") (newline)
(display "universe, and the kernel rejects ua of a non-equivalence and ua claiming a wrong identity.  The typing of") (newline)
(display "univalence is kernel-anchored and sound; full computational univalence (transport through Glue) remains a") (newline)
(display "separate, larger undertaking and is not claimed here.") (newline)
