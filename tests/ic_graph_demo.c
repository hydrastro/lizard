/* ic_graph_demo.c -- show that a program is an abstract syntax GRAPH, not a tree.
 *
 *   cc -std=c89 -O2 -Wall -Wextra -Isrc tests/ic_graph_demo.c src/ic.c -lgmp -o ic_graph_demo
 *
 * For each term it prints the compiled net (the ASG) and, where the term reduces
 * to a value, the net after reduction.  The point to look for is the DUP node: a
 * variable used twice is NOT copied in the graph -- it is one wire into a DUP
 * whose two outputs fan to the two use sites.  That is the sharing (DAG) property
 * that makes the representation a graph rather than a syntax tree.
 */
#include "ic.h"
#include <stdio.h>
#include <string.h>

static void dump(const char *src, const char *note) {
  char err[256], g[4096];
  ic_term_t *t;
  printf("  term : %s\n", src);
  if (note) printf("         (%s)\n", note);

  t = ic_parse(src, err, sizeof err);
  if (!t) { printf("  parse error: %s\n\n", err); return; }
  if (ic_dump_net(t, g, sizeof g, 0) == 1) printf("  ASG  : %s\n", g);
  ic_term_free(t);

  t = ic_parse(src, err, sizeof err);                 /* fresh tree to reduce */
  if (ic_dump_net(t, g, sizeof g, 1) == 1) printf("  =>   : %s\n", g);
  ic_term_free(t);
  printf("\n");
}

int main(void) {
  printf("A program, compiled, is a graph of agent nodes joined by wires.\n");
  printf("Read [p1 p2] as the two ports of a node; nK is a wire to node K;\n");
  printf("#n is a number, * an eraser. Variables are wires, so they do not\n");
  printf("appear by name -- and a value used twice goes through a DUP.\n\n");

  dump("(lam x (lam y x))",
       "K = \\x.\\y.x : two nested CON nodes, no reduction");

  dump("((lam x (op + x x)) 5)",
       "x used twice -> a DUP node shares it into both operands of +");

  dump("((lam x (op * x x)) (op + 2 3))",
       "the argument (2+3) is computed ONCE and shared by a DUP, not recomputed");

  dump("(op + {10 20} {100 200})",
       "two superpositions: SUP nodes in the graph");

  printf("In each compiled graph the variable never appears as a leaf: it is the\n");
  printf("wire itself, and sharing is a DUP fan-out -- a directed acyclic graph,\n");
  printf("not a tree. Reduction rewrites this graph locally until no active pair\n");
  printf("remains. That is the sense in which the program *is* the net.\n");
  return 0;
}
