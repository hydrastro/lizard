/* id_observe.c — see id_observe.h for the semantics this realises. */
#include "id_observe.h"
#include <stdlib.h>
#include <stdio.h>

/* ----------------------------------------------------------- constructors */
static id_node *mk(id_kind k) {
  id_node *t = (id_node *)malloc(sizeof *t);
  t->kind = k; t->idx = 0; t->a = NULL; t->b = NULL; t->c = NULL;
  return t;
}
id_node *id_base(id_kind k)                       { return mk(k); }
id_node *id_succ(id_node *n)                      { id_node *t = mk(ID_SUCC);  t->a = n; return t; }
id_node *id_var(int i)                            { id_node *t = mk(ID_VAR);   t->idx = i; return t; }
id_node *id_pair(id_node *x, id_node *y)          { id_node *t = mk(ID_PAIR);  t->a = x; t->b = y; return t; }
id_node *id_lam(id_node *body)                    { id_node *t = mk(ID_LAM);   t->a = body; return t; }
id_node *id_app(id_node *f, id_node *x)           { id_node *t = mk(ID_APP);   t->a = f; t->b = x; return t; }
id_node *id_prod(id_node *A, id_node *B)          { id_node *t = mk(ID_PROD);  t->a = A; t->b = B; return t; }
id_node *id_arr(id_node *A, id_node *B)           { id_node *t = mk(ID_ARR);   t->a = A; t->b = B; return t; }
id_node *id_pi(id_node *dom, id_node *body)       { id_node *t = mk(ID_PI);    t->a = dom; t->b = body; return t; }
id_node *id_idty(id_node *A, id_node *x, id_node *y) { id_node *t = mk(ID_ID); t->a = A; t->b = x; t->c = y; return t; }
id_node *id_equiv(id_node *A, id_node *B)         { id_node *t = mk(ID_EQUIV); t->a = A; t->b = B; return t; }
id_node *id_refl(id_node *x)                      { id_node *t = mk(ID_REFL);  t->a = x; return t; }
id_node *id_transp(id_node *P, id_node *p, id_node *x) { id_node *t = mk(ID_TRANSP); t->a = P; t->b = p; t->c = x; return t; }
id_node *id_if(id_node *c, id_node *th, id_node *el) { id_node *t = mk(ID_IF); t->a = c; t->b = th; t->c = el; return t; }
id_node *id_ua(id_node *f) { id_node *t = mk(ID_UA); t->a = f; return t; }

id_node *id_nat_lit(int n) {
  id_node *t = id_base(ID_ZERO);
  while (n-- > 0) t = id_succ(t);
  return t;
}

void id_free(id_node *t) {
  if (t == NULL) return;
  id_free(t->a); id_free(t->b); id_free(t->c);
  free(t);
}

/* --------------------------------------------------------- shift / copy / subst */
/* Return a fresh copy of t with every free de Bruijn index >= cutoff raised by d.
 * Binders: a LAM body and a PI body each sit one level deeper. */
static id_node *shift(const id_node *t, int d, int cutoff) {
  if (t == NULL) return NULL;
  switch (t->kind) {
    case ID_VAR:   return id_var(t->idx >= cutoff ? t->idx + d : t->idx);
    case ID_LAM:   return id_lam(shift(t->a, d, cutoff + 1));
    case ID_PI:    return id_pi(shift(t->a, d, cutoff), shift(t->b, d, cutoff + 1));
    case ID_SUCC:  return id_succ(shift(t->a, d, cutoff));
    case ID_REFL:  return id_refl(shift(t->a, d, cutoff));
    case ID_PAIR:  return id_pair(shift(t->a, d, cutoff), shift(t->b, d, cutoff));
    case ID_APP:   return id_app(shift(t->a, d, cutoff), shift(t->b, d, cutoff));
    case ID_PROD:  return id_prod(shift(t->a, d, cutoff), shift(t->b, d, cutoff));
    case ID_ARR:   return id_arr(shift(t->a, d, cutoff), shift(t->b, d, cutoff));
    case ID_EQUIV: return id_equiv(shift(t->a, d, cutoff), shift(t->b, d, cutoff));
    case ID_ID:    return id_idty(shift(t->a, d, cutoff), shift(t->b, d, cutoff), shift(t->c, d, cutoff));
    case ID_TRANSP:return id_transp(shift(t->a, d, cutoff), shift(t->b, d, cutoff), shift(t->c, d, cutoff));
    case ID_IF:    return id_if(shift(t->a, d, cutoff), shift(t->b, d, cutoff), shift(t->c, d, cutoff));
    case ID_UA:    return id_ua(shift(t->a, d, cutoff));
    case ID_BOOL: case ID_NAT: case ID_UNIT: case ID_EMPTY: case ID_U:
    case ID_TRUE: case ID_FALSE: case ID_ZERO: case ID_STAR:
      return mk(t->kind);
  }
  return mk(t->kind); /* unreachable */
}

id_node *id_copy(const id_node *t) { return shift(t, 0, 0); }

/* Instantiate the outermost binder of `body` with `arg`: replace de Bruijn
 * `depth` with arg (shifted past the binders crossed), and decrement the free
 * variables above it.  Used for beta:  (lam body) arg  -->  inst(body, arg). */
static id_node *inst(const id_node *body, int depth, const id_node *arg) {
  if (body == NULL) return NULL;
  switch (body->kind) {
    case ID_VAR:
      if (body->idx == depth) return shift(arg, depth, 0);
      return id_var(body->idx > depth ? body->idx - 1 : body->idx);
    case ID_LAM:   return id_lam(inst(body->a, depth + 1, arg));
    case ID_PI:    return id_pi(inst(body->a, depth, arg), inst(body->b, depth + 1, arg));
    case ID_SUCC:  return id_succ(inst(body->a, depth, arg));
    case ID_REFL:  return id_refl(inst(body->a, depth, arg));
    case ID_PAIR:  return id_pair(inst(body->a, depth, arg), inst(body->b, depth, arg));
    case ID_APP:   return id_app(inst(body->a, depth, arg), inst(body->b, depth, arg));
    case ID_PROD:  return id_prod(inst(body->a, depth, arg), inst(body->b, depth, arg));
    case ID_ARR:   return id_arr(inst(body->a, depth, arg), inst(body->b, depth, arg));
    case ID_EQUIV: return id_equiv(inst(body->a, depth, arg), inst(body->b, depth, arg));
    case ID_ID:    return id_idty(inst(body->a, depth, arg), inst(body->b, depth, arg), inst(body->c, depth, arg));
    case ID_TRANSP:return id_transp(inst(body->a, depth, arg), inst(body->b, depth, arg), inst(body->c, depth, arg));
    case ID_IF:    return id_if(inst(body->a, depth, arg), inst(body->b, depth, arg), inst(body->c, depth, arg));
    case ID_UA:    return id_ua(inst(body->a, depth, arg));
    case ID_BOOL: case ID_NAT: case ID_UNIT: case ID_EMPTY: case ID_U:
    case ID_TRUE: case ID_FALSE: case ID_ZERO: case ID_STAR:
      return mk(body->kind);
  }
  return mk(body->kind); /* unreachable */
}

/* ----------------------------------------------------- the by-observation rules */
static id_node *observe(const id_node *A, const id_node *x, const id_node *y);

/* Id Nat: recurse on the successor structure of the two numerals. */
static id_node *observe_nat(const id_node *x, const id_node *y) {
  int xz = (x->kind == ID_ZERO), yz = (y->kind == ID_ZERO);
  int xs = (x->kind == ID_SUCC), ys = (y->kind == ID_SUCC);
  if (xz && yz) return id_base(ID_UNIT);
  if (xz && ys) return id_base(ID_EMPTY);
  if (xs && yz) return id_base(ID_EMPTY);
  if (xs && ys) return observe_nat(x->a, y->a);     /* succ m = succ n  ~>  m = n */
  return id_idty(id_base(ID_NAT), id_copy(x), id_copy(y));  /* neutral (not a numeral) */
}

/* A, x, y are already in normal form; return the observed reduction (fresh). */
static id_node *observe(const id_node *A, const id_node *x, const id_node *y) {
  switch (A->kind) {
    case ID_BOOL: {
      int bx = (x->kind == ID_TRUE) ? 1 : (x->kind == ID_FALSE) ? 0 : -1;
      int by = (y->kind == ID_TRUE) ? 1 : (y->kind == ID_FALSE) ? 0 : -1;
      if (bx >= 0 && by >= 0) return id_base(bx == by ? ID_UNIT : ID_EMPTY);
      return id_idty(id_copy(A), id_copy(x), id_copy(y));
    }
    case ID_NAT:   return observe_nat(x, y);
    case ID_UNIT:  return id_base(ID_UNIT);                 /* contractible: all equal   */
    case ID_EMPTY: return id_base(ID_UNIT);                 /* vacuous: no elements       */
    case ID_PROD:                                           /* componentwise */
      if (x->kind == ID_PAIR && y->kind == ID_PAIR)
        return id_prod(observe(A->a, x->a, y->a), observe(A->b, x->b, y->b));
      return id_idty(id_copy(A), id_copy(x), id_copy(y));
    case ID_ARR: {                                          /* funext: Pi z:dom. Id cod (x z)(y z) */
      id_node *body = id_idty(shift(A->b, 1, 0),
                              id_app(shift(x, 1, 0), id_var(0)),
                              id_app(shift(y, 1, 0), id_var(0)));
      id_node *bnf = id_nf(body);
      id_free(body);
      return id_pi(id_copy(A->a), bnf);
    }
    case ID_U:     return id_equiv(id_copy(x), id_copy(y)); /* univalence: paths in U = equivalences */
    case ID_PI: case ID_ID: case ID_EQUIV:
    default:
      return id_idty(id_copy(A), id_copy(x), id_copy(y));   /* neutral / out of scope */
  }
}

/* ---------------------------------------------------------------- normaliser */
/* does the de Bruijn variable `lvl` occur free in t?  Used to detect a constant
 * type family: a family `lam X. body` whose body never mentions X, in which case
 * transport in that family is the identity regardless of the path. */
static int occurs(const id_node *t, int lvl) {
  if (t == NULL) return 0;
  switch (t->kind) {
    case ID_VAR: return t->idx == lvl;
    case ID_LAM: return occurs(t->a, lvl + 1);
    case ID_PI:  return occurs(t->a, lvl) || occurs(t->b, lvl + 1);
    default:     return occurs(t->a, lvl) || occurs(t->b, lvl) || occurs(t->c, lvl);
  }
}

id_node *id_nf(const id_node *t) {
  if (t == NULL) return NULL;
  switch (t->kind) {
    case ID_SUCC:  return id_succ(id_nf(t->a));
    case ID_REFL:  return id_refl(id_nf(t->a));
    case ID_PAIR:  return id_pair(id_nf(t->a), id_nf(t->b));
    case ID_LAM:   return id_lam(id_nf(t->a));
    case ID_PI:    return id_pi(id_nf(t->a), id_nf(t->b));
    case ID_PROD:  return id_prod(id_nf(t->a), id_nf(t->b));
    case ID_ARR:   return id_arr(id_nf(t->a), id_nf(t->b));
    case ID_EQUIV: return id_equiv(id_nf(t->a), id_nf(t->b));
    case ID_APP: {
      id_node *f = id_nf(t->a), *x = id_nf(t->b), *r;
      if (f->kind == ID_LAM) {
        id_node *ins = inst(f->a, 0, x);
        r = id_nf(ins);
        id_free(ins); id_free(f); id_free(x);
        return r;
      }
      return id_app(f, x);                                  /* neutral application */
    }
    case ID_TRANSP: {
      id_node *P = id_nf(t->a), *p = id_nf(t->b), *x = id_nf(t->c);
      /* transport along refl is the identity, for ANY family */
      if (p->kind == ID_REFL) { id_free(P); id_free(p); return x; }
      if (P->kind == ID_LAM) {
        /* constant family (the argument is unused): transport is the identity
         * for any path -- this is why transport never intrudes in the simply-
         * typed fragment, and why funext etc. need no transport corrections. */
        if (!occurs(P->a, 0)) { id_free(P); id_free(p); return x; }
        /* identity family  (lam X. X)  over the universe: this is the genuinely
         * univalent case.  transport^(lam X.X) (ua f) x  =  f x. */
        if (P->a->kind == ID_VAR && P->a->idx == 0 && p->kind == ID_UA) {
          id_node *ap = id_app(id_copy(p->a), x);   /* x's ownership moves into ap */
          id_node *r = id_nf(ap);
          id_free(ap); id_free(P); id_free(p);
          return r;
        }
      }
      return id_transp(P, p, x);   /* structural family cases: documented extension */
    }
    case ID_IF: {
      id_node *c = id_nf(t->a), *th = id_nf(t->b), *el = id_nf(t->c);
      if (c->kind == ID_TRUE)  { id_free(c); id_free(el); return th; }
      if (c->kind == ID_FALSE) { id_free(c); id_free(th); return el; }
      return id_if(c, th, el);     /* neutral */
    }
    case ID_UA: return id_ua(id_nf(t->a));
    case ID_ID: {
      id_node *A = id_nf(t->a), *x = id_nf(t->b), *y = id_nf(t->c), *r;
      r = observe(A, x, y);
      id_free(A); id_free(x); id_free(y);
      return r;
    }
    case ID_VAR:
    case ID_BOOL: case ID_NAT: case ID_UNIT: case ID_EMPTY: case ID_U:
    case ID_TRUE: case ID_FALSE: case ID_ZERO: case ID_STAR:
      return id_copy(t);
  }
  return id_copy(t); /* unreachable */
}

/* ------------------------------------------------------ structural equality */
int id_eq(const id_node *x, const id_node *y) {
  if (x == NULL || y == NULL) return x == y;
  if (x->kind != y->kind) return 0;
  if (x->kind == ID_VAR) return x->idx == y->idx;
  return id_eq(x->a, y->a) && id_eq(x->b, y->b) && id_eq(x->c, y->c);
}

/* --------------------------------------------------------------- diagnostics */
void id_print(const id_node *t) {
  if (t == NULL) { printf("<null>"); return; }
  switch (t->kind) {
    case ID_BOOL:  printf("Bool");  break;
    case ID_NAT:   printf("Nat");   break;
    case ID_UNIT:  printf("Unit");  break;
    case ID_EMPTY: printf("Empty"); break;
    case ID_U:     printf("U");     break;
    case ID_TRUE:  printf("true");  break;
    case ID_FALSE: printf("false"); break;
    case ID_ZERO:  printf("0");     break;
    case ID_STAR:  printf("*");     break;
    case ID_VAR:   printf("#%d", t->idx); break;
    case ID_SUCC:  printf("succ "); id_print(t->a); break;
    case ID_REFL:  printf("refl "); id_print(t->a); break;
    case ID_LAM:   printf("(lam "); id_print(t->a); printf(")"); break;
    case ID_PAIR:  printf("("); id_print(t->a); printf(", "); id_print(t->b); printf(")"); break;
    case ID_APP:   printf("("); id_print(t->a); printf(" "); id_print(t->b); printf(")"); break;
    case ID_PROD:  printf("("); id_print(t->a); printf(" * "); id_print(t->b); printf(")"); break;
    case ID_ARR:   printf("("); id_print(t->a); printf(" -> "); id_print(t->b); printf(")"); break;
    case ID_PI:    printf("(Pi "); id_print(t->a); printf(". "); id_print(t->b); printf(")"); break;
    case ID_EQUIV: printf("Equiv "); id_print(t->a); printf(" "); id_print(t->b); break;
    case ID_ID:    printf("Id "); id_print(t->a); printf(" "); id_print(t->b); printf(" "); id_print(t->c); break;
    case ID_TRANSP:printf("transport "); id_print(t->a); printf(" "); id_print(t->b); printf(" "); id_print(t->c); break;
    case ID_IF:    printf("(if "); id_print(t->a); printf(" "); id_print(t->b); printf(" "); id_print(t->c); printf(")"); break;
    case ID_UA:    printf("(ua "); id_print(t->a); printf(")"); break;
  }
}
