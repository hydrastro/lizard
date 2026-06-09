#ifndef IDNET_H
#define IDNET_H
/* idnet -- Id-by-observation as an interaction net (see idnet.c).
 * idnet_id_nf takes an ID_ID node (Id A x y) and computes the result type by
 * graph interaction (ID meets the type-former), reading it back as an id_node.
 * Returns NULL if the net reaches an unsupported former (read-back refuses
 * rather than returning a wrong type). The result matches id_nf() (the spec). */
#include "id_observe.h"
id_node *idnet_id_nf(const id_node *t, long *steps);
#endif /* IDNET_H */
