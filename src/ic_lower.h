#ifndef LIZARD_IC_LOWER_H
#define LIZARD_IC_LOWER_H

/* ------------------------------------------------------------------------
 * ic_lower — lowering elaborator output to interaction nets (Phase 12).
 *
 * The trusted IR is `kterm_t` (src/kernel.h): a dependent type theory with
 * VAR/LAM/APP, SIGMA/PAIR/PROJ1/PROJ2, NAT, ID, and so on; `core_term`
 * (src/core_term.h) wraps either one of those kernel terms or a *runtime* AST.
 *
 * This module lowers the computational fragment of that IR onto the ic engine.
 * It is the first concrete piece of "compile core_term to a net": it takes a
 * `core_t` term (a standalone mirror of the kterm computational fragment, in de
 * Bruijn form, so it can be exercised without the elaborator while <ds.h> keeps
 * the full library from linking) and produces a named `ic_term_t`, which the
 * existing, already-validated ic machinery reduces and reads back.
 *
 * Two fragments are covered:
 *   - the negative + Sigma core   — VAR, LAM, APP, PAIR, PROJ1, PROJ2, LET;
 *   - the runtime fragment        — LIT (exact GMP integer) and PRIM (the eight
 *                                   ic operators), i.e. core_term's
 *                                   LIZARD_CORE_RUNTIME_AST path.
 *
 * Pairs and projections are lowered by the standard constructor encoding —
 *     pair a b   ==>  \k. k a b
 *     proj1 p    ==>  p (\a. \b. a)
 *     proj2 p    ==>  p (\a. \b. b)
 * so no new agents are needed yet; the engine's CON beta does the work, and a
 * shared pair is duplicated by the same DUP machinery as any other value.
 * First-class PAIR/FST/SND agents are deferred to Phase 14, where the Id/transport
 * agent needs to dispatch on the Sigma former structurally (Id over Sigma =
 * componentwise).  See docs/INET_ENGINE_PLAN.md.
 * ------------------------------------------------------------------------ */

#include <gmp.h>
#include "ic.h"   /* ic_term_t, the op codes IC_ADD..IC_GT, ic_* constructors */

typedef enum {
  CT_VAR,    /* de Bruijn index (0 = innermost binder)        */
  CT_LAM,    /* \. body                                       */
  CT_APP,    /* f a                                           */
  CT_PAIR,   /* (a, b)                                         */
  CT_PROJ1,  /* pi1 p                                          */
  CT_PROJ2,  /* pi2 p                                          */
  CT_LET,    /* let _ = value in body  (body binds de Bruijn 0)*/
  CT_LIT,    /* an exact integer (runtime fragment)           */
  CT_PRIM    /* op a b, op in IC_ADD..IC_GT (runtime fragment) */
} core_kind_t;

typedef struct core_term {
  core_kind_t kind;
  int idx;                 /* CT_VAR de Bruijn index            */
  int op;                  /* CT_PRIM operator                  */
  struct core_term *a;     /* LAM body / APP fun / PROJ target / PAIR fst / LET value / PRIM lhs */
  struct core_term *b;     /* APP arg / PAIR snd / LET body / PRIM rhs */
  mpz_t num;               /* CT_LIT value (init iff kind == CT_LIT) */
} core_t;

/* constructors (heap-allocated; free the whole tree with core_free) */
core_t *core_var(int debruijn_index);
core_t *core_lam(core_t *body);
core_t *core_app(core_t *fun, core_t *arg);
core_t *core_pair(core_t *fst, core_t *snd);
core_t *core_proj1(core_t *p);
core_t *core_proj2(core_t *p);
core_t *core_let(core_t *value, core_t *body);
core_t *core_lit_si(long v);
core_t *core_lit_z(const mpz_t v);
core_t *core_prim(int op, core_t *l, core_t *r);
void    core_free(core_t *t);

/* Lower a core term to a named ic term.  Reduce it with ic_normalize_int /
 * ic_readback exactly as for any other ic term.  Returns a heap ic_term_t
 * (free with ic_term_free) or NULL on malformed input / depth overflow. */
ic_term_t *ic_lower(const core_t *t);

#endif /* LIZARD_IC_LOWER_H */
