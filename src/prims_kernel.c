/* src/prims_kernel.c
 *
 * Kernel primitives are split across:
 *   prims_kernel_core.c   -- infer/check/reduce/equality
 *   prims_kernel_proof.c  -- proof-state and tactic front-end primitives
 *   prims_kernel_meta.c   -- holes, metavariables, unification
 *   prims_kernel_defs.c   -- named kernel definitions
 *   prims_kernel_util.c   -- shared formatting helpers
 *
 * This file intentionally remains as a phase marker so primitive registration
 * can stay centralized in primitives.c while implementation families keep
 * moving toward the SurfaceTerm/CoreTerm/KernelTerm boundary.
 */
