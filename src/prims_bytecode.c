/* src/prims_bytecode.c -- bytecode/profiling primitives.
 *
 * Split out from primitives.c as part of Recoverable Core Phase 1B.
 */
#include "bytecode.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"
#include "printer.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int single_arg(lz_list_t *args) {
  return args != NULL && args->head != args->nil && args->head->next == args->nil;
}

static lizard_ast_node_t *bytecode_compile_error(lizard_heap_t *heap) {
  (void)lizard_compile_last_error();
  return lizard_make_error(heap, LIZARD_ERROR_BYTECODE_UNSUPPORTED);
}

/* Phase E: bytecode VM primitives. */
/* (vm-eval expr) — compile expr to bytecode and execute on the VM. */
lizard_ast_node_t *lizard_primitive_vm_eval(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_bc_chunk_t *chunk;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = lizard_reparse_datum(((lizard_ast_list_node_t *)args->head)->ast, heap);
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return bytecode_compile_error(heap);
  }
  return lizard_vm_exec(chunk, env, heap);
}

/* (disassemble expr) — compile expr and print the bytecode. */
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
  expr = lizard_reparse_datum(((lizard_ast_list_node_t *)args->head)->ast, heap);
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return bytecode_compile_error(heap);
  }
  printf("--- bytecode: %d instructions, %d constants ---\n",
         chunk->code_count, chunk->const_count);
  for (i = 0; i < chunk->code_count; i++) {
    printf("  %04d  %-8s %d", i, lizard_bc_opcode_name(chunk->code[i].op),
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

#include <time.h>

/* (vm-time expr) — compile + execute via bytecode VM, print elapsed time. */
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
  expr = lizard_reparse_datum(((lizard_ast_list_node_t *)args->head)->ast, heap);
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return bytecode_compile_error(heap);
  }
  start = clock();
  result = lizard_vm_exec(chunk, env, heap);
  end = clock();
  elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;
  printf("; vm-time: %.6f s\n", elapsed);
  return result;
}

/* (time-eval expr) — evaluate via tree-walker, print elapsed time. */
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

/* (profile expr) — compile + execute with full instruction profiling. */
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
  expr = lizard_reparse_datum(((lizard_ast_list_node_t *)args->head)->ast, heap);
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return bytecode_compile_error(heap);
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
      printf("    %-8s %lu\n", lizard_bc_opcode_name((lizard_opcode_t)i),
             (unsigned long)prof.opcode_counts[i]);
    }
  }
  printf("---\n");
  return result;
}

