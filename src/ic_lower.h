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
 * Pairs and projections lower to first-class PAIR/FST/SND agents (Phase 14): a
 * pair is a constructor node, fst/snd are eliminators that fire against it
 * (FST~PAIR keeps the first field and erases the second; SND the reverse), and a
 * shared pair is duplicated by the ordinary DUP machinery (PAIR commutes past
 * DUP).  For differential testing, ic_lower_encoded lowers the same Σ forms by
 * the older constructor *encoding* instead -
 *     pair a b   ==>  \k. k a b ,   proj_i p ==> p (\a.\b. selector) ;
 * the two must agree on every term, which is how the first-class rules are
 * validated against the already-trusted CON beta.
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

/* Same lowering, but Sigma forms use the constructor *encoding* (\k.k a b /
 * p (\a.\b.sel)) rather than first-class agents.  Exists so the first-class
 * PAIR/FST/SND rules can be differentially cross-checked against CON beta.
 * Same return contract as ic_lower. */
ic_term_t *ic_lower_encoded(const core_t *t);

#endif /* LIZARD_IC_LOWER_H */
