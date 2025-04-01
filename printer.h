#ifndef PRINTER_H
#define PRINTER_H

#include "lizard.h"

void print_ast(lizard_ast_node_t *node, int depth);
void fprint_ast(FILE *fp, lizard_ast_node_t *node, int depth);

#endif /* PRINTER_H */
