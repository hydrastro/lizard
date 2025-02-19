#ifndef LIZARD_ERRORS_H
#define LIZARD_ERRORS_H

#define LIZARD_ERROR_LIST                                                      \
  X(LIZARD_ERROR_NONE)                                                         \
  X(LIZARD_ERROR_UNBOUND_SYMBOL)                                               \
  X(LIZARD_ERROR_INVALID_FUNCTION_DEF)                                         \
  X(LIZARD_ERROR_INVALID_FUNCTION_NAME)                                        \
  X(LIZARD_ERROR_INVALID_PARAMETER)                                            \
  X(LIZARD_ERROR_INVALID_DEF)                                                  \
  X(LIZARD_ERROR_ASSIGNMENT)                                                   \
  X(LIZARD_ERROR_ASSIGNMENT_UNBOUND)                                           \
  X(LIZARD_ERROR_INVALID_MACRO_NAME)                                           \
  X(LIZARD_ERROR_NODE_TYPE)                                                    \
  X(LIZARD_ERROR_LAMBDA_PARAMS)                                                \
  X(LIZARD_ERROR_VARIADIC_UNFOLLOWED)                                          \
  X(LIZARD_ERROR_VARIADIC_SYMBOL)                                              \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS)                                            \
  X(LIZARD_ERROR_LAMBDA_PARAMETER)                                             \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE)                                            \
  X(LIZARD_ERROR_INVALID_APPLY)                                                \
  X(LIZARD_ERROR_PLUS)                                                         \
  X(LIZARD_ERROR_MINUS_ARGC)                                                   \
  X(LIZARD_ERROR_MINUS_ARGT)                                                   \
  X(LIZARD_ERROR_MINUS_ARGT_2)                                                 \
  X(LIZARD_ERROR_MUL)                                                          \
  X(LIZARD_ERROR_DIV_ARGC)                                                     \
  X(LIZARD_ERROR_DIV_ARGT)                                                     \
  X(LIZARD_ERROR_DIV_ARGT_2)                                                   \
  X(LIZARD_ERROR_DIV_ZERO)                                                     \
  X(LIZARD_ERROR_EQ_ARGC)                                                      \
  X(LIZARD_ERROR_POW_ARGC)                                                     \
  X(LIZARD_ERROR_POW_ARGT)                                                     \
  X(LIZARD_ERROR_LT_ARGC)                                                      \
  X(LIZARD_ERROR_LT_ARGT)                                                      \
  X(LIZARD_ERROR_LT_ARGT_2)                                                    \
  X(LIZARD_ERROR_LE_ARGC)                                                      \
  X(LIZARD_ERROR_LE_ARGT)                                                      \
  X(LIZARD_ERROR_LE_ARGT_2)                                                    \
  X(LIZARD_ERROR_GT_ARGC)                                                      \
  X(LIZARD_ERROR_GT_ARGT)                                                      \
  X(LIZARD_ERROR_GT_ARGT_2)                                                    \
  X(LIZARD_ERROR_GE_ARGC)                                                      \
  X(LIZARD_ERROR_GE_ARGT)                                                      \
  X(LIZARD_ERROR_GE_ARGT_2)                                                    \
  X(LIZARD_ERROR_MOD_ARGC)                                                     \
  X(LIZARD_ERROR_MOD_ARGT)                                                     \
  X(LIZARD_ERROR_MOD_ARGT_2)                                                   \
  X(LIZARD_ERROR_CONS_ARGC)                                                    \
  X(LIZARD_ERROR_CAR_ARGC)                                                     \
  X(LIZARD_ERROR_CAR_ARGT)                                                     \
  X(LIZARD_ERROR_CAR_NIL)                                                      \
  X(LIZARD_ERROR_CDR_ARGC)                                                     \
  X(LIZARD_ERROR_CDR_ARGT)                                                     \
  X(LIZARD_ERROR_CDR_NIL)                                                      \
  X(LIZARD_ERROR_TOKENS_ARGC)                                                  \
  X(LIZARD_ERROR_TOKENS_ARGT)                                                  \
  X(LIZARD_ERROR_EVAL_ARGC)                                                    \
  X(LIZARD_ERROR_UNQUOTE)                                                      \
  X(LIZARD_ERROR_UNQUOTE_ARGC)                                                 \
  X(LIZARD_ERROR_INVALID_SPLICE)                                               \
  X(LIZARD_ERROR_CALLCC_ARGC)

typedef enum lizard_error_code {
#define X(err) err,
  LIZARD_ERROR_LIST
#undef X
      LIZARD_ERROR_COUNT
} lizard_error_code_t;

#endif /* LIZARD_ERRORS_H */