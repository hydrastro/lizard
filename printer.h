#ifndef PRINTER_H
#define PRINTER_H

#include "lizard.h"

void lizard_print_indent(int depth);
void lizard_print_ast(lizard_ast_node_t *node, int depth);
void lizard_fprint_ast(FILE *fp, lizard_ast_node_t *node, int depth);

#endif /* PRINTER_H */
