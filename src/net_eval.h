#ifndef NET_EVAL_H
#define NET_EVAL_H

/* net_eval — the seam by which the interaction net becomes the primary engine
 * (Phase 16, started).
 *
 * The cross-check (Phase 13b) established that the net agrees with the trusted
 * reducer kt_whnf over the kernel's computational fragment.  This module turns
 * that into an actual evaluator: it takes a closed kernel term, runs it on the
 * net (kt_to_core -> ic_lower -> reduce -> observe), and reads the result back
 * as a kterm.  A gate selects, per call, between the net and kt_whnf, with the
 * trusted reducer as the fallback whenever the net declines (a term outside the
 * supported fragment).  Flipping the gate is what "the net becomes the engine"
 * means operationally; the fallback keeps every other term working meanwhile.
 *
 * Scope of this first step: the net currently reads back results of Nat or Bool
 * type (the caller states which).  Type-directed dispatch (so the kind need not
 * be supplied), richer result types, and routing the system's main evaluation
 * path through here are the remainder of Phase 16.
 */

#include "kernel.h"   /* kterm_t, kctx_t, lizard_heap_t */

typedef enum { NET_RESULT_NAT, NET_RESULT_BOOL } net_result_kind;

/* Evaluate `t` on the net.  On success (the term is in the supported fragment
 * and yields a value of the requested kind) write the fully-evaluated value as a
 * kterm — a Nat numeral, or a KT_TRUE/KT_FALSE node — into *out and return 1.
 * Return 0 if the net declines (the caller should fall back to kt_whnf). */
int kt_eval_via_net(lizard_heap_t *heap, kctx_t *ctx, const kterm_t *t,
                    net_result_kind kind, kterm_t **out);

/* The gate (default: off). */
void kt_net_eval_set_enabled(int on);
int  kt_net_eval_enabled(void);

/* Normalise through the gate: if the gate is on and the net handles `t`, return
 * the net's value; otherwise fall back to the trusted reducer kt_whnf. */
kterm_t *kt_normalize_gated(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t,
                            net_result_kind kind);

#endif /* NET_EVAL_H */
