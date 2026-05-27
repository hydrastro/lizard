/* src/bytecode.c — Bytecode compiler and stack-based VM (Phase E).
 *
 * Compiles a subset of Scheme to bytecode and executes it on a
 * stack machine. Forms not supported by the compiler fall back to
 * the tree-walking evaluator.
 */
#include "bytecode.h"
#include "errors.h"
#include "mem.h"
#include "primitives.h"
#include "printer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

/* ---- opcode names ---- */

const char *lizard_bc_opcode_name(lizard_opcode_t op) {
  switch (op) {
  case OP_CONST: return "CONST";
  case OP_NIL: return "NIL";
  case OP_TRUE: return "TRUE";
  case OP_FALSE: return "FALSE";
  case OP_LOAD: return "LOAD";
  case OP_STORE: return "STORE";
  case OP_SET: return "SET";
  case OP_POP: return "POP";
  case OP_ADD: return "ADD";
  case OP_SUB: return "SUB";
  case OP_MUL: return "MUL";
  case OP_DIV: return "DIV";
  case OP_MOD: return "MOD";
  case OP_EQ: return "EQ";
  case OP_LT: return "LT";
  case OP_GT: return "GT";
  case OP_NOT: return "NOT";
  case OP_CONS: return "CONS";
  case OP_CAR: return "CAR";
  case OP_CDR: return "CDR";
  case OP_JUMP: return "JUMP";
  case OP_JUMP_IF_FALSE: return "JIF";
  case OP_CALL: return "CALL";
  case OP_TAIL_CALL: return "TCALL";
  case OP_RETURN: return "RET";
  case OP_CLOSURE: return "CLOSURE";
  case OP_DISPLAY: return "DISPLAY";
  case OP_NEWLINE: return "NEWLINE";
  case OP_HALT: return "HALT";
  }
  return "???";
}

/* ---- chunk helpers ---- */

static lizard_bc_chunk_t *chunk_create(lizard_heap_t *heap) {
  lizard_bc_chunk_t *c;
  c = (lizard_bc_chunk_t *)malloc(sizeof(lizard_bc_chunk_t));
  if (c == NULL) return NULL;
  memset(c, 0, sizeof(*c));
  return c;
}

static int chunk_emit(lizard_bc_chunk_t *c, lizard_opcode_t op, int arg) {
  if (c->code_count >= LIZARD_BC_MAX_CODE) return -1;
  c->code[c->code_count].op = op;
  c->code[c->code_count].arg = arg;
  return c->code_count++;
}

static int chunk_add_const(lizard_bc_chunk_t *c, lizard_ast_node_t *val) {
  if (c->const_count >= LIZARD_BC_MAX_CONSTANTS) return -1;
  c->constants[c->const_count] = val;
  return c->const_count++;
}

/* ---- compiler ---- */

/* Forward declaration. */
static int compile_expr(lizard_bc_chunk_t *c, lizard_ast_node_t *expr,
                        lizard_heap_t *heap, int tail_position);

/* Get list length from an AST list (lz_list_t). */
static int list_len(lz_list_t *list) {
  int n = 0;
  lz_list_node_t *iter;
  if (list == NULL) return 0;
  for (iter = list->head; iter != list->nil; iter = iter->next) n++;
  return n;
}

/* Compile the body of a begin/sequence: compile each expr, pop
 * intermediates, keep the last. */
static int compile_sequence(lizard_bc_chunk_t *c, lz_list_t *exprs,
                             lizard_heap_t *heap, int tail_position) {
  lz_list_node_t *iter;
  int count = 0;
  if (exprs == NULL) {
    chunk_emit(c, OP_NIL, 0);
    return 0;
  }
  for (iter = exprs->head; iter != exprs->nil; iter = iter->next) {
    int is_last = (iter->next == exprs->nil);
    if (compile_expr(c, ((lizard_ast_list_node_t *)iter)->ast, heap,
                     is_last && tail_position) < 0) {
      return -1;
    }
    if (!is_last) {
      chunk_emit(c, OP_POP, 0);
    }
    count++;
  }
  if (count == 0) {
    chunk_emit(c, OP_NIL, 0);
  }
  return 0;
}

static int compile_expr(lizard_bc_chunk_t *c, lizard_ast_node_t *expr,
                        lizard_heap_t *heap, int tail_position) {
  if (expr == NULL) {
    chunk_emit(c, OP_NIL, 0);
    return 0;
  }

  switch (expr->type) {
  case AST_NUMBER:
  case AST_STRING: {
    int idx = chunk_add_const(c, expr);
    if (idx < 0) return -1;
    chunk_emit(c, OP_CONST, idx);
    return 0;
  }
  case AST_BOOL:
    chunk_emit(c, expr->data.boolean ? OP_TRUE : OP_FALSE, 0);
    return 0;
  case AST_NIL:
    chunk_emit(c, OP_NIL, 0);
    return 0;
  case AST_SYMBOL: {
    /* Variable reference. Store the symbol name as a constant. */
    lizard_ast_node_t *name_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    int idx;
    name_node->type = AST_STRING;
    name_node->data.string = expr->data.variable;
    idx = chunk_add_const(c, name_node);
    if (idx < 0) return -1;
    chunk_emit(c, OP_LOAD, idx);
    return 0;
  }
  case AST_QUOTE:
    /* Quoted datum: push as constant. */
    {
      int idx = chunk_add_const(c, expr->data.quoted);
      if (idx < 0) return -1;
      chunk_emit(c, OP_CONST, idx);
      return 0;
    }

  case AST_IF:
    /* (if pred cons alt) */
    {
      int jump_false, jump_end, after_cons;
      if (compile_expr(c, expr->data.if_clause.pred, heap, 0) < 0) return -1;
      jump_false = chunk_emit(c, OP_JUMP_IF_FALSE, 0);  /* patch later */
      if (compile_expr(c, expr->data.if_clause.cons, heap, tail_position) < 0) return -1;
      jump_end = chunk_emit(c, OP_JUMP, 0);  /* patch later */
      after_cons = c->code_count;
      c->code[jump_false].arg = after_cons;
      if (expr->data.if_clause.alt != NULL) {
        if (compile_expr(c, expr->data.if_clause.alt, heap, tail_position) < 0) return -1;
      } else {
        chunk_emit(c, OP_NIL, 0);
      }
      c->code[jump_end].arg = c->code_count;
      return 0;
    }

  case AST_DEFINITION:
    /* (define var val) */
    {
      lizard_ast_node_t *name_node;
      int idx;
      if (compile_expr(c, expr->data.definition.value, heap, 0) < 0) return -1;
      name_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      name_node->type = AST_STRING;
      name_node->data.string = expr->data.definition.variable->data.variable;
      idx = chunk_add_const(c, name_node);
      if (idx < 0) return -1;
      chunk_emit(c, OP_STORE, idx);
      return 0;
    }

  case AST_ASSIGNMENT:
    /* (set! var val) */
    {
      lizard_ast_node_t *name_node;
      int idx;
      if (compile_expr(c, expr->data.assignment.value, heap, 0) < 0) return -1;
      name_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      name_node->type = AST_STRING;
      name_node->data.string = expr->data.assignment.variable->data.variable;
      idx = chunk_add_const(c, name_node);
      if (idx < 0) return -1;
      chunk_emit(c, OP_SET, idx);
      return 0;
    }

  case AST_BEGIN:
    return compile_sequence(c, expr->data.begin_expressions, heap,
                            tail_position);

  case AST_LAMBDA:
    /* Compile lambda body to a sub-chunk, emit OP_CLOSURE. */
    {
      lizard_bc_chunk_t *sub;
      lz_list_node_t *iter;
      int param_count, i, idx;
      lizard_ast_node_t *chunk_holder;

      sub = chunk_create(heap);
      if (sub == NULL) return -1;

      /* Count and record parameter names. */
      param_count = list_len(expr->data.lambda.parameters);
      /* The last element of the parameter list is the body. */
      if (param_count > 0) param_count--;
      sub->arity = param_count;
      if (param_count > 0) {
        sub->param_names = (const char **)malloc(
            (size_t)param_count * sizeof(const char *));
        i = 0;
        for (iter = expr->data.lambda.parameters->head;
             iter != expr->data.lambda.parameters->nil && i < param_count;
             iter = iter->next) {
          lizard_ast_node_t *p = ((lizard_ast_list_node_t *)iter)->ast;
          if (p->type == AST_SYMBOL) {
            sub->param_names[i] = p->data.variable;
          } else {
            sub->param_names[i] = "?";
          }
          i++;
        }
      }

      /* The body is the last element of the parameter list. */
      {
        lz_list_node_t *last = NULL;
        for (iter = expr->data.lambda.parameters->head;
             iter != expr->data.lambda.parameters->nil;
             iter = iter->next) {
          last = iter;
        }
        if (last != NULL) {
          if (compile_expr(sub, ((lizard_ast_list_node_t *)last)->ast,
                           heap, 1) < 0) {
            return -1;
          }
        } else {
          chunk_emit(sub, OP_NIL, 0);
        }
      }
      chunk_emit(sub, OP_RETURN, 0);

      /* Store the sub-chunk as a constant. We wrap it in an AST node
       * for storage (using a string with a magic prefix). */
      chunk_holder = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      chunk_holder->type = AST_STRING;  /* store chunk pointer via memcpy (C89 safe) */
      memcpy(&chunk_holder->data.string, &sub, sizeof(sub));
      idx = chunk_add_const(c, chunk_holder);
      if (idx < 0) return -1;
      chunk_emit(c, OP_CLOSURE, idx);
      return 0;
    }

  case AST_APPLICATION:
    /* (f arg1 arg2 ...) — general function call. */
    {
      lz_list_node_t *iter;
      int argc = 0;
      if (expr->data.application_arguments == NULL) {
        chunk_emit(c, OP_NIL, 0);
        return 0;
      }
      /* First element is the function, rest are arguments. */
      iter = expr->data.application_arguments->head;
      if (iter == expr->data.application_arguments->nil) {
        chunk_emit(c, OP_NIL, 0);
        return 0;
      }
      /* Check for known built-in operators. */
      {
        lizard_ast_node_t *fn_expr = ((lizard_ast_list_node_t *)iter)->ast;
        if (fn_expr->type == AST_SYMBOL) {
          const char *name = fn_expr->data.variable;
          lz_list_node_t *a1, *a2;
          a1 = iter->next;

          /* Binary arithmetic/comparison. */
          if (a1 != expr->data.application_arguments->nil) {
            a2 = a1->next;
            if (a2 != expr->data.application_arguments->nil &&
                a2->next == expr->data.application_arguments->nil) {
              /* Exactly 2 args. */
              lizard_opcode_t binop = OP_HALT;
              if (strcmp(name, "+") == 0) binop = OP_ADD;
              else if (strcmp(name, "-") == 0) binop = OP_SUB;
              else if (strcmp(name, "*") == 0) binop = OP_MUL;
              else if (strcmp(name, "/") == 0) binop = OP_DIV;
              else if (strcmp(name, "modulo") == 0) binop = OP_MOD;
              else if (strcmp(name, "=") == 0) binop = OP_EQ;
              else if (strcmp(name, "<") == 0) binop = OP_LT;
              else if (strcmp(name, ">") == 0) binop = OP_GT;
              else if (strcmp(name, "cons") == 0) binop = OP_CONS;
              if (binop != OP_HALT) {
                if (compile_expr(c, ((lizard_ast_list_node_t *)a1)->ast, heap, 0) < 0) return -1;
                if (compile_expr(c, ((lizard_ast_list_node_t *)a2)->ast, heap, 0) < 0) return -1;
                chunk_emit(c, binop, 0);
                return 0;
              }
            }
            /* Unary operators. */
            if (a1->next == expr->data.application_arguments->nil) {
              lizard_opcode_t unop = OP_HALT;
              if (strcmp(name, "not") == 0) unop = OP_NOT;
              else if (strcmp(name, "car") == 0) unop = OP_CAR;
              else if (strcmp(name, "cdr") == 0) unop = OP_CDR;
              else if (strcmp(name, "display") == 0) unop = OP_DISPLAY;
              if (unop != OP_HALT) {
                if (compile_expr(c, ((lizard_ast_list_node_t *)a1)->ast, heap, 0) < 0) return -1;
                chunk_emit(c, unop, 0);
                return 0;
              }
            }
            /* Zero-arg operators. */
            if (a1 == expr->data.application_arguments->nil) {
              if (strcmp(name, "newline") == 0) {
                chunk_emit(c, OP_NEWLINE, 0);
                return 0;
              }
            }
          }
        }
      }

      /* General call: compile function, then args, then OP_CALL. */
      if (compile_expr(c, ((lizard_ast_list_node_t *)iter)->ast, heap, 0) < 0) return -1;
      for (iter = iter->next;
           iter != expr->data.application_arguments->nil;
           iter = iter->next) {
        if (compile_expr(c, ((lizard_ast_list_node_t *)iter)->ast, heap, 0) < 0) return -1;
        argc++;
      }
      chunk_emit(c, tail_position ? OP_TAIL_CALL : OP_CALL, argc);
      return 0;
    }

  default:
    /* Unsupported form: store it as a constant and push it.
     * The VM will return it as-is (no evaluation). */
    {
      int idx = chunk_add_const(c, expr);
      if (idx < 0) return -1;
      chunk_emit(c, OP_CONST, idx);
      return 0;
    }
  }
}

/* ---- public compiler entry point ---- */

lizard_bc_chunk_t *lizard_compile(lizard_ast_node_t *expr,
                                   lizard_heap_t *heap) {
  lizard_bc_chunk_t *c = chunk_create(heap);
  if (c == NULL) return NULL;
  if (compile_expr(c, expr, heap, 0) < 0) {
    free(c);
    return NULL;
  }
  chunk_emit(c, OP_HALT, 0);
  return c;
}

/* ---- VM execution ---- */

/* Bytecode closure: a chunk + captured environment. */
typedef struct {
  lizard_bc_chunk_t *chunk;
  lizard_env_t *env;
} bc_closure_t;

/* Tag for identifying bytecode closures stored in AST_PRIMITIVE nodes. */
#define BC_CLOSURE_TAG ((lizard_primitive_func_t)(void *)0xBC01)

static lizard_ast_node_t *make_bc_closure(lizard_bc_chunk_t *chunk,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  bc_closure_t *cl = (bc_closure_t *)malloc(sizeof(bc_closure_t));
  cl->chunk = chunk;
  cl->env = env;
  node->type = AST_PROMISE;
  node->data.promise.forced = 1;  /* tag: this is a BC closure, not a real promise */
  memcpy(&node->data.promise.value, &cl, sizeof(cl));
  return node;
}

/* BC closures are stored as AST_PROMISE nodes with forced=1 as a tag.
 * The closure pointer is stored in data.promise.value via memcpy. */
static int is_bc_closure(lizard_ast_node_t *node) {
  return node != NULL && node->type == AST_PROMISE && node->data.promise.forced;
}

/* Internal VM runner — shared by exec and exec_profiled. */
static lizard_ast_node_t *vm_run(lizard_bc_chunk_t *chunk,
                                  lizard_env_t *env,
                                  lizard_heap_t *heap,
                                  lizard_vm_profile_t *profile);

lizard_ast_node_t *lizard_vm_exec(lizard_bc_chunk_t *chunk,
                                   lizard_env_t *env,
                                   lizard_heap_t *heap) {
  return vm_run(chunk, env, heap, NULL);
}

lizard_ast_node_t *lizard_vm_exec_profiled(lizard_bc_chunk_t *chunk,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap,
                                            lizard_vm_profile_t *profile) {
  return vm_run(chunk, env, heap, profile);
}

static lizard_ast_node_t *vm_run(lizard_bc_chunk_t *chunk,
                                  lizard_env_t *env,
                                  lizard_heap_t *heap,
                                  lizard_vm_profile_t *profile) {
  lizard_vm_t vm;
  vm.chunk = chunk;
  vm.ip = 0;
  vm.sp = 0;
  vm.env = env;
  vm.heap = heap;
  vm.profile = profile;

  for (;;) {
    lizard_instruction_t instr;
    if (vm.ip >= vm.chunk->code_count) break;
    instr = vm.chunk->code[vm.ip++];

    /* Profiling: count each instruction. */
    if (profile != NULL) {
      profile->total_instructions++;
      if ((int)instr.op < LIZARD_OPCODE_COUNT) {
        profile->opcode_counts[(int)instr.op]++;
      }
      if (instr.op == OP_CALL) profile->total_calls++;
      if (instr.op == OP_TAIL_CALL) {
        profile->total_calls++;
        profile->tail_calls++;
      }
      /* Trace: print each instruction + stack. */
      if (profile->trace) {
        int si;
        printf("  [%04d] %-8s %d  | stack(%d):",
               vm.ip - 1,
               lizard_bc_opcode_name(instr.op),
               instr.arg, vm.sp);
        for (si = 0; si < vm.sp && si < 6; si++) {
          printf(" ");
          lizard_fprint_value(stdout, vm.stack[si]);
        }
        if (vm.sp > 6) printf(" ...");
        printf("\n");
      }
    }

    switch (instr.op) {
    case OP_CONST:
      vm.stack[vm.sp++] = vm.chunk->constants[instr.arg];
      break;
    case OP_NIL:
      vm.stack[vm.sp++] = lizard_make_nil(heap);
      break;
    case OP_TRUE:
      vm.stack[vm.sp++] = lizard_make_bool(heap, 1);
      break;
    case OP_FALSE:
      vm.stack[vm.sp++] = lizard_make_bool(heap, 0);
      break;
    case OP_LOAD: {
      const char *name = vm.chunk->constants[instr.arg]->data.string;
      lizard_ast_node_t *val = lizard_env_lookup(vm.env, name);
      if (val == NULL) {
        vm.stack[vm.sp++] = lizard_make_error(heap, LIZARD_ERROR_UNBOUND_SYMBOL);
        return vm.stack[vm.sp - 1];
      }
      vm.stack[vm.sp++] = val;
      break;
    }
    case OP_STORE: {
      const char *name = vm.chunk->constants[instr.arg]->data.string;
      lizard_ast_node_t *val = vm.stack[--vm.sp];
      lizard_env_define(heap, vm.env, name, val);
      vm.stack[vm.sp++] = val;
      break;
    }
    case OP_SET: {
      const char *name = vm.chunk->constants[instr.arg]->data.string;
      lizard_ast_node_t *val = vm.stack[--vm.sp];
      lizard_env_set(vm.env, name, val);
      vm.stack[vm.sp++] = val;
      break;
    }
    case OP_POP:
      if (vm.sp > 0) vm.sp--;
      break;
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD: {
      lizard_ast_node_t *b = vm.stack[--vm.sp];
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      lizard_ast_node_t *r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      r->type = AST_NUMBER;
      mpz_init(r->data.number);
      if (a->type == AST_NUMBER && b->type == AST_NUMBER) {
        switch (instr.op) {
        case OP_ADD: mpz_add(r->data.number, a->data.number, b->data.number); break;
        case OP_SUB: mpz_sub(r->data.number, a->data.number, b->data.number); break;
        case OP_MUL: mpz_mul(r->data.number, a->data.number, b->data.number); break;
        case OP_DIV:
          if (mpz_sgn(b->data.number) == 0) {
            return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
          }
          mpz_tdiv_q(r->data.number, a->data.number, b->data.number);
          break;
        case OP_MOD:
          if (mpz_sgn(b->data.number) == 0) {
            return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
          }
          mpz_mod(r->data.number, a->data.number, b->data.number);
          break;
        default: break;
        }
      }
      vm.stack[vm.sp++] = r;
      break;
    }
    case OP_EQ: {
      lizard_ast_node_t *b = vm.stack[--vm.sp];
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      int eq = 0;
      if (a->type == AST_NUMBER && b->type == AST_NUMBER) {
        eq = (mpz_cmp(a->data.number, b->data.number) == 0);
      } else if (a->type == AST_BOOL && b->type == AST_BOOL) {
        eq = (a->data.boolean == b->data.boolean);
      } else if (a->type == AST_NIL && b->type == AST_NIL) {
        eq = 1;
      }
      vm.stack[vm.sp++] = lizard_make_bool(heap, eq);
      break;
    }
    case OP_LT: case OP_GT: {
      lizard_ast_node_t *b = vm.stack[--vm.sp];
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      int cmp = 0;
      if (a->type == AST_NUMBER && b->type == AST_NUMBER) {
        cmp = mpz_cmp(a->data.number, b->data.number);
      }
      vm.stack[vm.sp++] = lizard_make_bool(heap,
          instr.op == OP_LT ? cmp < 0 : cmp > 0);
      break;
    }
    case OP_NOT: {
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      int falsy = (a->type == AST_BOOL && !a->data.boolean) ||
                  a->type == AST_NIL;
      vm.stack[vm.sp++] = lizard_make_bool(heap, falsy);
      break;
    }
    case OP_CONS: {
      lizard_ast_node_t *b = vm.stack[--vm.sp];
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = a;
      p->data.pair.cdr = b;
      vm.stack[vm.sp++] = p;
      break;
    }
    case OP_CAR: {
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      if (a->type != AST_PAIR) {
        return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGT);
      }
      vm.stack[vm.sp++] = a->data.pair.car;
      break;
    }
    case OP_CDR: {
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      if (a->type != AST_PAIR) {
        return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGT);
      }
      vm.stack[vm.sp++] = a->data.pair.cdr;
      break;
    }
    case OP_JUMP:
      vm.ip = instr.arg;
      break;
    case OP_JUMP_IF_FALSE: {
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      if ((a->type == AST_BOOL && !a->data.boolean) ||
          a->type == AST_NIL) {
        vm.ip = instr.arg;
      }
      break;
    }
    case OP_CLOSURE: {
      /* Create a bytecode closure capturing the current env. */
      lizard_bc_chunk_t *sub;
      memcpy(&sub, &vm.chunk->constants[instr.arg]->data.string, sizeof(sub));
      vm.stack[vm.sp++] = make_bc_closure(sub, vm.env, heap);
      break;
    }
    case OP_CALL:
    case OP_TAIL_CALL: {
      int argc = instr.arg;
      lizard_ast_node_t *fn;
      lizard_ast_node_t *args_arr[64];
      int i;
      int is_tail = (instr.op == OP_TAIL_CALL);
      /* Pop args (in reverse). */
      for (i = argc - 1; i >= 0; i--) {
        args_arr[i] = vm.stack[--vm.sp];
      }
      fn = vm.stack[--vm.sp];

      if (is_bc_closure(fn)) {
        /* Execute bytecode closure. */
        bc_closure_t *cl;
        lizard_env_t *call_env;
        memcpy(&cl, &fn->data.promise.value, sizeof(cl));
        call_env = lizard_env_create(heap, cl->env);
        /* Bind parameters. */
        for (i = 0; i < cl->chunk->arity && i < argc; i++) {
          lizard_env_define(heap, call_env, cl->chunk->param_names[i],
                            args_arr[i]);
        }
        if (is_tail) {
          /* TCO: replace the current frame — no recursion, no stack
           * growth. The VM loop restarts with the callee's chunk. */
          vm.chunk = cl->chunk;
          vm.env = call_env;
          vm.ip = 0;
          vm.sp = 0;
          continue;  /* restart the dispatch loop */
        }
        /* Non-tail call: recurse (pushes a C stack frame). */
        vm.stack[vm.sp++] = vm_run(cl->chunk, call_env, heap, profile);
      } else if (fn->type == AST_PRIMITIVE) {
        /* Call native primitive. Build an lz_list_t of args. */
        lz_list_t *arg_list;
        arg_list = list_create_alloc(lizard_heap_alloc,
                                     lizard_heap_free);
        for (i = 0; i < argc; i++) {
          lizard_ast_list_node_t *n;
          n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          n->ast = args_arr[i];
          list_append(arg_list, &n->node);
        }
        vm.stack[vm.sp++] = fn->data.primitive(arg_list, vm.env, heap);
      } else {
        /* Unsupported callable — error. */
        return lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY);
      }
      break;
    }
    case OP_RETURN:
      if (vm.sp > 0) return vm.stack[vm.sp - 1];
      return lizard_make_nil(heap);
    case OP_DISPLAY: {
      lizard_ast_node_t *a = vm.stack[--vm.sp];
      lizard_fprint_value(stdout, a);
      vm.stack[vm.sp++] = a;
      break;
    }
    case OP_NEWLINE:
      printf("\n");
      vm.stack[vm.sp++] = lizard_make_nil(heap);
      break;
    case OP_HALT:
      if (vm.sp > 0) return vm.stack[vm.sp - 1];
      return lizard_make_nil(heap);
    }
  }
  if (vm.sp > 0) return vm.stack[vm.sp - 1];
  return lizard_make_nil(heap);
}
