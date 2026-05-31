/* lzrt.h — runtime support for the Phase 9 native (compile-to-C) backend.
 *
 * Compiled programs reuse lizard's value representation (lizard_ast_node_t*),
 * its primitives, its numeric tower, its printer and its conservative GC, so a
 * compiled program is observably identical to the interpreted one for the
 * supported language subset.  This header is the *only* lizard header a
 * generated .c file includes; everything else is hidden behind these calls.
 *
 * Calling convention.  A compiled procedure is an AST_COMPILED_PROC whose
 * `entry` has signature
 *     lizard_ast_node_t *entry(freevars, argv, argc)
 * Proper tail calls are trampolined: in tail position a body returns
 * lzrt_tailcall(fn, argv, argc) (argv heap-allocated so it survives the
 * frame's return); lzrt_apply drives the trampoline to a real value.
 */
#ifndef LIZARD_LZRT_H
#define LIZARD_LZRT_H

#include "lizard_internal.h"

/* One-time runtime/heap/global-environment setup.  Call once at program start. */
void lzrt_init(void);
lizard_env_t *lzrt_global_env(void);

/* ---- value constructors (literals) ---- */
lizard_ast_node_t *lzrt_int(long v);
lizard_ast_node_t *lzrt_int_str(const char *decimal); /* arbitrary-precision literal */
lizard_ast_node_t *lzrt_real(double v);
lizard_ast_node_t *lzrt_bool(int v);
lizard_ast_node_t *lzrt_nil(void);
lizard_ast_node_t *lzrt_string(const char *s);
lizard_ast_node_t *lzrt_symbol(const char *s);
lizard_ast_node_t *lzrt_cons(lizard_ast_node_t *a, lizard_ast_node_t *d);

/* ---- environment ---- */
/* Look up a global/primitive by name (top-level reference). Errors if unbound. */
lizard_ast_node_t *lzrt_global_ref(const char *name);
/* Bind a top-level name (compiled `define` at the top level). */
void lzrt_define_global(const char *name, lizard_ast_node_t *value);

/* ---- procedures ---- */
/* Build a compiled closure capturing `nfree` free-variable values. The values
 * array is copied into a fresh GC-scanned heap array. */
lizard_ast_node_t *lzrt_closure(
    lizard_ast_node_t *(*entry)(lizard_ast_node_t **, lizard_ast_node_t **, long),
    lizard_ast_node_t **freevar_values, int nfree, int arity, int variadic);

/* Full application: drives the trampoline and returns a real value. argv may be
 * a caller stack array (it is consumed synchronously). */
lizard_ast_node_t *lzrt_apply(lizard_ast_node_t *fn, lizard_ast_node_t **argv,
                              long argc);
/* Tail application: record a pending call and return the trampoline sentinel.
 * argv MUST be heap-allocated (lzrt_argv) so it outlives the returning frame. */
lizard_ast_node_t *lzrt_tailcall(lizard_ast_node_t *fn, lizard_ast_node_t **argv,
                                 long argc);

/* Allocate a heap argv array of length n (GC-scanned); for tail calls. */
lizard_ast_node_t **lzrt_argv(long n);

/* ---- helpers ---- */
int lzrt_truthy(lizard_ast_node_t *v);          /* everything but #f is true */
lizard_ast_node_t *lzrt_error(const char *msg); /* make an error value */

#endif /* LIZARD_LZRT_H */
