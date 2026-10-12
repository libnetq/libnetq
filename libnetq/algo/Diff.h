/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_ALGO_DIFF_H
#define _LIBNETQ_ALGO_DIFF_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One difference between sequences a and b: a[aStart, aStart + aCount)
 * is replaced by b[bStart, bStart + bCount). aCount == 0 is an insertion
 * (before a[aStart]), bCount == 0 a deletion. */
typedef struct NQDiffHunk {
  size_t aStart;
  size_t aCount;
  size_t bStart;
  size_t bCount;
} NQDiffHunk;

/* Flags for NQDiff_computeEx and NQDiff_mark */
enum {
  /* Slide each run of changes over the equal elements around it, merging
   * runs that can be merged and lining runs up with runs of changes on the
   * other side, as GNU diff does: the script stays as short, and reads as
   * a person would write it ("}" after a function added below another,
   * not before). */
  NQ_DIFF_COMPACT = 1 << 0,
  /* Before the search, set aside as changed the elements with nothing
   * equal on the other side, and those equal to many inside stretches of
   * such elements, as GNU diff does unless given --minimal: quicker on
   * large inputs, and what makes the scripts the same as diff's, though
   * no longer always the shortest. */
  NQ_DIFF_DISCARD = 1 << 1,
  /* Let a search that has taken more than about sqrt(N) edit steps (at
   * least 4096) settle for the furthest point it reached, as GNU diff
   * does unless given --minimal: inputs with little in common then take
   * O(N * sqrt(N)) instead of O(N^2), and the script may be longer. */
  NQ_DIFF_COST_LIMIT = 1 << 2,
};

/* The shortest edit script between two sequences, by Myers' O(ND)
 * difference algorithm in its linear space form. The elements are given
 * as ids, equal exactly when the elements are: line numbers in a table of
 * unique lines, code points, word numbers, ... Among scripts of the same
 * length, the one chosen is the one GNU diff's search finds.
 *
 * On success *hunks is an array of *count differences in order, freed with
 * NQDiff_free, or NULL when the sequences are equal. Returns 0, or
 * -NQ_ENOMEM. */
NQ_EXPORT int NQDiff_compute(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength,
                             NQDiffHunk** hunks, size_t* count);

/* NQDiff_compute with NQ_DIFF_* flags. Every element given takes part:
 * runs of changes slide (NQ_DIFF_COMPACT) and counts are taken
 * (NQ_DIFF_DISCARD) over all of a and b. GNU diff first drops the common
 * start and end but for --horizon-lines of each; give it the rest to get
 * its results. */
NQ_EXPORT int NQDiff_computeEx(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength,
                               unsigned flags, NQDiffHunk** hunks, size_t* count);

/* The same comparison, as marks: aChanged[i] is set to 1 when a[i] is
 * removed and bChanged[j] when b[j] is inserted, 0 otherwise (arrays of
 * aLength and bLength bytes). Returns 0, or -NQ_ENOMEM. */
NQ_EXPORT int NQDiff_mark(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength, unsigned flags,
                          uint8_t* aChanged, uint8_t* bChanged);

/* The hunks that marks such as NQDiff_mark's make, as NQDiff_compute
 * returns them. Returns 0, or -NQ_ENOMEM. */
NQ_EXPORT int NQDiff_hunks(const uint8_t* aChanged, size_t aLength, const uint8_t* bChanged, size_t bLength,
                           NQDiffHunk** hunks, size_t* count);

NQ_EXPORT void NQDiff_free(NQDiffHunk* hunks);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_ALGO_DIFF_H */
