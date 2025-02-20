#ifndef LIZARD_LANG_EN_H
#define LIZARD_LANG_EN_H
#include "lizard.h"

#define LIZARD_ERROR_MESSAGES_EN                                               \
  X(LIZARD_ERROR_NONE, "Everything's good.")                                   \
  X(LIZARD_ERROR_UNBOUND_SYMBOL, "Error: unbound symbol.")                     \
  X(LIZARD_ERROR_INVALID_FUNCTION_DEF,                                         \
    "Error: invalid function definition syntax.")                              \
  X(LIZARD_ERROR_INVALID_FUNCTION_NAME,                                        \
    "Error: function name must be a symbol.")                                  \
  X(LIZARD_ERROR_INVALID_PARAMETER, "Error: parameter is not a symbol.")       \
  X(LIZARD_ERROR_INVALID_DEF, "Error: invalid definition syntax.")             \
  X(LIZARD_ERROR_ASSIGNMENT, "Error: set! expects a symbol as variable.")      \
  X(LIZARD_ERROR_ASSIGNMENT_UNBOUND, "Error: unbound symbol in set.")          \
  X(LIZARD_ERROR_INVALID_MACRO_NAME, "Error: macro name must be a symbol.")    \
  X(LIZARD_ERROR_NODE_TYPE, "Error: unknown AST node type in eval.")           \
  X(LIZARD_ERROR_LAMBDA_PARAMS,                                                \
    "Error: lambda parameters are not in application form.")                   \
  X(LIZARD_ERROR_VARIADIC_UNFOLLOWED,                                          \
    "Error: variadic marker must be followed by a parameter name.")            \
  X(LIZARD_ERROR_VARIADIC_SYMBOL,                                              \
    "Error: variadic parameter must be a symbol.")                             \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS,                                            \
    "Error: lambda arity mismatch (not enough arguments).")                    \
  X(LIZARD_ERROR_LAMBDA_PARAMETER, "Error: lambda parameter is not a symbol.") \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE,                                            \
    "Error: lambda arity mismatch (too many arguments).")                      \
  X(LIZARD_ERROR_INVALID_APPLY, "Error: Attempt to apply a non-function.")     \
  X(LIZARD_ERROR_PLUS, "Error: + expects numbers.")                            \
  X(LIZARD_ERROR_MINUS_ARGC, "Error: '-' expects at least one argument.")      \
  X(LIZARD_ERROR_MINUS_ARGT, "Error: '-' expects number arguments.")           \
  X(LIZARD_ERROR_MINUS_ARGT_2, "Error: '-' expects number arguments.")         \
  X(LIZARD_ERROR_MUL, "Error: '*' expects number arguments.")                  \
  X(LIZARD_ERROR_DIV_ARGC, "Error: '/' expects at least two arguments.")       \
  X(LIZARD_ERROR_DIV_ARGT, "Error: '/' expects number arguments.")             \
  X(LIZARD_ERROR_DIV_ARGT_2, "Error: '/' expects number arguments.")           \
  X(LIZARD_ERROR_DIV_ZERO, "Error: division by zero.")                         \
  X(LIZARD_ERROR_EQ_ARGC, "Error: '=' expects at least two arguments.")        \
  X(LIZARD_ERROR_POW_ARGC, "Error: '^' expects two arguments.")                \
  X(LIZARD_ERROR_POW_ARGT, "Error: '^' expects number arguments.")             \
  X(LIZARD_ERROR_LT_ARGC, "Error: '<' expects two arguments.")                 \
  X(LIZARD_ERROR_LT_ARGT, "Error: '<' expects number arguments.")              \
  X(LIZARD_ERROR_LT_ARGT_2, "Error: '<' expects number arguments.")            \
  X(LIZARD_ERROR_LE_ARGC, "Error: '<=' expects two arguments.")                \
  X(LIZARD_ERROR_LE_ARGT, "Error: '<=' expects number arguments.")             \
  X(LIZARD_ERROR_LE_ARGT_2, "Error: '<=' expects number arguments.")           \
  X(LIZARD_ERROR_GT_ARGC, "Error: '>' expects two arguments.")                 \
  X(LIZARD_ERROR_GT_ARGT, "Error: '>' expects number arguments.")              \
  X(LIZARD_ERROR_GT_ARGT_2, "Error: '>' expects number arguments.")            \
  X(LIZARD_ERROR_GE_ARGC, "Error: '>=' expects two arguments.")                \
  X(LIZARD_ERROR_GE_ARGT, "Error: '>=' expects number arguments.")             \
  X(LIZARD_ERROR_GE_ARGT_2, "Error: '>=' expects number arguments.")           \
  X(LIZARD_ERROR_MOD_ARGC, "Error: '%' expects two arguments.")                \
  X(LIZARD_ERROR_MOD_ARGT, "Error: '%' expects number arguments.")             \
  X(LIZARD_ERROR_MOD_ARGT_2, "Error: '%' expects number arguments.")           \
  X(LIZARD_ERROR_CONS_ARGC, "Error: 'cons' expects exactly two arguments.")    \
  X(LIZARD_ERROR_CAR_ARGC, "Error: 'car' expects exactly one argument.")       \
  X(LIZARD_ERROR_CAR_ARGT, "Error: 'car' expects a list.")                     \
  X(LIZARD_ERROR_CAR_NIL, "Error: 'car' called on an empty list.")             \
  X(LIZARD_ERROR_CDR_ARGC, "Error: 'cdr' expects exactly one argument.")       \
  X(LIZARD_ERROR_CDR_ARGT, "Error: 'cdr' expects a list.")                     \
  X(LIZARD_ERROR_CDR_NIL, "Error: 'cdr' called on an empty list.")             \
  X(LIZARD_ERROR_TOKENS_ARGC,                                                  \
    "Error: 'print-tokens' expects exactly one argument.")                     \
  X(LIZARD_ERROR_TOKENS_ARGT,                                                  \
    "Error: 'print-tokens' expects a string argument.")                        \
  X(LIZARD_ERROR_EVAL_ARGC, "Error: eval expects exactly one argument.")       \
  X(LIZARD_ERROR_UNQUOTE, "Error: unquote not allowed outside quasiquote.")    \
  X(LIZARD_ERROR_UNQUOTE_ARGC, "Error: unquote requires an argument.")         \
  X(LIZARD_ERROR_INVALID_SPLICE, "Error: invalid splice.")                     \
  X(LIZARD_ERROR_CALLCC_ARGC, "Error: 'call/cc' expects an argument.")         \
  X(LIZARD_ERROR_INVALID_FORCE, "Error: invalid force")                        \
  X(LIZARD_ERROR_INVALID_DELAY, "Error: invalid delay.")

#define LIZARD_ERROR_MESSAGES                                                  \
  X(LIZARD_ERROR_MESSAGES_EN, lizard_error_messages_lang_en)

#endif /* LIZARD_LANG_EN_H */