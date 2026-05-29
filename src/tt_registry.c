/* tt_registry.c
 *
 * Runtime-owned type-theory registries: fresh dimensions, HIT declarations,
 * logic-rule configuration, snapshots, and named logic bundles. Split from
 * tt_equality.c so judgmental equality remains focused on normalization.
 */

#include "primitives.h"
#include "lizard_internal.h"
#include "mem.h"
#include "runtime.h"

#include <stdlib.h>
#include <string.h>

/* Phase L.3 — per-session fresh-dimension counter.
 *
 * Each call returns a new, distinct natural-number dimension. Used
 * by the typing rule for `pi-fresh` and `sigma-fresh` to label the
 * dimension introduced by a quantification. The counter is process-
 * local (no global heap dependency), so it resets when a new lizard
 * session starts — useful for deterministic testing.
 *
 * Starts at 1000 to leave room for "ordinary" dimensions 0-999 that
 * users might write explicitly. Two pi-fresh calls in the same
 * session get dims 1000, 1001, 1002, ...
 */
long lizard_tt_next_fresh_dim(void) {
  static long counter = 1000;
  return counter++;
}

/* ===== Phase H.1 — HIT registry =====
 *
 * Simple linked-list registry of declared HITs, keyed by name. The
 * registry is per-process and lives in static storage; entries
 * accumulate as HIT declarations are evaluated.
 *
 * Two operations matter for correctness: register a new HIT (which
 * fails silently if the name already exists, treating re-declaration
 * as an overwrite), and look up by name (returns NULL if missing).
 */
typedef struct hit_registry_entry {
  const char *name;
  lizard_ast_node_t *decl;
  struct hit_registry_entry *next;
} hit_registry_entry_t;

static hit_registry_entry_t *hit_registry_head_fallback = NULL;
static hit_registry_entry_t **hit_registry_ptr(void) {
  return (heap != NULL && heap->runtime != NULL)
           ? (hit_registry_entry_t **)&heap->runtime->hit_registry_head
           : &hit_registry_head_fallback;
}

void lizard_tt_hit_register(lizard_ast_node_t *decl) {
  hit_registry_entry_t *entry;
  const char *name;
  if (decl == NULL || decl->type != AST_TT_HIT_DECL) return;
  if (decl->data.tt_hit_decl.name == NULL ||
      decl->data.tt_hit_decl.name->type != AST_SYMBOL) return;
  name = decl->data.tt_hit_decl.name->data.variable;
  /* Overwrite if already exists. */
  for (entry = *hit_registry_ptr(); entry != NULL; entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      entry->decl = decl;
      return;
    }
  }
  /* Otherwise prepend. Note: registry entries leak across heap
   * lifetimes by design — they're per-process metadata, not per-heap
   * data. The name string is borrowed from the decl AST node, which
   * is heap-allocated in lizard_heap. If the heap is destroyed, the
   * registry will dangle. This is acceptable for H.1; a future
   * iteration should either copy strings or tie registry life to
   * the heap. */
  entry = malloc(sizeof(hit_registry_entry_t));
  if (entry == NULL) return;
  entry->name = name;
  entry->decl = decl;
  entry->next = *hit_registry_ptr();
  *hit_registry_ptr() = entry;
}

lizard_ast_node_t *lizard_tt_hit_lookup(const char *name) {
  hit_registry_entry_t *entry;
  if (name == NULL) return NULL;
  for (entry = *hit_registry_ptr(); entry != NULL; entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      return entry->decl;
    }
  }
  return NULL;
}

void lizard_tt_hit_registry_reset(void) {
  hit_registry_entry_t *entry = *hit_registry_ptr();
  while (entry != NULL) {
    hit_registry_entry_t *next = entry->next;
    free(entry);
    entry = next;
  }
  *hit_registry_ptr() = NULL;
}

long lizard_tt_hit_registry_size(void) {
  hit_registry_entry_t *entry;
  long count = 0;
  for (entry = *hit_registry_ptr(); entry != NULL; entry = entry->next) {
    count++;
  }
  return count;
}

/* Look up which HIT declaration owns a constructor with the given
 * name. Returns the HIT_DECL or NULL if not found. Used by the
 * typing rule for HIT_APP. */
lizard_ast_node_t *lizard_tt_hit_lookup_constructor_host(const char *cname) {
  hit_registry_entry_t *entry;
  if (cname == NULL) return NULL;
  for (entry = *hit_registry_ptr(); entry != NULL; entry = entry->next) {
    lz_list_node_t *p;
    lz_list_t *ctors = entry->decl->data.tt_hit_decl.constructors;
    for (p = ctors->head; p != ctors->nil; p = p->next) {
      lizard_ast_node_t *ctor = ((lizard_ast_list_node_t *)p)->ast;
      if (ctor->type == AST_TT_HIT_CONSTRUCTOR &&
          ctor->data.tt_hit_constructor.name != NULL &&
          ctor->data.tt_hit_constructor.name->type == AST_SYMBOL &&
          strcmp(ctor->data.tt_hit_constructor.name->data.variable, cname) == 0) {
        return entry->decl;
      }
    }
  }
  return NULL;
}

/* ===== Phase M.1 — logic-rule configuration registry =====
 *
 * Lizard is intended as a configurable type-theory framework: which
 * inference rules are active determines which logic the kernel
 * implements. The lambda cube formalism is the inspiration — the
 * cube's eight corners (STLC, F, F2, Fω, LF, λP, λPω, CoC) are
 * obtained by toggling three orthogonal axes. Beyond the cube,
 * modalities, structural rules, lattice features, and HIT support
 * can all become toggleable axes of a larger configuration space.
 *
 * M.1 lands ONLY the infrastructure — the registry, the snapshot/
 * restore mechanism, and the user-facing primitives. NO rule is
 * pre-registered, and NO type-checking site yet consults the
 * registry. M.2 and beyond will wire specific rules to it.
 *
 * The registry is per-process, mirroring the precedent set by the
 * fresh-dim counter (L.3) and the HIT registry (H.1). A snapshot
 * is a copied linked list, used for "switch logic, do work, switch
 * back" patterns where the user wants to be sure nothing leaks
 * between configurations.
 */
typedef struct logic_rule_entry {
  const char *name;
  int enabled;            /* 0 or 1 */
  struct logic_rule_entry *next;
} logic_rule_entry_t;

static logic_rule_entry_t *logic_config_head_fallback = NULL;
static logic_rule_entry_t **logic_config_ptr(void) {
  return (heap != NULL && heap->runtime != NULL)
           ? (logic_rule_entry_t **)&heap->runtime->logic_config_head
           : &logic_config_head_fallback;
}
/* Phase M.5.7 — last explicitly-set bundle name, for current_bundle
 * reverse lookup. If a user calls (set-logic 'X) and X still matches
 * the active state, current-logic returns "X". Otherwise (e.g. after
 * a manual toggle change that breaks the match, or after a reset),
 * current-logic falls back to walking the table.
 *
 * This is state, not a pure function of toggles, because the toggle
 * state alone is ambiguous in some cases — "all modal toggles on"
 * could be CoC-with-extras or S5. set-logic's explicit name resolves
 * the ambiguity. */
static const char *logic_last_set_bundle_fallback = NULL;
static const char **logic_last_bundle_ptr(void) {
  return (heap != NULL && heap->runtime != NULL)
           ? (const char **)&heap->runtime->logic_last_set_bundle
           : &logic_last_set_bundle_fallback;
}

/* Internal: find a rule by name, return entry or NULL. */
static logic_rule_entry_t *logic_rule_find(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return NULL;
  for (e = *logic_config_ptr(); e != NULL; e = e->next) {
    if (strcmp(e->name, name) == 0) return e;
  }
  return NULL;
}

/* Register a new rule with a default enabled state. If the rule
 * already exists, this is a no-op (we don't overwrite). */
void lizard_logic_rule_register(const char *name, int default_enabled) {
  logic_rule_entry_t *e;
  size_t namelen;
  char *namedup;
  if (name == NULL) return;
  if (logic_rule_find(name) != NULL) return;  /* already registered */
  e = malloc(sizeof(logic_rule_entry_t));
  if (e == NULL) return;
  /* Copy the name so callers' strings can be freed without dangling
   * the registry. We use plain malloc; the registry leaks at process
   * exit, which is acceptable for M.1. */
  namelen = strlen(name) + 1;
  namedup = malloc(namelen);
  if (namedup == NULL) { free(e); return; }
  memcpy(namedup, name, namelen);
  e->name = namedup;
  e->enabled = default_enabled ? 1 : 0;
  e->next = *logic_config_ptr();
  *logic_config_ptr() = e;
}

/* Enable a rule. Auto-registers if not yet present. */
int lizard_logic_rule_enable(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return 0;
  e = logic_rule_find(name);
  if (e == NULL) {
    lizard_logic_rule_register(name, 1);
    return 1;
  }
  e->enabled = 1;
  return 1;
}

/* Disable a rule. Auto-registers as disabled if not yet present. */
int lizard_logic_rule_disable(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return 0;
  e = logic_rule_find(name);
  if (e == NULL) {
    lizard_logic_rule_register(name, 0);
    return 1;
  }
  e->enabled = 0;
  return 1;
}

/* Query a rule's enabled state. Returns 1 if enabled, 0 if disabled,
 * -1 if not registered. The -1 case lets callers distinguish "off by
 * default because configured off" from "unknown rule". */
int lizard_logic_rule_enabled(const char *name) {
  logic_rule_entry_t *e;
  if (name == NULL) return -1;
  e = logic_rule_find(name);
  if (e == NULL) return -1;
  return e->enabled;
}

long lizard_logic_config_size(void) {
  logic_rule_entry_t *e;
  long count = 0;
  for (e = *logic_config_ptr(); e != NULL; e = e->next) count++;
  return count;
}

/* Reset the entire configuration. Mostly for tests. */
void lizard_logic_config_reset(void) {
  logic_rule_entry_t *e = *logic_config_ptr();
  while (e != NULL) {
    logic_rule_entry_t *next = e->next;
    /* Cast away const for free(); we own the string via the malloc
     * in register/snapshot. */
    char *name_mut;
    memcpy(&name_mut, &e->name, sizeof(char *));
    free(name_mut);
    free(e);
    e = next;
  }
  *logic_config_ptr() = NULL;
  /* M.5.7 — clear remembered set-logic name. */
  *logic_last_bundle_ptr() = NULL;
}

/* Snapshot: take a deep copy of the current configuration and return
 * an opaque handle. The handle is a pointer to a copy of the linked
 * list. Callers must restore or free it later. */
/* Snapshot wrapper that includes the remembered set-logic name.
 * Returned as the opaque snapshot handle; restore unwraps it. */
typedef struct logic_snapshot {
  logic_rule_entry_t *config_head;
  const char *last_set_bundle;
} logic_snapshot_t;

void *lizard_logic_snapshot(void) {
  logic_rule_entry_t *src, *new_head, *new_tail;
  logic_snapshot_t *snap;
  new_head = NULL;
  new_tail = NULL;
  for (src = *logic_config_ptr(); src != NULL; src = src->next) {
    logic_rule_entry_t *copy;
    char *namedup;
    size_t namelen;
    copy = malloc(sizeof(logic_rule_entry_t));
    if (copy == NULL) break;  /* partial snapshot; OK */
    namelen = strlen(src->name) + 1;
    namedup = malloc(namelen);
    if (namedup == NULL) { free(copy); break; }
    memcpy(namedup, src->name, namelen);
    copy->name = namedup;
    copy->enabled = src->enabled;
    copy->next = NULL;
    if (new_head == NULL) {
      new_head = copy;
    } else {
      new_tail->next = copy;
    }
    new_tail = copy;
  }
  snap = malloc(sizeof(logic_snapshot_t));
  if (snap == NULL) {
    /* Out of memory; fall back to returning just the list. Restore
     * detects this case via the magic head pointer convention is
     * undefined here — easier to leak. */
    return new_head;
  }
  snap->config_head = new_head;
  snap->last_set_bundle = *logic_last_bundle_ptr();  /* Points into static table; no dup needed. */
  return snap;
}

/* Restore: replace the current configuration with the snapshot. The
 * snapshot is CONSUMED (becomes the new active config); callers
 * must not reuse it. To keep a snapshot for multiple restores, take
 * multiple snapshots. */
void lizard_logic_restore(void *snapshot) {
  logic_snapshot_t *snap = (logic_snapshot_t *)snapshot;
  lizard_logic_config_reset();
  if (snap != NULL) {
    *logic_config_ptr() = snap->config_head;
    *logic_last_bundle_ptr() = snap->last_set_bundle;
    free(snap);
  }
}

/* Iterator: walk the configuration in registration order (reverse
 * of insertion, since we prepend). Returns 0 to continue, non-zero
 * to stop. */
void lizard_logic_config_walk(int (*cb)(const char *name, int enabled,
                                        void *userdata),
                              void *userdata) {
  logic_rule_entry_t *e;
  for (e = *logic_config_ptr(); e != NULL; e = e->next) {
    if (cb(e->name, e->enabled, userdata)) return;
  }
}

/* ===== Phase M.3 — named logic bundles =====
 *
 * Sugar on top of the M.2 cube toggles. Each named logic corresponds
 * to a specific combination of the three cube axes:
 *
 *                   term-on-type  type-on-term  type-on-type
 *   STLC            off           off           off
 *   F               on            off           off
 *   LF (= lambda-P) off           on            off
 *   F-omega         off           off           on
 *   lambda-P2       on            on            off
 *   lambda-P-omega  off           on            on
 *   lambda-omega    on            off           on
 *   CoC             on            on            on
 *
 * M.3 is sugar — it doesn't add new behavior. `(set-logic 'STLC)`
 * is equivalent to disabling all three cube axes. The reverse,
 * `(current-logic)`, looks at the active config and returns the
 * matching bundle name (or 'custom if no match).
 */
typedef struct logic_bundle {
  const char *name;
  int term_on_type;
  int type_on_term;
  int type_on_type;
  /* Phase M.4: structural rules. -1 = "don't care". */
  int weakening;
  int contraction;
  int exchange;
  /* Phase M.6: lattice and HIT features. -1 = "don't care". */
  int pi_fresh_enabled;
  int co_pi_fresh_enabled;
  int HIT_enabled;
  int lattice_universes;
  int couniverse_lattice;
  /* Phase M.5.3: modal logic toggles. -1 = "don't care". */
  int modalities_enabled;
  int modal_strict_typing;
  /* Phase M.5.4: 4-axiom (□A → □□A) toggle. -1 = "don't care".
   * Distinguishes T (no 4-axiom) from S4 (yes 4-axiom). */
  int modal_4_axiom;
  /* Phase M.5.5 Turn 2: 5-axiom (◇A → □◇A) toggle. -1 = "don't care".
   * Distinguishes S4 (no 5-axiom) from S5 (yes 5-axiom).
   *
   * When ON, let-diamond extends the VALID context — the unboxed
   * Diamond content is treated as necessarily-possibly-true. When
   * OFF, let-diamond extends truth (the M.5.5 Turn 1 behavior). */
  int modal_5_axiom;
  /* Phase M.5.6: T-axiom (□A → A) toggle. -1 = "don't care".
   * Distinguishes K (no T-axiom; unbox cannot extract) from T+
   * (T-axiom present; unbox extracts via binder).
   *
   * When OFF, unbox requires body to be Box-typed — no extraction.
   * When ON (default), unbox is full S4-style extraction. */
  int t_axiom;
  /* Phase M.5.9: Symmetric S5 (Pfenning-Davies three-judgment form).
   * -1 = "don't care".
   *
   * When ON, the new symmetric AST forms (dia, let-dia, poss-coerce)
   * typecheck under the three-context discipline. When OFF, those
   * forms reject; the existing asymmetric forms (diamond, let-diamond)
   * are unaffected either way.
   *
   * Bundle settings track modal_5_axiom: K/T/S4 have it off, S5 has
   * it on. */
  int modal_symmetric;
  /* Optional proof-theory scaffolds. -1 requires the feature to be off. */
  int cubical_s1;
  int truncations;
  int theory_extensions;
} logic_bundle_t;

/* Table of predefined logics. The order matters for reverse lookup:
 * we walk this table and return the FIRST match where all specified
 * (non -1) fields agree with the active config. Cube-only bundles
 * leave structural and feature fields at -1 (which match only when
 * the active config has those at their defaults, all on).
 *
 * Phase M.6: two new bundles demonstrate the matrix:
 *   STLC-strict     — STLC with ALL lizard extras disabled
 *   CoC-plus-lattice — CoC with lattice on, HITs off
 *
 * Phase M.5.3: modal-logic bundles. At lizard's current depth, K, T,
 * S4, and S5 are operationally indistinguishable — the strict box-
 * intro and unbox rules implemented in M.5.2 Turn 2 are the same
 * across these logics. What differs between them are AXIOMS:
 *
 *   K: no extra axioms
 *   T: K + (□A → A)
 *   S4: T + (□A → □□A)
 *   S5: S4 + (◇A → □◇A)
 *
 * These axioms are not yet wired as type-checkable rules. Selecting
 * 'K vs 'S4 currently flips the same toggles and produces identical
 * type-checking behavior. The bundle names are forward-looking:
 * they declare intent and reserve the names for future axiom work.
 */
static logic_bundle_t logic_bundles[] = {
  /* name           cube           structural     features          modal     4ax  5ax  tax sym */
  /*                tot ton too    wk ct ex       pf cpf H lat colat me ms   4    5    T   S  */
  {"STLC",            0, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"F",               1, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"LF",              0, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"lambda-P",        0, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"F-omega",         0, 0, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"lambda-P2",       1, 1, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"lambda-P-omega",  0, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"lambda-omega",    1, 0, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"CoC",             1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  /* M.4 substructural variants */
  {"linear-STLC",     0, 0, 0,     0, 0, 1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"affine-STLC",     0, 0, 0,     1, 0, 1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"relevant-STLC",   0, 0, 0,     0, 1, 1,    -1,-1,-1,-1,-1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  /* M.6 feature-matrix variants */
  {"STLC-strict",     0, 0, 0,    -1,-1,-1,     0, 0, 0, 0, 0,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  {"CoC-plus-lattice",1, 1, 1,    -1,-1,-1,     1, 1, 0, 1, 1,   -1,-1,    -1,  -1,  -1, -1, -1, -1, -1},
  /* M.5.3+ modal-logic bundles. T differs from S4 by the 4-axiom
   * (modal_4_axiom). S5 differs from S4 by the 5-axiom (modal_5_axiom,
   * M.5.5 Turn 2). K differs from T by the T-axiom (t_axiom, M.5.6):
   * K disables extraction-via-unbox, forcing unbox to keep results
   * boxed. All four modal logics are now operationally distinct. */
  {"K",               1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1,     0,   0,  0, 0, -1, -1, -1},
  {"T",               1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1,     0,   0,  1, 0, -1, -1, -1},
  {"S4",              1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1,     1,   0,  1, 0, -1, -1, -1},
  {"S5",              1, 1, 1,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1,     1,   1,  1, 1, -1, -1, -1},
  /* Composite: STLC base with modalities (S4-flavored: no 5-axiom). */
  {"modal-STLC",      0, 0, 0,    -1,-1,-1,    -1,-1,-1,-1,-1,    1, 1,     1,   0,  1, 0, -1, -1, -1},
  {"cubical-S1",       1, 1, 1,    -1,-1,-1,     1, 1, 1, 1, 1,    1, 1,     1,   1,  1, 0, 1, 0, 0},
  {"truncations",      1, 1, 1,    -1,-1,-1,     1, 1, 1, 1, 1,    1, 1,     1,   1,  1, 0, 0, 1, 0},
  {"proof-scaffold",   1, 1, 1,    -1,-1,-1,     1, 1, 1, 1, 1,    1, 1,     1,   1,  1, 0, 1, 1, 1},
  {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int lizard_logic_set_bundle(const char *name) {
  logic_bundle_t *b;
  if (name == NULL) return 0;
  for (b = logic_bundles; b->name != NULL; b++) {
    if (strcmp(b->name, name) == 0) {
      if (b->term_on_type)  lizard_logic_rule_enable("term-depends-on-type");
      else                  lizard_logic_rule_disable("term-depends-on-type");
      if (b->type_on_term)  lizard_logic_rule_enable("type-depends-on-term");
      else                  lizard_logic_rule_disable("type-depends-on-term");
      if (b->type_on_type)  lizard_logic_rule_enable("type-depends-on-type");
      else                  lizard_logic_rule_disable("type-depends-on-type");
      if (b->weakening != -1) {
        if (b->weakening)   lizard_logic_rule_enable("weakening");
        else                lizard_logic_rule_disable("weakening");
      }
      if (b->contraction != -1) {
        if (b->contraction) lizard_logic_rule_enable("contraction");
        else                lizard_logic_rule_disable("contraction");
      }
      if (b->exchange != -1) {
        if (b->exchange)    lizard_logic_rule_enable("exchange");
        else                lizard_logic_rule_disable("exchange");
      }
      /* M.6 feature toggles. */
      if (b->pi_fresh_enabled != -1) {
        if (b->pi_fresh_enabled) lizard_logic_rule_enable("pi-fresh-enabled");
        else                     lizard_logic_rule_disable("pi-fresh-enabled");
      }
      if (b->co_pi_fresh_enabled != -1) {
        if (b->co_pi_fresh_enabled) lizard_logic_rule_enable("co-pi-fresh-enabled");
        else                        lizard_logic_rule_disable("co-pi-fresh-enabled");
      }
      if (b->HIT_enabled != -1) {
        if (b->HIT_enabled)      lizard_logic_rule_enable("HIT-enabled");
        else                     lizard_logic_rule_disable("HIT-enabled");
      }
      if (b->lattice_universes != -1) {
        if (b->lattice_universes) lizard_logic_rule_enable("lattice-universes");
        else                      lizard_logic_rule_disable("lattice-universes");
      }
      if (b->couniverse_lattice != -1) {
        if (b->couniverse_lattice) lizard_logic_rule_enable("couniverse-lattice");
        else                       lizard_logic_rule_disable("couniverse-lattice");
      }
      /* M.5.3 modal toggles. */
      if (b->modalities_enabled != -1) {
        if (b->modalities_enabled) lizard_logic_rule_enable("modalities-enabled");
        else                       lizard_logic_rule_disable("modalities-enabled");
      }
      if (b->modal_strict_typing != -1) {
        if (b->modal_strict_typing) lizard_logic_rule_enable("modal-strict-typing");
        else                        lizard_logic_rule_disable("modal-strict-typing");
      }
      /* M.5.4 4-axiom. */
      if (b->modal_4_axiom != -1) {
        if (b->modal_4_axiom) lizard_logic_rule_enable("modal-4-axiom");
        else                  lizard_logic_rule_disable("modal-4-axiom");
      }
      /* M.5.5 5-axiom. */
      if (b->modal_5_axiom != -1) {
        if (b->modal_5_axiom) lizard_logic_rule_enable("modal-5-axiom");
        else                  lizard_logic_rule_disable("modal-5-axiom");
      }
      /* M.5.6 T-axiom. */
      if (b->t_axiom != -1) {
        if (b->t_axiom) lizard_logic_rule_enable("t-axiom-enabled");
        else            lizard_logic_rule_disable("t-axiom-enabled");
      }
      /* M.5.9 symmetric S5. */
      if (b->modal_symmetric != -1) {
        if (b->modal_symmetric) lizard_logic_rule_enable("modal-symmetric");
        else                    lizard_logic_rule_disable("modal-symmetric");
      }
      /* Optional proof-theory scaffolds. */
      if (b->cubical_s1 != -1) {
        if (b->cubical_s1) lizard_logic_rule_enable("cubical-s1-enabled");
        else               lizard_logic_rule_disable("cubical-s1-enabled");
      }
      if (b->truncations != -1) {
        if (b->truncations) lizard_logic_rule_enable("truncations-enabled");
        else                lizard_logic_rule_disable("truncations-enabled");
      }
      if (b->theory_extensions != -1) {
        if (b->theory_extensions) lizard_logic_rule_enable("theory-extensions-enabled");
        else                      lizard_logic_rule_disable("theory-extensions-enabled");
      }
      /* M.5.7 — remember the explicit name so current_bundle can
       * disambiguate when multiple bundles match the same toggle
       * state. Points into the static table; no allocation. */
      *logic_last_bundle_ptr() = b->name;
      return 1;
    }
  }
  return 0;
}

/* Helper: test whether a specific bundle matches the active state.
 * Same conditions as the table walk; factored out for reuse by
 * current_bundle's remembered-name fast path. */
static int bundle_matches_active(logic_bundle_t *b) {
  int term_on, type_on_term, type_on_type;
  int weakening, contraction, exchange;
  int pi_fresh, co_pi_fresh, hit_en, lat_u, lat_co;
  int modalities_en, modal_strict, modal_4, modal_5, t_ax, modal_sym;
  int cubical_s1, truncations, theory_ext;
  int match = 1;
  term_on      = lizard_logic_rule_enabled("term-depends-on-type");
  type_on_term = lizard_logic_rule_enabled("type-depends-on-term");
  type_on_type = lizard_logic_rule_enabled("type-depends-on-type");
  weakening    = lizard_logic_rule_enabled("weakening");
  contraction  = lizard_logic_rule_enabled("contraction");
  exchange     = lizard_logic_rule_enabled("exchange");
  pi_fresh     = lizard_logic_rule_enabled("pi-fresh-enabled");
  co_pi_fresh  = lizard_logic_rule_enabled("co-pi-fresh-enabled");
  hit_en       = lizard_logic_rule_enabled("HIT-enabled");
  lat_u        = lizard_logic_rule_enabled("lattice-universes");
  lat_co       = lizard_logic_rule_enabled("couniverse-lattice");
  modalities_en = lizard_logic_rule_enabled("modalities-enabled");
  modal_strict  = lizard_logic_rule_enabled("modal-strict-typing");
  modal_4 = lizard_logic_rule_enabled("modal-4-axiom");
  modal_5 = lizard_logic_rule_enabled("modal-5-axiom");
  t_ax    = lizard_logic_rule_enabled("t-axiom-enabled");
  modal_sym = lizard_logic_rule_enabled("modal-symmetric");
  cubical_s1 = lizard_logic_rule_enabled("cubical-s1-enabled");
  truncations = lizard_logic_rule_enabled("truncations-enabled");
  theory_ext = lizard_logic_rule_enabled("theory-extensions-enabled");
  if (term_on == -1)      term_on = 1;
  if (type_on_term == -1) type_on_term = 1;
  if (type_on_type == -1) type_on_type = 1;
  if (weakening == -1)    weakening = 1;
  if (contraction == -1)  contraction = 1;
  if (exchange == -1)     exchange = 1;
  if (pi_fresh == -1)     pi_fresh = 1;
  if (co_pi_fresh == -1)  co_pi_fresh = 1;
  if (hit_en == -1)       hit_en = 1;
  if (lat_u == -1)        lat_u = 1;
  if (lat_co == -1)       lat_co = 1;
  if (modalities_en == -1) modalities_en = 1;
  if (modal_strict == -1)  modal_strict = 1;
  if (modal_4 == -1)       modal_4 = 1;
  if (modal_5 == -1)       modal_5 = 1;
  if (t_ax == -1)          t_ax = 1;
  if (modal_sym == -1)     modal_sym = 1;
  if (cubical_s1 == -1)    cubical_s1 = 0;
  if (truncations == -1)   truncations = 0;
  if (theory_ext == -1)    theory_ext = 0;
  if (b->term_on_type != term_on) return 0;
  if (b->type_on_term != type_on_term) return 0;
  if (b->type_on_type != type_on_type) return 0;
  if (b->weakening != -1) {
    if (b->weakening != weakening) match = 0;
  } else if (weakening != 1) match = 0;
  if (b->contraction != -1) {
    if (b->contraction != contraction) match = 0;
  } else if (contraction != 1) match = 0;
  if (b->exchange != -1) {
    if (b->exchange != exchange) match = 0;
  } else if (exchange != 1) match = 0;
  if (b->pi_fresh_enabled != -1) {
    if (b->pi_fresh_enabled != pi_fresh) match = 0;
  } else if (pi_fresh != 1) match = 0;
  if (b->co_pi_fresh_enabled != -1) {
    if (b->co_pi_fresh_enabled != co_pi_fresh) match = 0;
  } else if (co_pi_fresh != 1) match = 0;
  if (b->HIT_enabled != -1) {
    if (b->HIT_enabled != hit_en) match = 0;
  } else if (hit_en != 1) match = 0;
  if (b->lattice_universes != -1) {
    if (b->lattice_universes != lat_u) match = 0;
  } else if (lat_u != 1) match = 0;
  if (b->couniverse_lattice != -1) {
    if (b->couniverse_lattice != lat_co) match = 0;
  } else if (lat_co != 1) match = 0;
  if (b->modalities_enabled != -1) {
    if (b->modalities_enabled != modalities_en) match = 0;
  } else if (modalities_en != 1) match = 0;
  if (b->modal_strict_typing != -1) {
    if (b->modal_strict_typing != modal_strict) match = 0;
  } else if (modal_strict != 1) match = 0;
  if (b->modal_4_axiom != -1) {
    if (b->modal_4_axiom != modal_4) match = 0;
  } else if (modal_4 != 1) match = 0;
  if (b->modal_5_axiom != -1) {
    if (b->modal_5_axiom != modal_5) match = 0;
  } else if (modal_5 != 1) match = 0;
  if (b->t_axiom != -1) {
    if (b->t_axiom != t_ax) match = 0;
  } else if (t_ax != 1) match = 0;
  if (b->modal_symmetric != -1) {
    if (b->modal_symmetric != modal_sym) match = 0;
  } else if (modal_sym != 1) match = 0;
  if (b->cubical_s1 != -1) {
    if (b->cubical_s1 != cubical_s1) match = 0;
  } else if (cubical_s1 != 0) match = 0;
  if (b->truncations != -1) {
    if (b->truncations != truncations) match = 0;
  } else if (truncations != 0) match = 0;
  if (b->theory_extensions != -1) {
    if (b->theory_extensions != theory_ext) match = 0;
  } else if (theory_ext != 0) match = 0;
  return match;
}

/* Returns the name of the current logic, or "custom" if no bundle
 * matches.
 *
 * M.5.7 — if the user explicitly called (set-logic 'X) and X still
 * matches the active state, return "X". This resolves the ambiguity
 * where multiple bundles can match the same toggle state (e.g. S5
 * and CoC both match all-toggles-on). Falls back to walking the
 * table when no name was set or the remembered name no longer
 * applies. */
const char *lizard_logic_current_bundle(void) {
  logic_bundle_t *b;
  /* Fast path: try the remembered name first. */
  if (*logic_last_bundle_ptr() != NULL) {
    for (b = logic_bundles; b->name != NULL; b++) {
      if (strcmp(b->name, *logic_last_bundle_ptr()) == 0) {
        if (bundle_matches_active(b)) return b->name;
        break;  /* Found the bundle but it no longer matches; fall through. */
      }
    }
  }
  /* Fall back to table walk. */
  for (b = logic_bundles; b->name != NULL; b++) {
    if (bundle_matches_active(b)) return b->name;
  }
  return "custom";
}

void lizard_logic_bundles_walk(int (*cb)(const char *name, void *userdata),
                                void *userdata) {
  logic_bundle_t *b;
  for (b = logic_bundles; b->name != NULL; b++) {
    if (cb(b->name, userdata)) return;
  }
}

