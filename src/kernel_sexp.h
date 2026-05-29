#ifndef LIZARD_KERNEL_SEXP_H
#define LIZARD_KERNEL_SEXP_H

#include "kernel.h"

kterm_t *lizard_kernel_sexp_to_kterm(lizard_heap_t *heap,
                                     lizard_ast_node_t *expr);

#endif /* LIZARD_KERNEL_SEXP_H */
