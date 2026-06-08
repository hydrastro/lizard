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
  IC_TERA,   /* *  — the eraser as a value          */
  IC_TPAIR,  /* (pair a b)  — Σ introduction        */
  IC_TFST,   /* (fst p)     — Σ first projection  π1 */
  IC_TSND,   /* (snd p)     — Σ second projection π2 */
  IC_TTRANSP,/* (transp v)  — transport / Id-by-observation (reflexive: identity) */
  IC_TREF    /* (ref D a)   — a call to recursive definition D with argument a;
              * the recursive cycle made explicit as a graph node (Phase: recursion) */
} ic_tkind_t;

/* Built-in recursive definitions reachable through IC_TREF.  Each is realised as
 * a self-referential graph: its body contains further (ref D ...) nodes, so the
 * "recursion is a cycle" rather than a finite tree.  The cycle is unfolded lazily
 * and terminates because the definition branches on its concrete argument (the
 * base case introduces no further ref), exactly as pattern-matching definitions
 * do in an interaction-net runtime such as HVM. */
enum {
  IC_DEF_FACT,    /* factorial:  fact n = n=0 ? 1 : n * fact (n-1)        */
  IC_DEF_SUMTO,   /* triangular: sumto n = n=0 ? 0 : n + sumto (n-1)      */
  IC_DEF_FIB,     /* fibonacci:  fib n = n<2 ? n : fib (n-1) + fib (n-2)  */
  IC_DEF_GCD,     /* gcd of (n encodes a fixed pair) — see build_def_body */
  IC_DEF_POW2     /* pow2 n = n=0 ? 1 : 2 * pow2 (n-1)                    */
};

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
ic_term_t *ic_pair(ic_term_t *fst, ic_term_t *snd);  /* (a, b) */
ic_term_t *ic_fst(ic_term_t *p);                     /* pi1 p  */
ic_term_t *ic_snd(ic_term_t *p);                     /* pi2 p  */
ic_term_t *ic_transp(ic_term_t *v);                  /* transport along refl = id */
ic_term_t *ic_ref(int def_id, ic_term_t *arg);       /* (ref D a): call recursive def D */
void ic_term_free(ic_term_t *t);

/* ---- evaluation -------------------------------------------------------- */

/* Reduce `t` to normal form on a fresh net and read back an integer.
 * Returns 1 and sets `out` (+ `*interactions` if non-NULL) on success;
 * 0 if the normal form is not a single integer; -1 on resource exhaustion.
 * `out` must be an initialised mpz_t. */
int ic_normalize_int(ic_term_t *t, mpz_t out, long *interactions);

/* Select the reduction order (default LIFO).  Setting FIFO makes the reducer
 * consume the redex queue front-to-back; the result and the interaction count
 * are invariant (strong confluence), which tests/ic_confluence_test.c checks. */
void ic_set_reduce_fifo(int on);

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

/* Render the compiled net (the abstract syntax graph) into buf, optionally after
 * reducing.  Each "nK" is an agent node; its [p1 p2] are wires to other nodes'
 * principal ports ("nK"), numbers ("#n"), or erasers ("*").  Variables are wires
 * and do not appear; a shared value is reached through a DUP node, so it is
 * represented once rather than duplicated.  Returns 1, 0 on small buffer,
 * -1 on exhaustion. */
int ic_dump_net(ic_term_t *t, char *buf, size_t cap, int reduce_first);

/* ---- a tiny textual front-end (so ic is a real engine, not just an API) -
 * Grammar (whitespace-separated, parens/braces as shown):
 *   term  := NUM | NAME | '*' | '(' app ')' | '{' [':' INT] term term '}'
 *          | '(' 'lam' NAME term ')'             ; abstraction
 *          | '(' 'op'  OP term term ')'          ; arithmetic, OP in + - * / %% = < >
 *          | '(' 'dup' [':' INT] NAME NAME term term ')' ; dup x y = v in body
 *          | '{' term term '}'                   ; superposition (label 0)
 *          | '{' ':' INT term term '}'           ; superposition with a label
 *          | '(' 'pair' term term ')'            ; Σ introduction (a, b)
 *          | '(' 'fst' term ')'                  ; Σ first projection  π1
 *          | '(' 'snd' term ')'                  ; Σ second projection π2
 *          | '(' 'transp' term ')'               ; transport / Id-by-observation
 *   app   := term term*                          ; left-associated application
 * Labels are optional and written ':'N (e.g. {:1 a b}, (dup :1 x y v body)); a
 * bare leading number is an ordinary element, so {2 3} is an unlabelled pair.
 * Reserved words (cannot be used as variable names): lam op dup pair fst snd transp.
 * Returns a heap term (free with ic_term_free) or NULL on parse error,
 * writing a short message to `err` (capacity `errcap`) when non-NULL. */
ic_term_t *ic_parse(const char *src, char *err, size_t errcap);

#endif /* LIZARD_IC_H */
