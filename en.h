#ifndef LIZARD_LANG_EN_H
#define LIZARD_LANG_EN_H
#include "lizard.h"

#define LIZARD_ERROR_MESSAGES_EN                                               \
  X(LIZARD_ERROR_NONE, "Everything's good.")                                   \
  X(LIZARD_ERROR_UNDEFINED, "Something went wrong—but I'm not sure what!")     \
  X(LIZARD_ERROR_UNBOUND_SYMBOL, "Error: unbound symbol.")                     \
  X(LIZARD_ERROR_INVALID_FUNCTION_DEF,                                         \
    "Error: invalid function definition syntax.")                              \
  X(LIZARD_ERROR_INVALID_FUNCTION_NAME,                                        \
    "Error: function name must be a symbol.")                                  \
  X(LIZARD_ERROR_INVALID_PARAMETER, "Error: parameter is not a symbol.")       \
  X(LIZARD_ERROR_INVALID_DEF, "Error: invalid definition syntax.")             \
  X(LIZARD_ERROR_ASSIGNMENT, "Error: set! expects a symbol as variable.")      \
  X(LIZARD_ERROR_ASSIGNMENT_UNBOUND, "Error: unbound symbol in set!")          \
  X(LIZARD_ERROR_INVALID_MACRO_NAME, "Error: macro name must be a symbol.")    \
  X(LIZARD_ERROR_NODE_TYPE, "Error: unknown AST node type in eval.")           \
  X(LIZARD_ERROR_LAMBDA_PARAMS,                                                \
    "Error: lambda parameters are not in application form.")                   \
  X(LIZARD_ERROR_LAMBDA_PARAMS_2,                                              \
    "Error: alternative lambda parameter format is wrong.")                    \
  X(LIZARD_ERROR_VARIADIC_UNFOLLOWED,                                          \
    "Error: variadic marker must be followed by a parameter name.")            \
  X(LIZARD_ERROR_VARIADIC_SYMBOL,                                              \
    "Error: variadic parameter must be a symbol.")                             \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS,                                            \
    "Error: lambda arity mismatch (not enough arguments).")                    \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS_2,                                          \
    "Error: not enough arguments for lambda (alternative check).")             \
  X(LIZARD_ERROR_LAMBDA_PARAMETER, "Error: lambda parameter is not a symbol.") \
  X(LIZARD_ERROR_LAMBDA_PARAMETER_2,                                           \
    "Error: alternative lambda parameter is not a symbol.")                    \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE,                                            \
    "Error: lambda arity mismatch (too many arguments).")                      \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE_2,                                          \
    "Error: too many arguments for lambda (alternative check).")               \
  X(LIZARD_ERROR_INVALID_APPLY, "Error: attempt to apply a non-function.")     \
  X(LIZARD_ERROR_INVALID_APPLY_2,                                              \
    "Error: cannot apply this value; it's not callable.")                      \
  X(LIZARD_ERROR_PLUS_ARGC,                                                    \
    "Error: '+' got an incorrect number of arguments.")                        \
  X(LIZARD_ERROR_PLUS_ARGT, "Error: '+' expects number arguments.")            \
  X(LIZARD_ERROR_MINUS_ARGC, "Error: '-' expects at least one argument.")      \
  X(LIZARD_ERROR_MINUS_ARGT, "Error: '-' expects number arguments.")           \
  X(LIZARD_ERROR_MINUS_ARGT_2, "Error: '-' expects number arguments.")         \
  X(LIZARD_ERROR_MUL_ARGC, "Error: '*' got an incorrect number of arguments.") \
  X(LIZARD_ERROR_MUL_ARGT, "Error: '*' expects number arguments.")             \
  X(LIZARD_ERROR_MUL_ARGT_2, "Error: '*' expects number arguments.")           \
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
  X(LIZARD_ERROR_MOD_ARGC, "Error: '%%' expects two arguments.")               \
  X(LIZARD_ERROR_MOD_ARGT, "Error: '%%' expects number arguments.")            \
  X(LIZARD_ERROR_MOD_ARGT_2, "Error: '%%' expects number arguments.")          \
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
  X(LIZARD_ERROR_TOKENIZATION,                                                 \
    "Error: tokenization failed due to a syntax error.")                       \
  X(LIZARD_ERROR_AST_ARGC,                                                     \
    "Error: AST function got an incorrect number of arguments.")               \
  X(LIZARD_ERROR_AST_ARGT,                                                     \
    "Error: AST function received an invalid argument type.")                  \
  X(LIZARD_ERROR_EVAL_ARGC, "Error: eval expects exactly one argument.")       \
  X(LIZARD_ERROR_UNQUOTE_ARGC, "Error: unquote requires an argument.")         \
  X(LIZARD_ERROR_UNQUOTE_ARGT, "Error: unquote expects a valid argument.")     \
  X(LIZARD_ERROR_INVALID_SPLICE, "Error: invalid splice.")                     \
  X(LIZARD_ERROR_CALLCC_ARGC, "Error: 'call/cc' expects an argument.")         \
  X(LIZARD_ERROR_CALLCC_APPLY,                                                 \
    "Error: 'call/cc' cannot be applied in this context.")                     \
  X(LIZARD_ERROR_INVALID_FORCE, "Error: invalid force.")                       \
  X(LIZARD_ERROR_INVALID_DELAY, "Error: invalid delay.")                       \
  X(LIZARD_ERROR_NULLP_ARGC, "Error: 'null?' expects exactly one argument.")   \
  X(LIZARD_ERROR_PAIRP_ARGC, "Error: 'pair?' expects exactly one argument.")   \
  X(LIZARD_ERROR_STRINGP_ARGC,                                                 \
    "Error: 'string?' expects exactly one argument.")                          \
  X(LIZARD_ERROR_BOOLP_ARGC, "Error: 'bool?' expects exactly one argument.")   \
  X(LIZARD_ERROR_NOT_ARGC, "Error: 'not' expects exactly one argument.")

#define LIZARD_ERROR_MESSAGES                                                  \
  X(LIZARD_ERROR_MESSAGES_EN, lizard_error_messages_lang_en)

#endif /* LIZARD_LANG_EN_H */
