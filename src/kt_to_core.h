/* kt_to_core.h — bridge the trusted kernel's term representation (kterm_t) to the
 * core IR that lowers to interaction nets (core_t / ic_lower).
 *
 * Purpose (Phase 13b): cross-check the interaction-net engine against the trusted
 * kernel reducer kt_whnf.  We translate the *shared computational fragment* of a
 * kernel term and reduce it both on kt_whnf and on the net, then compare.
 *
 * Shared fragment translated here:
 *   KT_VAR, KT_LAM (the domain annotation is dropped — the net is untyped),
 *   KT_APP, KT_PAIR, KT_PROJ1, KT_PROJ2, KT_LET, and Bool via the Church encoding
 *     true     = \t.\f. t
 *     false    = \t.\f. f
 *     bool_rec C t f s  ==>  (s t) f
 * Any term using a former outside this fragment (Nat, Id, List, cubical, ...)
 * yields NULL, and the caller skips it.  This keeps the bridge total and honest
 * rather than silently mis-encoding what the net cannot yet represent.
 */
#ifndef KT_TO_CORE_H
#define KT_TO_CORE_H

#include "kernel.h"     /* kterm_t  */
#include "ic_lower.h"   /* core_t   */

core_t *kt_to_core(const kterm_t *t);

#endif /* KT_TO_CORE_H */
