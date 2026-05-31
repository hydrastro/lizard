#ifndef LIZARD_INET_H
#define LIZARD_INET_H

/* ------------------------------------------------------------------------
 * inet — an interaction-combinator runtime (Phase 10: the syntax GRAPH).
 *
 * Terms are represented not as a tree but as an interaction net: a graph of
 * agents wired port-to-port, with explicit sharing nodes (DUP) and erasers
 * (ERA).  Computation is local graph rewriting on "active pairs" (two agents
 * whose principal ports are wired together).  Two rules suffice for the pure
 * fragment — annihilation (same agent) and commutation (different agents) —
 * plus erasure; numeric agents add exact arithmetic.
 *
 * This is the model behind interaction combinators (Lafont) and the HVM /
 * HigherOrderCO runtime (Taelin).  Sharing makes the reduction Levy-optimal:
 * a duplicated function is reduced once, not once per use.  Two innovations
 * here relative to HVM2: numbers are exact GMP integers (not 24-bit), and the
 * reducer counts interactions so the sharing can be demonstrated empirically.
 *
 * A small lambda-calculus front-end compiles named-variable terms to nets;
 * `inet_normalize_int` reduces a term to normal form and reads back an integer
 * result (the observable used for differential testing against the
 * interpreter).  This module depends only on GMP and the C runtime.
 * ------------------------------------------------------------------------ */

#include <gmp.h>

/* ---- surface lambda terms (named variables) --------------------------- */
typedef enum {
  INET_TVAR,   /* x                          */
  INET_TLAM,   /* (lambda (x) body)          */
  INET_TAPP,   /* (f a)                      */
  INET_TNUM,   /* an exact integer literal   */
  INET_TOP     /* (op lhs rhs)               */
} inet_tkind_t;

/* binary numeric operators */
enum {
  INET_ADD, INET_SUB, INET_MUL, INET_DIV, INET_MOD,
  INET_EQ,  INET_LT,  INET_GT
};

typedef struct inet_term {
  inet_tkind_t kind;
  char *name;                /* TVAR / TLAM binder name (owned copy)        */
  struct inet_term *a;       /* TLAM body / TAPP fun / TOP lhs                */
  struct inet_term *b;       /* TAPP arg / TOP rhs                            */
  int op;                    /* TOP operator                                 */
  mpz_t num;                 /* TNUM value (initialised iff kind == TNUM)    */
} inet_term_t;

/* term constructors (heap-allocated; free with inet_term_free) */
inet_term_t *inet_var(const char *name);
inet_term_t *inet_lam(const char *name, inet_term_t *body);
inet_term_t *inet_app(inet_term_t *f, inet_term_t *a);
inet_term_t *inet_num_si(long v);
inet_term_t *inet_num_str(const char *decimal);
inet_term_t *inet_num_z(const mpz_t v);
inet_term_t *inet_op(int op, inet_term_t *l, inet_term_t *r);
void inet_term_free(inet_term_t *t);

/* ---- evaluation -------------------------------------------------------- */

/* Reduce `t` to normal form on a fresh net and read back an integer.
 * On success returns 1, sets `out` to the result and (if non-NULL)
 * `*interactions` to the number of graph rewrites performed.
 * Returns 0 if the term does not reduce to an integer (e.g. it is a lambda),
 * or -1 on resource exhaustion.  `out` must be an initialised mpz_t. */
int inet_normalize_int(inet_term_t *t, mpz_t out, long *interactions);

/* Reduce `t` to normal form and read the result back as a lambda term written
 * in de Bruijn notation: variables "#k" (0 = innermost binder), applications
 * "(f a)", abstractions "(lam body)", and integers in decimal.  Writes a
 * NUL-terminated string into `buf` (capacity `cap`).  Returns 1 on success,
 * 0 if the normal form is not representable (residual operator, or the buffer
 * was too small), or -1 on resource exhaustion.  Correct for closed normal
 * forms in the stratified fragment compiled by the front-end. */
int inet_readback(inet_term_t *t, char *buf, size_t cap, long *interactions);

/* Number of agent nodes still live after the last reduction (for tests). */
long inet_live_nodes(void);

#endif /* LIZARD_INET_H */
