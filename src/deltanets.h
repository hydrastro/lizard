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

/* Translate `t` to a Delta-net, reduce with the core interaction rules, read
 * back. On success returns the normal form and sets *interactions. If read-back
 * meets a cyclic net (a term outside the core's reach, needing canonicalization)
 * returns NULL and sets *cyclic to 1. */
lc_term *dn_normalize(const lc_term *t, long *interactions, int *cyclic);

#endif
