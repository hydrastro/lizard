#ifndef LIZARD_KERNEL_SEXP_H
#define LIZARD_KERNEL_SEXP_H

#include "kernel.h"

kterm_t *lizard_kernel_sexp_to_kterm(lizard_heap_t *heap,
                                     lizard_ast_node_t *expr);
kterm_t *lizard_kernel_sexp_to_kterm_in(lizard_heap_t *heap,
                                        lizard_ast_node_t *e,
                                        const char **names, int n);

#endif /* LIZARD_KERNEL_SEXP_H */
