#ifndef IDNET_H
#define IDNET_H
/* idnet -- Id-by-observation as an interaction net (see idnet.c).
 * idnet_id_nf takes an ID_ID node (Id A x y) and computes the result type by
 * graph interaction (ID meets the type-former), reading it back as an id_node.
 * Returns NULL if the net reaches an unsupported former (read-back refuses
 * rather than returning a wrong type). The result matches id_nf() (the spec). */
#include "id_observe.h"
id_node *idnet_id_nf(const id_node *t, long *steps);
/* Parallel reducer: same result as idnet_id_nf, using `nthreads` worker threads (lock-free
 * disjoint-neighbourhood firing of the local Id/equality rules + a serial fallback for the
 * subtree-copying rules).  Validated to produce identical normal forms to idnet_id_nf. */
id_node *idnet_id_nf_par(const id_node *t, long *steps, int nthreads);
#endif /* IDNET_H */
