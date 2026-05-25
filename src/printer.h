#ifndef PRINTER_H
#define PRINTER_H

#include "lizard.h"

void lizard_print_indent(int depth);
void lizard_print_ast(lizard_ast_node_t *node, int depth);
void lizard_fprint_ast(FILE *fp, lizard_ast_node_t *node, int depth);

/* Pretty value printer: emits values in Scheme syntax,
   e.g. 3628800, (1 2 3), #t, "hi", <procedure>. */
void lizard_print_value(lizard_ast_node_t *node);
void lizard_fprint_value(FILE *fp, lizard_ast_node_t *node);

#endif /* PRINTER_H */
