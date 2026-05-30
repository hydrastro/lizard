/* src/bytecode.h — Bytecode compiler and stack-based VM (Phase E).
 *
 * lizard compiles Scheme AST to a flat bytecode representation,
 * then executes it on a stack machine. This avoids the overhead of
 * recursive tree-walking for hot paths.
 *
 * The instruction set is intentionally small. Complex forms (macros,
 * call/cc, type-theory nodes) fall back to the tree-walking evaluator.
 */
#ifndef LIZARD_BYTECODE_H
#define LIZARD_BYTECODE_H

#include "lizard_internal.h"
#include "env.h"

/* ---- opcodes ---- */

typedef enum {
  OP_CONST,          /* push constants[arg] */
  OP_NIL,            /* push nil */
  OP_TRUE,           /* push #t */
  OP_FALSE,          /* push #f */
  OP_LOAD,           /* push env-lookup(constants[arg]) where arg indexes a symbol name */
  OP_STORE,          /* pop value, define constants[arg] = value in env */
  OP_SET,            /* pop value, set! constants[arg] = value in env */
  OP_POP,            /* discard top of stack */
  OP_ADD,            /* pop two, push sum */
  OP_SUB,            /* pop two, push difference */
  OP_MUL,            /* pop two, push product */
  OP_DIV,            /* pop two, push quotient */
  OP_MOD,            /* pop two, push remainder */
  OP_EQ,             /* pop two, push (= a b) */
  OP_LT,             /* pop two, push (< a b) */
  OP_GT,             /* pop two, push (> a b) */
  OP_NOT,            /* pop one, push (not x) */
  OP_CONS,           /* pop two, push (cons a b) */
  OP_CAR,            /* pop one, push (car x) */
  OP_CDR,            /* pop one, push (cdr x) */
  OP_JUMP,           /* unconditional jump to arg */
  OP_JUMP_IF_FALSE,  /* pop, jump to arg if #f or nil */
  OP_CALL,           /* call: arg = number of arguments */
  OP_TAIL_CALL,      /* tail call: arg = number of arguments */
  OP_RETURN,         /* return top of stack */
  OP_CLOSURE,        /* create closure from chunk constants[arg] */
  OP_DISPLAY,        /* pop, display, push result */
  OP_NEWLINE,        /* print newline, push nil */
  OP_HALT            /* stop VM */
} lizard_opcode_t;

/* ---- bytecode instruction ---- */

typedef struct {
  lizard_opcode_t op;
  int arg;           /* operand (meaning depends on opcode) */
} lizard_instruction_t;

/* ---- bytecode chunk ---- */

#define LIZARD_BC_MAX_CODE      4096
#define LIZARD_BC_MAX_CONSTANTS 256

typedef struct lizard_bc_chunk {
  lizard_instruction_t code[LIZARD_BC_MAX_CODE];
  int code_count;
  lizard_ast_node_t *constants[LIZARD_BC_MAX_CONSTANTS];
  int const_count;
  int arity;         /* for lambda chunks: number of parameters */
  const char **param_names;  /* parameter names (arity-length array) */
} lizard_bc_chunk_t;

/* ---- VM state ---- */

#define LIZARD_VM_STACK_SIZE 1024
#define LIZARD_OPCODE_COUNT 30

typedef struct {
  size_t total_instructions;
  size_t opcode_counts[LIZARD_OPCODE_COUNT];
  size_t total_calls;
  size_t tail_calls;
  double elapsed_seconds;
  int trace;  /* if nonzero, print each instruction + stack */
} lizard_vm_profile_t;

typedef struct {
  lizard_ast_node_t *stack[LIZARD_VM_STACK_SIZE];
  int sp;            /* stack pointer (index of next free slot) */
  lizard_bc_chunk_t *chunk;
  int ip;            /* instruction pointer */
  lizard_env_t *env;
  lizard_heap_t *heap;
  lizard_vm_profile_t *profile;  /* NULL = no profiling */
} lizard_vm_t;

/* ---- public API ---- */

/* Compile an AST expression to a bytecode chunk. Returns NULL on
 * failure (unsupported form). The chunk is heap-allocated. */
lizard_bc_chunk_t *lizard_compile(lizard_ast_node_t *expr,
                                   lizard_heap_t *heap);

/* Execute a bytecode chunk in the given environment. Returns the
 * result value (top of stack after OP_HALT). */
lizard_ast_node_t *lizard_vm_exec(lizard_bc_chunk_t *chunk,
                                   lizard_env_t *env,
                                   lizard_heap_t *heap);

/* Execute with profiling. Fills the profile struct with counters. */
lizard_ast_node_t *lizard_vm_exec_profiled(lizard_bc_chunk_t *chunk,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap,
                                            lizard_vm_profile_t *profile);

/* Return the human-readable name of an opcode. */
const char *lizard_bc_opcode_name(lizard_opcode_t op);

#endif /* LIZARD_BYTECODE_H */
