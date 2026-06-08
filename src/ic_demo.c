/* ic_demo.c -- standalone driver exercising the ic interaction-calculus core.
 *
 *   gcc -std=c89 -O2 -Wall -Wextra -I. ic_demo.c ic.c -lgmp -o ic_demo
 *
 * What it demonstrates:
 *   1. arithmetic + lambda agree with the inet.c baseline (the sharing / "P" side)
 *   2. superposition fans a computation out over alternatives (the search / "NP" side)
 *   3. the DUP/SUP duality and HVM-style label semantics:
 *        - dup label == sup label  -> the search COLLAPSES to a pair (construction
 *                                       meets its matching observation)
 *        - same-label superpositions stay CORRELATED (a single shared choice)
 *        - different-label superpositions form the OUTER PRODUCT (independent choices)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ic.h"

static int failures = 0;
static int checks   = 0;

static void expect(const char *src, const char *want) {
  char err[256];
  char buf[1024];
  long cost = 0;
  int  rc;
  ic_term_t *t = ic_parse(src, err, sizeof err);
  checks++;
  if (!t) {
    printf("  PARSE FAIL  %-34s : %s\n", src, err);
    failures++;
    return;
  }
  rc = ic_readback(t, buf, sizeof buf, &cost);
  if (rc != 1) {
    printf("  READBACK rc=%d  %-34s (cost %ld)\n", rc, src, cost);
    failures++;
    ic_term_free(t);
    return;
  }
  if (strcmp(buf, want) != 0) {
    printf("  FAIL  %-34s => %-18s want %-18s (%ld interactions)\n",
           src, buf, want, cost);
    failures++;
  } else {
    printf("  ok    %-34s => %-18s (%ld interactions)\n", src, buf, cost);
  }
  ic_term_free(t);
}

static void show_cost(const char *label, const char *src) {
  char err[256];
  char buf[1024];
  long cost = 0;
  int  rc;
  ic_term_t *t = ic_parse(src, err, sizeof err);
  if (!t) { printf("  %-16s PARSE FAIL: %s\n", label, err); failures++; checks++; return; }
  rc = ic_readback(t, buf, sizeof buf, &cost);
  if (rc != 1) { strcpy(buf, "<rc!=1>"); }
  printf("  %-16s %-34s => %-18s cost=%ld live=%ld\n",
         label, src, buf, cost, ic_live_nodes());
  ic_term_free(t);
}

int main(void) {
  printf("== arithmetic (exact GMP, must match inet baseline) ==\n");
  expect("(op + 3 4)", "7");
  expect("(op * 12 (op + 3 4))", "84");
  expect("(op - 100 1)", "99");
  expect("(op * 123 123)", "15129");

  printf("\n== lambda / application (affine readback) ==\n");
  expect("((lam x x) 99)", "99");
  expect("((lam x (op + x (op + x x))) 7)", "21");
  expect("((lam x (op + x x)) ((lam x (op + x x)) 10))", "40");

  printf("\n== Church-style combinators ==\n");
  /* S K K 5 = 5 : S = \\f.\\g.\\x. f x (g x) ; K = \\x.\\y. x */
  expect(
    "((((lam f (lam g (lam x ((f x) (g x))))) (lam a (lam b a))) (lam c (lam d c))) 5)",
    "5");

  printf("\n== superposition: bare pairs read back faithfully ==\n");
  expect("{2 3}", "{2 3}");
  expect("{10 {20 30}}", "{10 {20 30}}");
  expect("{:1 2 3}", "{2 3}");                 /* label is internal, not printed */

  printf("\n== a computation distributes over a superposition (search) ==\n");
  /* an operator applied to a superposition runs on both alternatives */
  expect("(op + 1 {2 3})", "{3 4}");
  expect("(op * {2 3} 10)", "{20 30}");
  /* a lambda that uses its argument twice, fed a superposition: the two copies
   * keep the argument's label, so they stay correlated -> {2+2, 3+3} */
  expect("((lam x (op + x x)) {2 3})", "{4 6}");

  printf("\n== DUP meets SUP: collapse when labels match ==\n");
  /* dup label 7 meets sup label 7 -> annihilate: x<-10, y<-20 -> 30 */
  expect("(dup :7 x y {:7 10 20} (op + x y))", "30");

  printf("\n== same label = one shared choice (correlated / zip) ==\n");
  /* both superpositions carry the default label 0, so they are the SAME choice:
   * left pairs with left, right with right -> {10+100, 20+200} */
  expect("(op + {10 20} {100 200})", "{110 220}");

  printf("\n== different labels = independent choices (outer product) ==\n");
  /* labels 1 and 2 differ, so the two choices multiply:
   * {10,20} x {100,200} -> {{110 210} {120 220}} */
  expect("(op + {:1 10 20} {:2 100 200})", "{{110 210} {120 220}}");

  printf("\n== sharing (P) vs search (NP): interaction counts ==\n");
  /* The same per-element work, reached three ways.  Sharing keeps it linear;
   * a correlated superposition adds a constant; an independent one multiplies. */
  show_cost("shared",       "((lam x (op + x x)) 21)");
  show_cost("correlated",   "((lam x (op + x x)) {20 21})");
  show_cost("independent",  "(op + {:1 20 21} {:2 20 21})");
  show_cost("wide search",  "((lam x (op + x x)) {18 {19 {20 21}}})");

  printf("\n----------------------------------------------------------\n");
  if (failures == 0)
    printf("ALL %d CHECKS PASSED\n", checks);
  else
    printf("%d/%d CHECKS FAILED\n", failures, checks);
  return failures ? 1 : 0;
}
