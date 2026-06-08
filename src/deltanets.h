/* deltanets.h — Delta-Nets (Salvadori, arXiv:2505.20314, 2025).
 *
 * An interaction-net model for optimal parallel lambda-reduction that replaces
 * the brackets/croissants of Lamping / Asperti-Guerrini (which accumulate until
 * they overwhelm reduction -- the wall hit in opt_core) with a SINGLE agent: the
 * `replicator`, an n-ary fan carrying a level and a per-port level-delta. Both
 * abstraction and application are plain fans; beta is fan annihilation.
 *
 * This is the CORE interaction system (fans, erasers, replicators) plus the
 * lambda<->net translation. The core is perfectly confluent and, validated
 * against the reference normaliser, correctly reduces a large fragment that
 * opt_core could not -- including Church successor and ADDITION. Full lambda-K
 * (e.g. Church multiplication, which leaves a cyclic net) additionally needs the
 * paper's non-interaction canonicalization rules and a global reduction order;
 * those are not implemented here, and such terms are reported as cyclic rather
 * than mis-normalised.
 *
 * Reuses the lambda term type and reference oracle from opt_core.h.
 */
#ifndef DELTANETS_H
#define DELTANETS_H

#include "opt_core.h"   /* lc_term, lc_var/lc_lam/lc_app, lc_nf_ref, lc_eq */

/* Translate `t` to a Delta-net, reduce with the core interaction rules, and read
 * back. Returns the normal form and sets *interactions on success; returns NULL
 * and sets *cyclic = 1 when read-back cannot safely linearise the net (a cycle,
 * residual sharing, or a reachable eraser -- i.e. the net is not canonical).
 *
 * SOUNDNESS. The result is GUARANTEED correct for linear (lambda-L) terms: their
 * reduction uses only fan annihilation, which is perfectly confluent, so order is
 * irrelevant and the net is always canonical (validated by a 260k-term random
 * differential test against the reference -- zero mismatches). For nonlinear
 * (lambda-A / lambda-K) terms the core lacks the leftmost-outermost order and the
 * canonicalization rules the paper requires for correctness; dn_normalize refuses
 * the non-canonical results it can detect, but a rare term may still reduce to a
 * structurally-valid yet wrong net that read-back cannot catch. Therefore a
 * non-NULL result is only relied upon as a normal form for linear inputs; the
 * specific lambda-K examples that are known-correct are verified individually in
 * tests/deltanets_test.c. The canonicalization layer that would make lambda-K
 * results guaranteed is documented in docs/INET_ENGINE_PLAN.md, section 7. */
lc_term *dn_normalize(const lc_term *t, long *interactions, int *cyclic);

#endif
