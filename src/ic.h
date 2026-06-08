#ifndef LIZARD_IC_H
#define LIZARD_IC_H

/* ------------------------------------------------------------------------
 * ic — the interaction-calculus core (Phase 11: the FOUR agents).
 *
 * This is the successor to inet.c.  inet.c implements a correct Lafont
 * interaction-combinator runtime, but only three of the four 3-arrow agents
 * the design calls for: CON (lambda / application), DUP (duplication), and the
 * numeric agents — it has no SUPERPOSITION agent, and no way to express the
 * construction/observation duality at the value level.
 *
 * ic.c closes that gap.  The agent set is exactly the one in the design note:
 *
 *     nulls    (0 aux)  : ERA            — the eraser/terminal
 *     numbers  (0 aux)  : NUM            — an exact GMP integer
 *     the four 3-arrow agents (1 principal + 2 aux), labelled:
 *                         CON  (lambda / application)
 *                         DUP  (duplication        — the "construction" reading)
 *                         SUP  (superposition      — the "observation" reading)
 *                         OPR/OP1 (binary numeric operators)
 *
 * DUP and SUP are the *same* combinator read in the two dual directions, which
 * is exactly the directional, non-symmetric duality at the heart of the design
 * (construction implies observation; checking implies searching; P sits inside
 * NP).  They are distinguished only by a label, and the whole computational
 * content of the duality is one line in the rule table:
 *
 *     DUP{L} ~ SUP{L}   (same label)   -> ANNIHILATE  (the two superposed values
 *                                        flow straight to the two consumers —
 *                                        the search collapses to a pair)
 *     DUP{L} ~ SUP{K}   (L != K)       -> COMMUTE     (the alternatives cross and
 *                                        multiply — the search fans out)
 *
 * Everything else is the standard interaction-combinator table:
 *   CON{a} ~ CON{a}  -> annihilate (this is beta when one CON is a lambda and the
 *                       other an application);
 *   same family + same label -> annihilate, otherwise -> commute;
 *   ERA erases; NUM ~ OPR/OP1 computes exact arithmetic; NUM ~ DUP/SUP copies a
 *   number into both wires.
 *
 * Two innovations relative to HVM2 are inherited from inet.c: numbers are exact
 * GMP integers (not 24-bit), and the reducer counts interactions so that
 * sharing (DUP) and search (SUP) can be measured empirically.
 *
 * The module depends only on GMP and the C runtime, so it builds and is tested
 * in isolation from the rest of lizard while the engine migration proceeds.
 * ------------------------------------------------------------------------ */

#include <gmp.h>
#include <stddef.h>

/* ---- surface terms (named variables) ---------------------------------- */
typedef enum {
  IC_TVAR,   /* x                                   */
  IC_TLAM,   /* (lambda (x) body)                   */
  IC_TAPP,   /* (f a)                               */
  IC_TNUM,   /* an exact integer literal            */
  IC_TOP,    /* (op lhs rhs)                        */
  IC_TSUP,   /* {a b}^L  — a superposition, label L */
  IC_TDUP,   /* dup^L x y = v in body  — a let-style splitter, label L */
  IC_TERA    /* *  — the eraser as a value          */
} ic_tkind_t;

/* binary numeric operators (same codes as inet) */
enum {
  IC_ADD, IC_SUB, IC_MUL, IC_DIV, IC_MOD,
  IC_EQ,  IC_LT,  IC_GT
};

typedef struct ic_term {
  ic_tkind_t kind;
  char *name;             /* TVAR/TLAM binder; TDUP: name (left); see name2 */
  char *name2;            /* TDUP: right binder name                       */
  struct ic_term *a;      /* TLAM body / TAPP fun / TOP lhs / TSUP left / TDUP value */
  struct ic_term *b;      /* TAPP arg / TOP rhs / TSUP right / TDUP body    */
  int op;                 /* TOP operator                                  */
  int label;              /* TSUP / TDUP label (user-visible, small >= 0)  */
  mpz_t num;              /* TNUM value (init iff kind == IC_TNUM)         */
} ic_term_t;

/* constructors (heap-allocated; free with ic_term_free) */
ic_term_t *ic_var(const char *name);
ic_term_t *ic_lam(const char *name, ic_term_t *body);
ic_term_t *ic_app(ic_term_t *f, ic_term_t *a);
ic_term_t *ic_num_si(long v);
ic_term_t *ic_num_str(const char *decimal);
ic_term_t *ic_num_z(const mpz_t v);
ic_term_t *ic_op(int op, ic_term_t *l, ic_term_t *r);
ic_term_t *ic_sup(int label, ic_term_t *l, ic_term_t *r);
ic_term_t *ic_dup(int label, const char *x, const char *y,
                  ic_term_t *value, ic_term_t *body);
ic_term_t *ic_era(void);
void ic_term_free(ic_term_t *t);

/* ---- evaluation -------------------------------------------------------- */

/* Reduce `t` to normal form on a fresh net and read back an integer.
 * Returns 1 and sets `out` (+ `*interactions` if non-NULL) on success;
 * 0 if the normal form is not a single integer; -1 on resource exhaustion.
 * `out` must be an initialised mpz_t. */
int ic_normalize_int(ic_term_t *t, mpz_t out, long *interactions);

/* Reduce `t` to normal form and write a textual rendering into `buf`.
 *   integers           -> decimal
 *   superpositions     -> "{a b}"  (optionally "{L: a b}" if labelled)
 *   abstractions/apps   in the affine fragment (each binder used <= 1 time,
 *                        so the normal form has no residual DUP) ->
 *                        "(lam body)", "(f a)", "#k" de Bruijn variables
 *   anything that still carries DUP-sharing in normal form ->
 *                        a faithful net rendering "<net: ...>" (never a wrong
 *                        tree — see note in ic.c).
 * Returns 1 on success, 0 if the buffer was too small, -1 on exhaustion. */
int ic_readback(ic_term_t *t, char *buf, size_t cap, long *interactions);

/* Live agent nodes after the last reduction (for tests / stats). */
long ic_live_nodes(void);

/* ---- a tiny textual front-end (so ic is a real engine, not just an API) -
 * Grammar (whitespace-separated, parens/braces as shown):
 *   term  := NUM | NAME | '*' | '(' app ')' | '{' [':' INT] term term '}'
 *          | '(' 'lam' NAME term ')'             ; abstraction
 *          | '(' 'op'  OP term term ')'          ; arithmetic, OP in + - * / %% = < >
 *          | '(' 'dup' [':' INT] NAME NAME term term ')' ; dup x y = v in body
 *          | '{' term term '}'                   ; superposition (label 0)
 *          | '{' ':' INT term term '}'           ; superposition with a label
 *   app   := term term*                          ; left-associated application
 * Labels are optional and written ':'N (e.g. {:1 a b}, (dup :1 x y v body)); a
 * bare leading number is an ordinary element, so {2 3} is an unlabelled pair.
 * Returns a heap term (free with ic_term_free) or NULL on parse error,
 * writing a short message to `err` (capacity `errcap`) when non-NULL. */
ic_term_t *ic_parse(const char *src, char *err, size_t errcap);

#endif /* LIZARD_IC_H */
