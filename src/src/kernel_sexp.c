/* src/kernel_sexp.c -- Lisp datum to kernel-term conversion.
 *
 * Split out of prims_kernel.c as part of Recoverable Core Phase 1M.
 */

#include "kernel_sexp.h"
#include "mem.h"
#include "parser.h"

#include <string.h>

/* Convert the n-th argument (0-based, after the head symbol) of an
 * application's argument list to a kernel term.  Returns NULL if the
 * argument is missing or unconvertible. */
static kterm_t *sexp_nth_kterm(lizard_heap_t *heap, lz_list_t *parts, int n) {
  lz_list_node_t *it;
  if (parts == NULL) return NULL;
  it = parts->head->next; /* skip the head symbol */
  while (n-- > 0 && it != parts->nil) it = it->next;
  if (it == parts->nil) return NULL;
  return lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)it)->ast);
}

/* Helper: convert Lisp S-expression to kernel term. */
kterm_t *lizard_kernel_sexp_to_kterm(lizard_heap_t *heap, lizard_ast_node_t *e) {
  if (e == NULL) return NULL;

  if (e->type == AST_PAIR) {
    lizard_ast_node_t *reparsed;
    reparsed = lizard_reparse_datum(e, heap);
    if (reparsed != NULL && reparsed != e) {
      return lizard_kernel_sexp_to_kterm(heap, reparsed);
    }
  }
  if (e->type == AST_NUMBER) {
    /* Build a Nat literal: 0 → kt_zero, n → succ^n(zero) */
    long n = mpz_get_si(e->data.number);
    kterm_t *t = kt_zero(heap);
    long i;
    for (i = 0; i < n; i++) t = kt_succ(heap, t);
    return t;
  }
  if (e->type == AST_SYMBOL) {
    const char *s = e->data.variable;
    if (strcmp(s, "Nat") == 0) return kt_nat(heap);
    if (strcmp(s, "zero") == 0) return kt_zero(heap);
      if (strcmp(s, "Bool") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_BOOL; return b;
      }
      if (strcmp(s, "true") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_TRUE; return b;
      }
      if (strcmp(s, "false") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_FALSE; return b;
      }
      if (strcmp(s, "Unit") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_UNIT; return b;
      }
      if (strcmp(s, "*") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_STAR; return b;
      }
      if (strcmp(s, "nil") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_NIL_K; return b;
      }
      if (strcmp(s, "nothing") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_NOTHING; return b;
      }
      if (strcmp(s, "Empty") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_EMPTY; return b;
      }
    /* de Bruijn variable: #0, #1, etc. */
    if (s[0] == '#' && s[1] >= '0' && s[1] <= '9') {
      return kt_var(heap, s[1] - '0');
    }
    /* metavariable: ?0, ?1, etc. */
    if (s[0] == '?' && s[1] >= '0' && s[1] <= '9') {
      kterm_t *m = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
      memset(m, 0, sizeof(*m));
      m->tag = KT_META;
      m->data.meta.id = s[1] - '0';
      return m;
    }
    return NULL;
  }
  if (e->type == AST_PAIR || e->type == AST_APPLICATION) {
    /* Parse (Sort n), (succ e), (Pi (x A) B), (lam (x A) body),
     * (app f a), (Id A a b), (refl a) */
    lz_list_t *parts;
    lizard_ast_node_t *head;
    if (e->type == AST_APPLICATION) {
      parts = e->data.application_arguments;
      if (parts == NULL || parts->head == parts->nil) return NULL;
      head = ((lizard_ast_list_node_t *)parts->head)->ast;
    } else {
      head = e->data.pair.car;
      parts = NULL;
    }
    if (head->type == AST_SYMBOL) {
      const char *name = head->data.variable;
      if (strcmp(name, "Sort") == 0 && parts != NULL) {
        lizard_ast_node_t *lvl = ((lizard_ast_list_node_t *)parts->head->next)->ast;
        if (lvl->type == AST_NUMBER) {
          return kt_sort(heap, (int)mpz_get_si(lvl->data.number));
        }
      }
      if (strcmp(name, "succ") == 0 && parts != NULL) {
        kterm_t *inner = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        return inner ? kt_succ(heap, inner) : NULL;
      }
      /* (List A) */
      if (strcmp(name, "List") == 0 && parts != NULL) {
        kterm_t *et = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (et) {
          kterm_t *l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(l, 0, sizeof(*l));
          l->tag = KT_LIST;
          l->data.list.elem_type = et;
          return l;
        }
        return NULL;
      }
      /* (cons h t) */
      if (strcmp(name, "cons") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *h, *t;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        h = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        t = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (h && t) {
          kterm_t *c = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(c, 0, sizeof(*c));
          c->tag = KT_CONS_K;
          c->data.cons_k.head = h;
          c->data.cons_k.tail = t;
          return c;
        }
        return NULL;
      }
      if (strcmp(name, "refl") == 0 && parts != NULL) {
        kterm_t *inner = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        return inner ? kt_refl(heap, inner) : NULL;
      }
      /* (Id A a b) */
      if (strcmp(name, "Id") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3;
        kterm_t *ta, *ka, *kb;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        n3 = n2->next;
        if (n3 == parts->nil) return NULL;
        ta = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        ka = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        kb = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        if (ta && ka && kb) return kt_id(heap, ta, ka, kb);
        return NULL;
      }
      /* (Pi (name domain) codomain) */
      if (strcmp(name, "Pi") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *domain, *codomain;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              domain = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            } else { return NULL; }
          } else { return NULL; }
        } else { return NULL; }
        codomain = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (domain && codomain) return kt_pi(heap, pname, domain, codomain);
        return NULL;
      }
      /* (Sigma (name type) body) — same binder parsing as Pi. */
      if (strcmp(name, "Sigma") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *fst_type = NULL, *snd_type;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              fst_type = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            }
          }
        }
        if (fst_type == NULL) return NULL;
        snd_type = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (snd_type == NULL) return NULL;
        {
          kterm_t *sig = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(sig, 0, sizeof(*sig));
          sig->tag = KT_SIGMA;
          sig->data.sigma.name = pname;
          sig->data.sigma.fst_type = fst_type;
          sig->data.sigma.snd_type = snd_type;
          return sig;
        }
      }
      /* (pair a b) */
      if (strcmp(name, "pair") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *a, *b;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        a = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        b = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (a && b) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PAIR;
          p->data.pair.fst = a;
          p->data.pair.snd = b;
          return p;
        }
        return NULL;
      }
      /* (fst e) */
      if (strcmp(name, "fst") == 0 && parts != NULL) {
        kterm_t *inner = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (inner) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PROJ1;
          p->data.proj.target = inner;
          return p;
        }
        return NULL;
      }
      /* (snd e) */
      if (strcmp(name, "snd") == 0 && parts != NULL) {
        kterm_t *inner = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (inner) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PROJ2;
          p->data.proj.target = inner;
          return p;
        }
        return NULL;
      }
      /* (lam (x A) body) */
      if (strcmp(name, "lam") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *domain = NULL, *body;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              domain = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            }
          }
        }
        if (domain == NULL) return NULL;
        body = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (body == NULL) return NULL;
        return kt_lam(heap, pname, domain, body);
      }
      /* (app f a) — explicit application. */
      if (strcmp(name, "app") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *f, *a;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        f = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        a = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (f && a) return kt_app(heap, f, a);
        return NULL;
      }
      /* (natrec motive zero-case succ-case scrutinee) */
      if (strcmp(name, "natrec") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3, *n4;
        kterm_t *mot, *zc, *sc, *scrut;
        kterm_t *nr;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        n4 = n3->next; if (n4 == parts->nil) return NULL;
        mot   = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        zc    = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        sc    = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        scrut = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n4)->ast);
        if (!mot || !zc || !sc || !scrut) return NULL;
        nr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(nr, 0, sizeof(*nr));
        nr->tag = KT_NAT_REC;
        nr->data.nat_rec.motive = mot;
        nr->data.nat_rec.zero_case = zc;
        nr->data.nat_rec.succ_case = sc;
        nr->data.nat_rec.scrutinee = scrut;
        return nr;
      }
      /* (Maybe A) */
      if (strcmp(name, "Maybe") == 0 && parts != NULL) {
        kterm_t *et = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (et) {
          kterm_t *m = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(m, 0, sizeof(*m));
          m->tag = KT_MAYBE;
          m->data.maybe.elem_type = et;
          return m;
        }
        return NULL;
      }
      /* (just v) */
      if (strcmp(name, "just") == 0 && parts != NULL) {
        kterm_t *v = lizard_kernel_sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (v) {
          kterm_t *j = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(j, 0, sizeof(*j));
          j->tag = KT_JUST;
          j->data.just.value = v;
          return j;
        }
        return NULL;
      }
      /* (Sum A B) */
      if (strcmp(name, "Sum") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *a, *b, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        a = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        b = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (!a || !b) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_SUM_K;
        s->data.sum_k.left_type = a;
        s->data.sum_k.right_type = b;
        return s;
      }
      /* (inl v B) — left injection */
      if (strcmp(name, "inl") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *v, *rt, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        v = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        rt = (n2 != parts->nil) ? lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast) : NULL;
        if (!v) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_INL;
        s->data.inl.value = v;
        s->data.inl.right_type = rt;
        return s;
      }
      /* (inr v A) — right injection */
      if (strcmp(name, "inr") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *v, *lt, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        v = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        lt = (n2 != parts->nil) ? lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast) : NULL;
        if (!v) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_INR;
        s->data.inr.value = v;
        s->data.inr.left_type = lt;
        return s;
      }
      /* (if scrutinee true-case false-case) — Bool eliminator. */
      if (strcmp(name, "if") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3;
        kterm_t *scrut, *tc, *fc, *br;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        scrut = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        tc    = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        fc    = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        if (!scrut || !tc || !fc) return NULL;
        br = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(br, 0, sizeof(*br));
        br->tag = KT_BOOL_REC;
        br->data.bool_rec.motive = NULL;
        br->data.bool_rec.true_case = tc;
        br->data.bool_rec.false_case = fc;
        br->data.bool_rec.scrutinee = scrut;
        return br;
      }
      /* (J motive base-case type a b proof) */
      if (strcmp(name, "J") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3, *n4, *n5, *n6;
        kterm_t *mot, *bc, *ty, *a, *b, *pf;
        kterm_t *j;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        n4 = n3->next; if (n4 == parts->nil) return NULL;
        n5 = n4->next; if (n5 == parts->nil) return NULL;
        n6 = n5->next; if (n6 == parts->nil) return NULL;
        mot = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        bc  = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        ty  = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        a   = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n4)->ast);
        b   = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n5)->ast);
        pf  = lizard_kernel_sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n6)->ast);
        if (!mot || !bc || !ty || !a || !b || !pf) return NULL;
        j = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(j, 0, sizeof(*j));
        j->tag = KT_J;
        j->data.j.motive = mot;
        j->data.j.base_case = bc;
        j->data.j.type = ty;
        j->data.j.a = a;
        j->data.j.b = b;
        j->data.j.proof = pf;
        return j;
      }
      /* (var N) — de Bruijn variable, reader-safe (the bare #N reader
       * syntax is consumed as a boolean, so applications use this form). */
      if (strcmp(name, "var") == 0) {
        lz_list_node_t *n1 = parts->head->next;
        lizard_ast_node_t *idx_node;
        if (n1 == parts->nil) return NULL;
        idx_node = ((lizard_ast_list_node_t *)n1)->ast;
        if (idx_node != NULL && idx_node->type == AST_NUMBER) {
          return kt_var(heap, (int)mpz_get_si(idx_node->data.number));
        }
        return NULL;
      }
      /* (nil A) — empty list with an element-type annotation, so list
       * eliminations over it can be kernel-typed. */
      if (strcmp(name, "nil") == 0) {
        kterm_t *et = sexp_nth_kterm(heap, parts, 0);
        kterm_t *nl;
        if (!et) return NULL;
        nl = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(nl, 0, sizeof(*nl));
        nl->tag = KT_NIL_K;
        nl->data.nil_k.elem_type = et;
        return nl;
      }
      /* (boolrec motive true-case false-case scrutinee) — dependent Bool
       * eliminator.  Unlike `if`, this carries a motive so the kernel can
       * type it as a dependent elimination. */
      if (strcmp(name, "boolrec") == 0) {
        kterm_t *mot = sexp_nth_kterm(heap, parts, 0);
        kterm_t *tc = sexp_nth_kterm(heap, parts, 1);
        kterm_t *fc = sexp_nth_kterm(heap, parts, 2);
        kterm_t *sc = sexp_nth_kterm(heap, parts, 3);
        kterm_t *br;
        if (!mot || !tc || !fc || !sc) return NULL;
        br = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(br, 0, sizeof(*br));
        br->tag = KT_BOOL_REC;
        br->data.bool_rec.motive = mot;
        br->data.bool_rec.true_case = tc;
        br->data.bool_rec.false_case = fc;
        br->data.bool_rec.scrutinee = sc;
        return br;
      }
      /* (listrec motive nil-case cons-case scrutinee) */
      if (strcmp(name, "listrec") == 0) {
        kterm_t *mot = sexp_nth_kterm(heap, parts, 0);
        kterm_t *nc = sexp_nth_kterm(heap, parts, 1);
        kterm_t *cc = sexp_nth_kterm(heap, parts, 2);
        kterm_t *sc = sexp_nth_kterm(heap, parts, 3);
        kterm_t *lr;
        if (!mot || !nc || !cc || !sc) return NULL;
        lr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(lr, 0, sizeof(*lr));
        lr->tag = KT_LIST_REC;
        lr->data.list_rec.motive = mot;
        lr->data.list_rec.nil_case = nc;
        lr->data.list_rec.cons_case = cc;
        lr->data.list_rec.scrutinee = sc;
        return lr;
      }
      /* (mayberec motive nothing-case just-case scrutinee) */
      if (strcmp(name, "mayberec") == 0) {
        kterm_t *mot = sexp_nth_kterm(heap, parts, 0);
        kterm_t *nc = sexp_nth_kterm(heap, parts, 1);
        kterm_t *jc = sexp_nth_kterm(heap, parts, 2);
        kterm_t *sc = sexp_nth_kterm(heap, parts, 3);
        kterm_t *mr;
        if (!mot || !nc || !jc || !sc) return NULL;
        mr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(mr, 0, sizeof(*mr));
        mr->tag = KT_MAYBE_REC;
        mr->data.maybe_rec.motive = mot;
        mr->data.maybe_rec.nothing_case = nc;
        mr->data.maybe_rec.just_case = jc;
        mr->data.maybe_rec.scrutinee = sc;
        return mr;
      }
      /* (sumrec motive left-case right-case scrutinee) */
      if (strcmp(name, "sumrec") == 0) {
        kterm_t *mot = sexp_nth_kterm(heap, parts, 0);
        kterm_t *lc = sexp_nth_kterm(heap, parts, 1);
        kterm_t *rc = sexp_nth_kterm(heap, parts, 2);
        kterm_t *sc = sexp_nth_kterm(heap, parts, 3);
        kterm_t *sr;
        if (!mot || !lc || !rc || !sc) return NULL;
        sr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(sr, 0, sizeof(*sr));
        sr->tag = KT_SUM_REC;
        sr->data.sum_rec.motive = mot;
        sr->data.sum_rec.left_case = lc;
        sr->data.sum_rec.right_case = rc;
        sr->data.sum_rec.scrutinee = sc;
        return sr;
      }
      /* (absurd A) — ex falso: absurd {A} : Empty -> A. */
      if (strcmp(name, "absurd") == 0) {
        kterm_t *ty = sexp_nth_kterm(heap, parts, 0);
        kterm_t *ab;
        if (!ty) return NULL;
        ab = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(ab, 0, sizeof(*ab));
        ab->tag = KT_ABSURD;
        ab->data.absurd.target_type = ty;
        return ab;
      }
      /* (let value body) — body refers to the bound value as #0. */
      if (strcmp(name, "let") == 0) {
        kterm_t *val = sexp_nth_kterm(heap, parts, 0);
        kterm_t *body = sexp_nth_kterm(heap, parts, 1);
        kterm_t *lt;
        if (!val || !body) return NULL;
        lt = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(lt, 0, sizeof(*lt));
        lt->tag = KT_LET;
        lt->data.let.name = "x";
        lt->data.let.value = val;
        lt->data.let.body = body;
        return lt;
      }
    }
  }
  return NULL;
}

