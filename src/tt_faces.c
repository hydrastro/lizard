/* tt_faces.c
 *
 * Cubical face entailment and system lookup. Split from tt_equality.c so
 * equality normalization is not responsible for the face-decision API.
 */

#include "primitives.h"
#include "errors.h"
#include "mem.h"

#include <string.h>

static int tt_faces_two_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}

static lizard_ast_node_t *tt_faces_nth_arg(lz_list_t *args, int n) {
  lz_list_node_t *it;
  it = args->head;
  while (n-- > 0 && it != args->nil) {
    it = it->next;
  }
  if (it == args->nil) {
    return NULL;
  }
  return ((lizard_ast_list_node_t *)it)->ast;
}

/* ----- Face entailment decision procedure (Turn 7) -----
 *
 * Decides whether face formula φ entails ψ: φ ⊨ ψ.
 * Returns:
 *   1 — definitely entails
 *   0 — definitely does not entail
 *  -1 — undecidable by the procedure
 *
 * Strategy is structural:
 *   F0 ⊨ anything                      (anything follows from false)
 *   anything ⊨ F1                      (truth follows from anything)
 *   φ ⊨ φ                              (reflexivity via alpha-equal)
 *   (F-or φ ψ) ⊨ χ  iff  φ ⊨ χ AND ψ ⊨ χ
 *   φ ⊨ (F-and ψ χ) iff  φ ⊨ ψ AND φ ⊨ χ
 *   (F-and φ _) ⊨ φ                    (and-left projection)
 *   (F-and _ φ) ⊨ φ                    (and-right projection)
 *   φ ⊨ (F-or ψ _) if φ ⊨ ψ           (or-left injection, incomplete)
 *   φ ⊨ (F-or _ ψ) if φ ⊨ ψ           (or-right injection, incomplete)
 *
 * Used by comp (Turn 8) and Sub typing. */

int lizard_tt_face_entails(lizard_ast_node_t *phi,
                           lizard_ast_node_t *psi) {  if (phi == NULL || psi == NULL) return -1;
  if (phi->type == AST_TT_F0) return 1;        /* F0 entails anything */
  if (psi->type == AST_TT_F1) return 1;        /* anything entails F1 */
  if (lizard_tt_alpha_equal(phi, psi)) return 1; /* reflexive */
  /* (F-or phi1 phi2) ⊨ psi iff phi1 ⊨ psi AND phi2 ⊨ psi */
  if (phi->type == AST_TT_F_OR) {
    int l = lizard_tt_face_entails(phi->data.tt_f_binop.left, psi);
    int r = lizard_tt_face_entails(phi->data.tt_f_binop.right, psi);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  /* phi ⊨ (F-and psi1 psi2) iff phi ⊨ psi1 AND phi ⊨ psi2 */
  if (psi->type == AST_TT_F_AND) {
    int l = lizard_tt_face_entails(phi, psi->data.tt_f_binop.left);
    int r = lizard_tt_face_entails(phi, psi->data.tt_f_binop.right);
    if (l == 1 && r == 1) return 1;
    if (l == 0 || r == 0) return 0;
    return -1;
  }
  /* (F-and phi1 phi2) ⊨ phi1 and ⊨ phi2 (projection). */
  if (phi->type == AST_TT_F_AND) {
    if (lizard_tt_alpha_equal(phi->data.tt_f_binop.left, psi)) return 1;
    if (lizard_tt_alpha_equal(phi->data.tt_f_binop.right, psi)) return 1;
    /* Try recursively. */
    {
      int l = lizard_tt_face_entails(phi->data.tt_f_binop.left, psi);
      int r = lizard_tt_face_entails(phi->data.tt_f_binop.right, psi);
      if (l == 1 || r == 1) return 1;
    }
  }
  /* phi ⊨ (F-or psi1 psi2) if phi ⊨ psi1 OR phi ⊨ psi2 (sufficient
   * but incomplete: e.g. (i=i0 ∨ i=i1) on i ⊨ (i=i0 ∨ i=i1) by
   * reflexivity, but the disjunction-elim-style version would fail). */
  if (psi->type == AST_TT_F_OR) {
    int l = lizard_tt_face_entails(phi, psi->data.tt_f_binop.left);
    int r = lizard_tt_face_entails(phi, psi->data.tt_f_binop.right);
    if (l == 1 || r == 1) return 1;
  }
  return -1;
}

lizard_ast_node_t *lizard_primitive_tt_face_entails(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *phi, *psi;
  int r;
  (void)env;
  if (!tt_faces_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  phi = tt_faces_nth_arg(args, 0);
  psi = tt_faces_nth_arg(args, 1);
  phi = lizard_tt_reduce(phi, heap);
  psi = lizard_tt_reduce(psi, heap);
  r = lizard_tt_face_entails(phi, psi);
  if (r == 1) return lizard_make_bool(heap, 1);
  if (r == 0) return lizard_make_bool(heap, 0);
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

/* System lookup (Turn 11):
 *
 * Given a system and a "context face" φ_ctx (the face we know to be
 * true), find the first clause (φ_i, u_i) in the system such that
 * φ_ctx ⊨ φ_i (the context face entails this clause's face). If
 * found, return u_i. If not, return NULL.
 *
 * Used by comp Glue to look up the equiv/T for the active face,
 * and by system reduction when one of the clause faces becomes F1
 * (which is entailed by everything, so its clause is selected).
 */
lizard_ast_node_t *lizard_tt_system_lookup(lizard_ast_node_t *system,
                                           lizard_ast_node_t *phi_ctx) {
  while (system && system->type == AST_TT_SYSTEM_CONS) {
    lizard_ast_node_t *clause_face = system->data.tt_system_cons.face;
    /* If phi_ctx entails this clause's face, the clause fires. */
    if (lizard_tt_face_entails(phi_ctx, clause_face) == 1) {
      return system->data.tt_system_cons.value;
    }
    system = system->data.tt_system_cons.next;
  }
  return NULL;
}

/* Primitive wrapper. */
lizard_ast_node_t *lizard_primitive_tt_system_lookup(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *sys, *phi, *r;
  (void)env;
  if (!tt_faces_two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  sys = lizard_tt_reduce(tt_faces_nth_arg(args, 0), heap);
  phi = lizard_tt_reduce(tt_faces_nth_arg(args, 1), heap);
  r = lizard_tt_system_lookup(sys, phi);
  if (r == NULL) return lizard_make_nil(heap);
  return r;
}

