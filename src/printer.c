#include "printer.h"
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lizard_print_indent(int depth) {
  int i;
  for (i = 0; i < depth; i++) {
    printf("  ");
  }
}

void lizard_fprint_ast(FILE *fp, lizard_ast_node_t *node, int depth) {
  if (!node) {
    fprintf(fp, "NULL\n");
    return;
  }
  lizard_print_indent(depth);
  switch (node->type) {
  case AST_STRING:
    fprintf(fp, "String: \"%s\"\n", node->data.string);
    break;
  case AST_NUMBER:
    fprintf(fp, "Number: ");
    gmp_printf("%Zd", node->data.number);
    fprintf(fp, "\n");
    break;
  case AST_SYMBOL:
    fprintf(fp, "Symbol: %s\n", node->data.variable);
    break;
  case AST_BOOL:
    fprintf(fp, "Boolean: %s\n", node->data.boolean ? "#t" : "#f");
    break;
  case AST_PAIR:
    fprintf(fp, "Pair:\n");
    lizard_print_indent(depth + 1);
    fprintf(fp, "Car:\n");
    lizard_fprint_ast(fp, node->data.pair.car, depth + 2);
    lizard_print_indent(depth + 1);
    fprintf(fp, "Cdr:\n");
    lizard_fprint_ast(fp, node->data.pair.cdr, depth + 2);
    break;
  case AST_NIL:
    fprintf(fp, "Nil\n");
    break;
  case AST_QUOTE:
    fprintf(fp, "Quote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_QUASIQUOTE:
    fprintf(fp, "Quasiquote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_UNQUOTE:
    fprintf(fp, "Unquote:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_UNQUOTE_SPLICING:
    fprintf(fp, "Unquote-splicing:\n");
    lizard_fprint_ast(fp, node->data.quoted, depth + 1);
    break;
  case AST_ASSIGNMENT:
    fprintf(fp, "Assignment:\n");
    lizard_fprint_ast(fp, node->data.assignment.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.assignment.value, depth + 1);
    break;
  case AST_DEFINITION:
    fprintf(fp, "Definition:\n");
    lizard_fprint_ast(fp, node->data.definition.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.definition.value, depth + 1);
    break;
  case AST_IF:
    fprintf(fp, "If:\n");
    lizard_fprint_ast(fp, node->data.if_clause.pred, depth + 1);
    lizard_fprint_ast(fp, node->data.if_clause.cons, depth + 1);
    if (node->data.if_clause.alt)
      lizard_fprint_ast(fp, node->data.if_clause.alt, depth + 1);
    break;
  case AST_LAMBDA:
    fprintf(fp, "Lambda:\n");
    if (node->data.lambda.parameters) {
      lz_list_node_t *iter = node->data.lambda.parameters->head;
      while (iter != node->data.lambda.parameters->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_BEGIN:
    fprintf(fp, "Begin:\n");
    {
      lz_list_node_t *iter = node->data.begin_expressions->head;
      while (iter != node->data.begin_expressions->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_APPLICATION:
    fprintf(fp, "Application:\n");
    {
      lz_list_node_t *iter = node->data.application_arguments->head;
      if (iter == node->data.application_arguments->nil) {
        lizard_print_indent(depth + 1);
        fprintf(fp, "Empty application\n");
      }
      while (iter != node->data.application_arguments->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_MACRO:
    fprintf(fp, "Macro:\n");
    lizard_fprint_ast(fp, node->data.macro_def.variable, depth + 1);
    lizard_fprint_ast(fp, node->data.macro_def.transformer, depth + 1);
    break;
  case AST_PROMISE:
    fprintf(fp, "Promise (forced=%s)\n",
            node->data.promise.forced ? "true" : "false");
    if (node->data.promise.forced)
      lizard_fprint_ast(fp, node->data.promise.value, depth + 1);
    else
      lizard_fprint_ast(fp, node->data.promise.expr, depth + 1);
    break;
  case AST_CONTINUATION:
    fprintf(fp, "Continuation: %p\n",
            (void *)(uintptr_t)node->data.continuation.captured_cont);
    break;
  case AST_CALLCC:
    fprintf(fp, "Call/cc:\n");
    {
      lz_list_node_t *iter = node->data.application_arguments->head;
      while (iter != node->data.application_arguments->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  case AST_ERROR:
    fprintf(fp, "Error (code %d):\n", node->data.error.code);
    {
      lz_list_node_t *iter = node->data.error.data->head;
      while (iter != node->data.error.data->nil) {
        lizard_fprint_ast(fp, ((lizard_ast_list_node_t *)iter)->ast, depth + 1);
        iter = iter->next;
      }
    }
    break;
  default:
    fprintf(fp, "Unknown AST node type.\n");
  }
}

void lizard_print_ast(lizard_ast_node_t *node, int depth) {
  lizard_fprint_ast(stdout, node, depth);
}

/* ---- Scheme-style value printer ----
   Used by the REPL so users see real Lisp instead of an AST dump. */

static int value_is_empty_list(lizard_ast_node_t *node) {
  if (!node) {
    return 1;
  }
  if (node->type == AST_NIL) {
    return 1;
  }
  if (node->type == AST_APPLICATION &&
      node->data.application_arguments->head ==
          node->data.application_arguments->nil) {
    return 1;
  }
  return 0;
}

void lizard_fprint_value(FILE *fp, lizard_ast_node_t *node) {
  if (!node) {
    fprintf(fp, "()");
    return;
  }
  switch (node->type) {
  case AST_NIL:
    fprintf(fp, "()");
    return;
  case AST_NUMBER:
    gmp_fprintf(fp, "%Zd", node->data.number);
    return;
  case AST_BOOL:
    fprintf(fp, "%s", node->data.boolean ? "#t" : "#f");
    return;
  case AST_STRING:
    fprintf(fp, "\"%s\"", node->data.string);
    return;
  case AST_SYMBOL:
    fprintf(fp, "%s", node->data.variable);
    return;
  case AST_QUOTE:
    /* The quote mark is input syntax; the value of '(a b c) is the
       list (a b c). Print the underlying datum directly. */
    lizard_fprint_value(fp, node->data.quoted);
    return;
  case AST_QUASIQUOTE:
    lizard_fprint_value(fp, node->data.quoted);
    return;
  case AST_UNQUOTE:
    fprintf(fp, ",");
    lizard_fprint_value(fp, node->data.quoted);
    return;
  case AST_UNQUOTE_SPLICING:
    fprintf(fp, ",@");
    lizard_fprint_value(fp, node->data.quoted);
    return;
  case AST_PAIR: {
    /* Walk the cdr chain; print as a proper list (a b c) when cdr ends
       in nil, otherwise dotted (a b . c). */
    lizard_ast_node_t *cur = node;
    int first = 1;
    fprintf(fp, "(");
    while (cur && cur->type == AST_PAIR) {
      if (!first) {
        fprintf(fp, " ");
      }
      first = 0;
      lizard_fprint_value(fp, cur->data.pair.car);
      cur = cur->data.pair.cdr;
    }
    if (cur && !value_is_empty_list(cur)) {
      fprintf(fp, " . ");
      lizard_fprint_value(fp, cur);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_APPLICATION: {
    /* A "list literal" produced by quote — print like a list. */
    lz_list_node_t *iter;
    int first = 1;
    fprintf(fp, "(");
    for (iter = node->data.application_arguments->head;
         iter != node->data.application_arguments->nil; iter = iter->next) {
      if (!first) {
        fprintf(fp, " ");
      }
      first = 0;
      lizard_fprint_value(fp, ((lizard_ast_list_node_t *)iter)->ast);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_LAMBDA:
    fprintf(fp, "<procedure>");
    return;
  case AST_PRIMITIVE:
    fprintf(fp, "<primitive>");
    return;
  case AST_VECTOR: {
    size_t i;
    fprintf(fp, "#(");
    for (i = 0; i < node->data.vector.size; i++) {
      if (i > 0) fprintf(fp, " ");
      lizard_fprint_value(fp, node->data.vector.elements[i]);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_HASH: {
    size_t i;
    int first = 1;
    fprintf(fp, "#hash(");
    for (i = 0; i < node->data.hash.cap; i++) {
      if (node->data.hash.keys[i] != NULL) {
        if (!first) fprintf(fp, " ");
        first = 0;
        fprintf(fp, "(");
        lizard_fprint_value(fp, node->data.hash.keys[i]);
        fprintf(fp, " . ");
        lizard_fprint_value(fp, node->data.hash.values[i]);
        fprintf(fp, ")");
      }
    }
    fprintf(fp, ")");
    return;
  }
  case AST_TT_PI:
    fprintf(fp, "(Pi (");
    if (node->data.tt_pi.binder) {
      lizard_fprint_value(fp, node->data.tt_pi.binder);
      fprintf(fp, " ");
    }
    lizard_fprint_value(fp, node->data.tt_pi.domain);
    fprintf(fp, ") ");
    lizard_fprint_value(fp, node->data.tt_pi.codomain);
    fprintf(fp, ")");
    return;
  case AST_TT_SIGMA:
    fprintf(fp, "(Sigma (");
    if (node->data.tt_sigma.binder) {
      lizard_fprint_value(fp, node->data.tt_sigma.binder);
      fprintf(fp, " ");
    }
    lizard_fprint_value(fp, node->data.tt_sigma.domain);
    fprintf(fp, ") ");
    lizard_fprint_value(fp, node->data.tt_sigma.codomain);
    fprintf(fp, ")");
    return;
  case AST_TT_APP:
    fprintf(fp, "(@ ");
    lizard_fprint_value(fp, node->data.tt_app.fun);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_app.arg);
    fprintf(fp, ")");
    return;
  case AST_TT_SUM:
    fprintf(fp, "(Sum ");
    lizard_fprint_value(fp, node->data.tt_sum.left);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_sum.right);
    fprintf(fp, ")");
    return;
  case AST_TT_UNIVERSE:
    fprintf(fp, "(U %ld)", node->data.tt_universe.level);
    return;
  case AST_TT_COUNIVERSE:
    fprintf(fp, "(Uco %ld)", node->data.tt_couniverse.level);
    return;
  case AST_TT_ID:
    fprintf(fp, "(Id ");
    lizard_fprint_value(fp, node->data.tt_id.domain);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_id.a);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_id.b);
    fprintf(fp, ")");
    return;
  case AST_TT_REFL:
    fprintf(fp, "(refl ");
    lizard_fprint_value(fp, node->data.tt_refl.value);
    fprintf(fp, ")");
    return;
  case AST_TT_INDUCTIVE: {
    lz_list_node_t *it;
    fprintf(fp, "(Inductive ");
    lizard_fprint_value(fp, node->data.tt_inductive.name);
    for (it = node->data.tt_inductive.constructors->head;
         it != node->data.tt_inductive.constructors->nil; it = it->next) {
      fprintf(fp, " ");
      lizard_fprint_value(fp, ((lizard_ast_list_node_t *)it)->ast);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_TT_COINDUCTIVE: {
    lz_list_node_t *it;
    fprintf(fp, "(Coinductive ");
    lizard_fprint_value(fp, node->data.tt_coinductive.name);
    for (it = node->data.tt_coinductive.destructors->head;
         it != node->data.tt_coinductive.destructors->nil; it = it->next) {
      fprintf(fp, " ");
      lizard_fprint_value(fp, ((lizard_ast_list_node_t *)it)->ast);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_TT_ANNOT:
    fprintf(fp, "(: ");
    lizard_fprint_value(fp, node->data.tt_annot.term);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_annot.type);
    fprintf(fp, ")");
    return;
  case AST_TT_VARIABLE:
    fprintf(fp, "(variable ");
    lizard_fprint_value(fp, node->data.tt_variable.name);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_variable.type);
    fprintf(fp, ")");
    return;
  case AST_TT_CONTEXT: {
    lz_list_node_t *it;
    int first = 1;
    fprintf(fp, "(context");
    for (it = node->data.tt_context.bindings->head;
         it != node->data.tt_context.bindings->nil; it = it->next) {
      (void)first;
      fprintf(fp, " ");
      lizard_fprint_value(fp, ((lizard_ast_list_node_t *)it)->ast);
    }
    fprintf(fp, ")");
    return;
  }
  case AST_TT_SUBSTITUTION: {
    lz_list_node_t *it;
    fprintf(fp, "(substitution ");
    lizard_fprint_value(fp, node->data.tt_substitution.source);
    fprintf(fp, " => ");
    lizard_fprint_value(fp, node->data.tt_substitution.target);
    fprintf(fp, " [");
    for (it = node->data.tt_substitution.mappings->head;
         it != node->data.tt_substitution.mappings->nil; it = it->next) {
      if (it != node->data.tt_substitution.mappings->head) fprintf(fp, " ");
      lizard_fprint_value(fp, ((lizard_ast_list_node_t *)it)->ast);
    }
    fprintf(fp, "])");
    return;
  }
  case AST_TT_JUDGMENT:
    fprintf(fp, "(");
    lizard_fprint_value(fp, node->data.tt_judgment.context);
    fprintf(fp, " |- ");
    lizard_fprint_value(fp, node->data.tt_judgment.term);
    fprintf(fp, " : ");
    lizard_fprint_value(fp, node->data.tt_judgment.type);
    fprintf(fp, ")");
    return;
  case AST_TT_EQUIV:
    fprintf(fp, "(equivalence ");
    lizard_fprint_value(fp, node->data.tt_equiv.left);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_equiv.right);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_equiv.fwd);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_equiv.bwd);
    fprintf(fp, ")");
    return;
  case AST_TT_TRANSPORT:
    fprintf(fp, "(transport ");
    lizard_fprint_value(fp, node->data.tt_transport.path);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_transport.value);
    fprintf(fp, ")");
    return;
  case AST_TT_ID_SYM:
    fprintf(fp, "(Id-sym ");
    lizard_fprint_value(fp, node->data.tt_id_sym.path);
    fprintf(fp, ")");
    return;
  case AST_TT_ID_TRANS:
    fprintf(fp, "(Id-trans ");
    lizard_fprint_value(fp, node->data.tt_id_trans.p);
    fprintf(fp, " ");
    lizard_fprint_value(fp, node->data.tt_id_trans.q);
    fprintf(fp, ")");
    return;
  case AST_MACRO:
    fprintf(fp, "<macro>");
    return;
  case AST_CONTINUATION:
    fprintf(fp, "<continuation>");
    return;
  case AST_PROMISE:
    if (node->data.promise.forced) {
      lizard_fprint_value(fp, node->data.promise.value);
    } else {
      fprintf(fp, "<promise>");
    }
    return;
  case AST_ERROR: {
    lz_list_node_t *iter = node->data.error.data->head;
    if (iter != node->data.error.data->nil) {
      lizard_ast_node_t *msg = ((lizard_ast_list_node_t *)iter)->ast;
      if (msg->type == AST_STRING) {
        fprintf(fp, "%s", msg->data.string);
      } else {
        lizard_fprint_value(fp, msg);
      }
    } else {
      fprintf(fp, "error (code %d)", node->data.error.code);
    }
    return;
  }
  default:
    /* Fall back to the AST dump for forms that have no value-form
       representation (defines, ifs, etc., which shouldn't appear here). */
    lizard_fprint_ast(fp, node, 0);
    return;
  }
}

void lizard_print_value(lizard_ast_node_t *node) {
  lizard_fprint_value(stdout, node);
}
