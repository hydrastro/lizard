/* prims_bytecode.c — extracted from primitives.c (#7 monolith split).
 * Registration stays in primitives.c; definitions linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "prims_shared.h"
#include "bytecode.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

static const char *opcode_name(lizard_opcode_t op) {
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
lizard_ast_node_t *lizard_primitive_vm_eval(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_bc_chunk_t *chunk;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  /* Strict bytecode: the VM only executes the runtime fragment.  Type-theory
   * terms (Pi/Sigma/U/Id/... through extensions) are not runtime values and
   * must be rejected rather than silently compiled. */
  if (expr != NULL && expr->type >= AST_TT_PI &&
      expr->type <= AST_TT_EXTENSION) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_vm_exec(chunk, env, heap);
}
lizard_ast_node_t *lizard_primitive_disassemble(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_bc_chunk_t *chunk;
  int i;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  printf("--- bytecode: %d instructions, %d constants ---\n",
         chunk->code_count, chunk->const_count);
  for (i = 0; i < chunk->code_count; i++) {
    printf("  %04d  %-8s %d", i, opcode_name(chunk->code[i].op),
           chunk->code[i].arg);
    /* Show constant value for CONST/LOAD/STORE instructions. */
    if ((chunk->code[i].op == OP_CONST ||
         chunk->code[i].op == OP_LOAD ||
         chunk->code[i].op == OP_STORE) &&
        chunk->code[i].arg < chunk->const_count) {
      printf("  ; ");
      lizard_fprint_value(stdout, chunk->constants[chunk->code[i].arg]);
    }
    printf("\n");
  }
  printf("---\n");
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_vm_time(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  lizard_bc_chunk_t *chunk;
  clock_t start, end;
  double elapsed;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  start = clock();
  result = lizard_vm_exec(chunk, env, heap);
  end = clock();
  elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;
  printf("; vm-time: %.6f s\n", elapsed);
  return result;
}
lizard_ast_node_t *lizard_primitive_time_eval(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  clock_t start, end;
  double elapsed;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  start = clock();
  result = lizard_eval(expr, env, heap, lizard_identity_cont);
  end = clock();
  elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;
  printf("; time-eval: %.6f s\n", elapsed);
  return result;
}
lizard_ast_node_t *lizard_primitive_profile(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  lizard_bc_chunk_t *chunk;
  lizard_vm_profile_t prof;
  clock_t start, end;
  int i;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  memset(&prof, 0, sizeof(prof));
  start = clock();
  result = lizard_vm_exec_profiled(chunk, env, heap, &prof);
  end = clock();
  prof.elapsed_seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;

  printf("--- profile ---\n");
  printf("  elapsed:      %.6f s\n", prof.elapsed_seconds);
  printf("  instructions: %lu\n", (unsigned long)prof.total_instructions);
  printf("  calls:        %lu\n", (unsigned long)prof.total_calls);
  printf("  tail calls:   %lu\n", (unsigned long)prof.tail_calls);
  if (prof.total_instructions > 0) {
    printf("  MIPS:         %.1f\n",
           (double)prof.total_instructions / prof.elapsed_seconds / 1e6);
  }
  printf("  top opcodes:\n");
  for (i = 0; i < LIZARD_OPCODE_COUNT; i++) {
    if (prof.opcode_counts[i] > 0) {
      printf("    %-8s %lu\n", opcode_name((lizard_opcode_t)i),
             (unsigned long)prof.opcode_counts[i]);
    }
  }
  printf("---\n");
  return result;
}
