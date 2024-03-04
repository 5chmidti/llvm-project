.. title:: clang-tidy - openmp-critical-section-deadlock

openmp-critical-section-deadlock
================================

Detects deadlocks from nesting OpenMP critical regions.
Detection is done by comparing the ordering when nesting critical sections,
including traversal of the statically known call graph.
