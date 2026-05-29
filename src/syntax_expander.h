#ifndef LIZARD_SYNTAX_EXPANDER_H
#define LIZARD_SYNTAX_EXPANDER_H

#include "env.h"
#include "expansion_context.h"
#include "surface_term.h"

/* Phase 2H scaffold: an adapter around the existing macro expander.
 *
 * This does not change macro semantics. It lets callers pass a SurfaceTerm
 * plus an ExpansionContext, receive the old expanded runtime AST, and also get
 * a new SurfaceTerm that preserves untrusted syntax metadata and records a
 * trace event. */
int lizard_syntax_expand_surface(lizard_expansion_context_t *context,
                                 lizard_env_t *env,
                                 const lizard_surface_term_t *input,
                                 lizard_surface_term_t **out_surface,
                                 lizard_ast_node_t **out_ast);

int lizard_syntax_expand_ast(lizard_expansion_context_t *context,
                             lizard_env_t *env,
                             lizard_ast_node_t *input,
                             lizard_surface_term_t **out_surface,
                             lizard_ast_node_t **out_ast);

#endif /* LIZARD_SYNTAX_EXPANDER_H */
