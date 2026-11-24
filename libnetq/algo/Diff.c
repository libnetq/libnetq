/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/algo/Diff.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

/* The elements of a and b are marked as changed (removed from a, inserted
 * into b) by divide and conquer on the middle snake of the edit graph
 * (E. Myers, "An O(ND) Difference Algorithm and Its Variations", 1986,
 * section 4b): a forward search from the top left corner and a backward
 * one from the bottom right corner take turns, one edit step at a time,
 * until their furthest reaching paths meet. Diagonals are numbered
 * k = x - y; each search keeps the furthest x it reached on each
 * diagonal. The hunks are then read off the marks. */

struct DiffContext {
  const uint32_t* a;
  const uint32_t* b;
  uint8_t* aChanged;
  uint8_t* bChanged;
  ptrdiff_t* fwd; /* furthest x of the forward search on each diagonal */
  ptrdiff_t* bwd; /* smallest x of the backward search on each diagonal */
  ptrdiff_t limit; /* edit steps a search may take before it settles; 0: no limit */
};

#define FAR_LEFT ((ptrdiff_t)-1)
#define FAR_RIGHT PTRDIFF_MAX

static void mark(uint8_t* changed, ptrdiff_t start, ptrdiff_t end)
{
  for (ptrdiff_t i = start; i < end; i++)
    changed[i] = 1;
}

/* Where the forward path from (x0, y0) and the backward path from
 * (x1, y1) meet: sets *xmid, *ymid to a point on an optimal path. Both
 * ranges are non-empty. Ties go to the higher diagonal and, among equal
 * paths, to the one that keeps elements of a longest, as diff does.
 *
 * With a step limit and `minimal` 0, a search that takes more steps than
 * that settles for the point either side got furthest to, and the side
 * that stops there is solved minimally after: *lo_minimal or *hi_minimal
 * says which. The result is no longer always the shortest, but large
 * inputs that have little in common take O(N * limit), not O(N^2). */
static void split(struct DiffContext* ctx, ptrdiff_t x0, ptrdiff_t x1, ptrdiff_t y0, ptrdiff_t y1, int minimal,
                  ptrdiff_t* xmid, ptrdiff_t* ymid, int* lo_minimal, int* hi_minimal)
{
  const uint32_t* a = ctx->a;
  const uint32_t* b = ctx->b;
  ptrdiff_t* fwd = ctx->fwd;
  ptrdiff_t* bwd = ctx->bwd;
  ptrdiff_t kmin = x0 - y1, kmax = x1 - y0; /* diagonals inside the box */
  ptrdiff_t fk = x0 - y0, bk = x1 - y1;     /* where each search starts */
  ptrdiff_t flo = fk, fhi = fk, blo = bk, bhi = bk;
  int odd = ((fk - bk) & 1) != 0; /* the forward search is the one to meet */

  fwd[fk] = x0;
  bwd[bk] = x1;
  *lo_minimal = *hi_minimal = 1;

  for (ptrdiff_t steps = 1;; steps++) {
    /* one more edit step forward on every diagonal in reach */
    if (flo > kmin)
      fwd[--flo - 1] = FAR_LEFT;
    else
      flo++;
    if (fhi < kmax)
      fwd[++fhi + 1] = FAR_LEFT;
    else
      fhi--;
    for (ptrdiff_t k = fhi; k >= flo; k -= 2) {
      ptrdiff_t from_below = fwd[k - 1], from_above = fwd[k + 1];
      ptrdiff_t x = from_below >= from_above ? from_below + 1 : from_above;
      ptrdiff_t y = x - k;
      while (x < x1 && y < y1 && a[x] == b[y]) {
        x++;
        y++;
      }
      fwd[k] = x;
      if (odd && blo <= k && k <= bhi && bwd[k] <= x) {
        *xmid = x;
        *ymid = y;
        return;
      }
    }

    /* and one more backward */
    if (blo > kmin)
      bwd[--blo - 1] = FAR_RIGHT;
    else
      blo++;
    if (bhi < kmax)
      bwd[++bhi + 1] = FAR_RIGHT;
    else
      bhi--;
    for (ptrdiff_t k = bhi; k >= blo; k -= 2) {
      ptrdiff_t from_below = bwd[k - 1], from_above = bwd[k + 1];
      ptrdiff_t x = from_below < from_above ? from_below : from_above - 1;
      ptrdiff_t y = x - k;
      while (x0 < x && y0 < y && a[x - 1] == b[y - 1]) {
        x--;
        y--;
      }
      bwd[k] = x;
      if (!odd && flo <= k && k <= fhi && x <= fwd[k]) {
        *xmid = x;
        *ymid = y;
        return;
      }
    }

    if (minimal || ctx->limit == 0 || steps < ctx->limit)
      continue;

    /* too many steps: the forward point furthest from the start (most
     * x + y) against the backward point furthest from the end */
    ptrdiff_t fbest = -1, fx = x0;
    for (ptrdiff_t k = fhi; k >= flo; k -= 2) {
      ptrdiff_t x = fwd[k] < x1 ? fwd[k] : x1;
      ptrdiff_t y = x - k;
      if (y > y1) {
        x = y1 + k;
        y = y1;
      }
      if (x + y > fbest) {
        fbest = x + y;
        fx = x;
      }
    }
    ptrdiff_t bbest = PTRDIFF_MAX, bx = x1;
    for (ptrdiff_t k = bhi; k >= blo; k -= 2) {
      ptrdiff_t x = bwd[k] > x0 ? bwd[k] : x0;
      ptrdiff_t y = x - k;
      if (y < y0) {
        x = y0 + k;
        y = y0;
      }
      if (x + y < bbest) {
        bbest = x + y;
        bx = x;
      }
    }
    if ((x1 + y1) - bbest < fbest - (x0 + y0)) {
      *xmid = fx;
      *ymid = fbest - fx;
      *hi_minimal = 0;
    } else {
      *xmid = bx;
      *ymid = bbest - bx;
      *lo_minimal = 0;
    }
    return;
  }
}

static void compare(struct DiffContext* ctx, ptrdiff_t x0, ptrdiff_t x1, ptrdiff_t y0, ptrdiff_t y1, int minimal)
{
  /* the common prefix and suffix are not part of any difference */
  while (x0 < x1 && y0 < y1 && ctx->a[x0] == ctx->b[y0]) {
    x0++;
    y0++;
  }
  while (x0 < x1 && y0 < y1 && ctx->a[x1 - 1] == ctx->b[y1 - 1]) {
    x1--;
    y1--;
  }

  if (x0 == x1) {
    mark(ctx->bChanged, y0, y1);
  } else if (y0 == y1) {
    mark(ctx->aChanged, x0, x1);
  } else {
    ptrdiff_t xmid, ymid;
    int lo_minimal, hi_minimal;
    split(ctx, x0, x1, y0, y1, minimal, &xmid, &ymid, &lo_minimal, &hi_minimal);
    compare(ctx, x0, xmid, y0, ymid, lo_minimal);
    compare(ctx, xmid, x1, ymid, y1, hi_minimal);
  }
}

/* The step limit NQ_DIFF_COST_LIMIT sets for sequences of n elements in
 * all: about the square root of n, rounded up to a power of 2, but at
 * least 4096 */
static ptrdiff_t step_limit(unsigned flags, ptrdiff_t n)
{
  if (!(flags & NQ_DIFF_COST_LIMIT))
    return 0;
  ptrdiff_t limit = 1;
  for (size_t diagonals = (size_t)n + 3; diagonals != 0; diagonals >>= 2)
    limit <<= 1;
  return limit > 4096 ? limit : 4096;
}

/* Slides each run of changes in x over the equal elements around it: up
 * as far as it goes, merging with runs before it, then down as far as it
 * goes, merging with runs after it, and then back up to the last place
 * where it faces a run of changes in y, so the two read as one change.
 * Which elements are changed moves, how many does not. */
static int compact(uint8_t* xChanged, const uint32_t* x, ptrdiff_t nx, const uint8_t* yChanged, ptrdiff_t ny)
{
  /* where each unchanged element of y is; the n-th unchanged elements of
   * x and y are paired */
  ptrdiff_t* partner = NQMalloc(((size_t)ny + 1) * sizeof(ptrdiff_t));
  if (!partner)
    return -NQ_ENOMEM;
  ptrdiff_t unchanged = 0;
  for (ptrdiff_t j = 0; j < ny; j++) {
    if (!yChanged[j])
      partner[unchanged++] = j;
  }
  partner[unchanged] = ny;

  /* u: unchanged elements of x before the end of the run. The run faces
   * changes in y when the element of y paired with the one after the run
   * (or the end) has a changed element before it. */
#define FACES_CHANGE(u) (partner[u] > 0 && yChanged[partner[u] - 1])

  ptrdiff_t i = 0, u = 0;
  for (;;) {
    while (i < nx && !xChanged[i]) {
      i++;
      u++;
    }
    if (i == nx)
      break;
    ptrdiff_t start = i;
    while (i < nx && xChanged[i])
      i++;

    ptrdiff_t length, facing;
    do {
      length = i - start;
      while (start > 0 && x[start - 1] == x[i - 1]) {
        xChanged[--start] = 1;
        xChanged[--i] = 0;
        u--;
        while (start > 0 && xChanged[start - 1])
          start--;
      }
      facing = FACES_CHANGE(u) ? i : nx;
      while (i < nx && x[start] == x[i]) {
        xChanged[start++] = 0;
        xChanged[i++] = 1;
        u++;
        while (i < nx && xChanged[i])
          i++;
        if (FACES_CHANGE(u))
          facing = i;
      }
    } while (length != i - start);

    while (facing < i) {
      xChanged[--start] = 1;
      xChanged[--i] = 0;
      u--;
    }
  }
#undef FACES_CHANGE

  NQFree(partner);
  return 0;
}

/* --- setting lines aside ---------------------------------------------- *
 * Before the search, elements with nothing equal on the other side are set
 * aside as changed: no script keeps them. So are elements equal to many on
 * the other side ("}" and blank lines in source files), but only inside a
 * stretch of set-aside elements, where taking them out keeps the search
 * from pairing them across unrelated text. The search then runs on what is
 * left, which is quicker, and its marks are put back in place. This is
 * the pass GNU diff makes unless asked for a minimal script. */

struct IdCount {
  uint32_t id;
  uint32_t used;
  size_t count[2];
};

struct IdTable {
  struct IdCount* slots;
  size_t mask;
};

static struct IdCount* id_slot(struct IdTable* t, uint32_t id)
{
  size_t i = (size_t)(id * 2654435761u) & t->mask;
  while (t->slots[i].used && t->slots[i].id != id)
    i = (i + 1) & t->mask;
  t->slots[i].used = 1;
  t->slots[i].id = id;
  return &t->slots[i];
}

enum { KEEP = 0, SET_ASIDE = 1, MAYBE_ASIDE = 2 };

/* Keeps the "maybe" elements of a stretch [start, end) that starts and
 * ends with elements definitely set aside, except as the rules allow */
static void settle_stretch(uint8_t* aside, ptrdiff_t start, ptrdiff_t end)
{
  ptrdiff_t length = end - start, maybe = 0;
  for (ptrdiff_t k = start; k < end; k++)
    maybe += aside[k] == MAYBE_ASIDE;

  /* a stretch that is more than a quarter "maybe" keeps them all */
  if (maybe * 4 > length) {
    for (ptrdiff_t k = start; k < end; k++) {
      if (aside[k] == MAYBE_ASIDE)
        aside[k] = KEEP;
    }
    return;
  }

  /* a run of "maybe" elements longer than about the square root of a
   * quarter of the stretch is kept, as it may well be real text */
  ptrdiff_t longest = 1;
  for (ptrdiff_t q = length >> 2; (q >>= 2) > 0;)
    longest <<= 1;
  longest++;
  ptrdiff_t run = 0;
  for (ptrdiff_t k = 0; k < length; k++) {
    if (aside[start + k] != MAYBE_ASIDE) {
      run = 0;
    } else if (++run == longest) {
      k -= run; /* back to the start of the run, keeping it all */
    } else if (run > longest) {
      aside[start + k] = KEEP;
    }
  }

  /* near either end, "maybe" elements are kept until three definite ones
   * in a row, or a definite one at least 8 in */
  for (int dir = 0; dir < 2; dir++) {
    ptrdiff_t definite = 0;
    for (ptrdiff_t k = 0; k < length; k++) {
      ptrdiff_t at = dir == 0 ? start + k : end - 1 - k;
      if (k >= 8 && aside[at] == SET_ASIDE)
        break;
      if (aside[at] == MAYBE_ASIDE) {
        definite = 0;
        aside[at] = KEEP;
      } else if (aside[at] == KEEP) {
        definite = 0;
      } else if (++definite == 3) {
        break;
      }
    }
  }
}

/* Decides which of x[0, n) to set aside, given how often each id occurs
 * on the other side */
static void choose_aside(struct IdTable* t, const uint32_t* x, ptrdiff_t n, int other, uint8_t* aside)
{
  /* "many" grows with about the square root of the length */
  size_t many = 5;
  for (ptrdiff_t q = n / 64; (q >>= 2) > 0;)
    many *= 2;

  for (ptrdiff_t i = 0; i < n; i++) {
    size_t matches = id_slot(t, x[i])->count[other];
    aside[i] = matches == 0 ? SET_ASIDE : matches > many ? MAYBE_ASIDE : KEEP;
  }

  for (ptrdiff_t i = 0; i < n; i++) {
    if (aside[i] == MAYBE_ASIDE) {
      aside[i] = KEEP; /* not inside a stretch */
      continue;
    }
    if (aside[i] == KEEP)
      continue;
    /* a stretch, from here to the next kept element, without the
     * "maybe" elements at its end */
    ptrdiff_t end = i;
    while (end < n && aside[end] != KEEP)
      end++;
    while (end > i && aside[end - 1] == MAYBE_ASIDE)
      aside[--end] = KEEP;
    settle_stretch(aside, i, end);
    i = end - 1;
  }
}

/* The search, on the elements of a[0, na) and b[0, nb) not set aside */
static int compare_kept(const uint32_t* a, ptrdiff_t na, const uint32_t* b, ptrdiff_t nb, unsigned flags,
                        uint8_t* aChanged, uint8_t* bChanged)
{
  int result = -NQ_ENOMEM;
  size_t size = 64;
  while (size < (size_t)(na + nb) * 2)
    size *= 2;
  struct IdTable table = {NQCalloc(size, sizeof(struct IdCount)), size - 1};
  uint8_t* aside[2] = {NQMalloc((size_t)na + 1), NQMalloc((size_t)nb + 1)};
  uint32_t* kept[2] = {NQMalloc(((size_t)na + 1) * sizeof(uint32_t)), NQMalloc(((size_t)nb + 1) * sizeof(uint32_t))};
  ptrdiff_t* where[2] = {NQMalloc(((size_t)na + 1) * sizeof(ptrdiff_t)), NQMalloc(((size_t)nb + 1) * sizeof(ptrdiff_t))};
  uint8_t* marks[2] = {NQCalloc((size_t)na + 1, 1), NQCalloc((size_t)nb + 1, 1)};
  ptrdiff_t* fwd = NQMalloc(((size_t)(na + nb) + 3) * sizeof(ptrdiff_t));
  ptrdiff_t* bwd = NQMalloc(((size_t)(na + nb) + 3) * sizeof(ptrdiff_t));
  if (!table.slots || !aside[0] || !aside[1] || !kept[0] || !kept[1] || !where[0] || !where[1] || !marks[0] ||
      !marks[1] || !fwd || !bwd)
    goto done;

  const uint32_t* seq[2] = {a, b};
  ptrdiff_t len[2] = {na, nb};
  uint8_t* changed[2] = {aChanged, bChanged};
  for (int s = 0; s < 2; s++) {
    for (ptrdiff_t i = 0; i < len[s]; i++)
      id_slot(&table, seq[s][i])->count[s]++;
  }
  ptrdiff_t nkept[2];
  for (int s = 0; s < 2; s++) {
    choose_aside(&table, seq[s], len[s], !s, aside[s]);
    nkept[s] = 0;
    for (ptrdiff_t i = 0; i < len[s]; i++) {
      if (aside[s][i] != KEEP) {
        changed[s][i] = 1;
      } else {
        kept[s][nkept[s]] = seq[s][i];
        where[s][nkept[s]++] = i;
      }
    }
  }

  struct DiffContext ctx = {
    kept[0], kept[1], marks[0], marks[1], fwd + nkept[1] + 1, bwd + nkept[1] + 1, step_limit(flags, nkept[0] + nkept[1])
  };
  compare(&ctx, 0, nkept[0], 0, nkept[1], 0);
  for (int s = 0; s < 2; s++) {
    for (ptrdiff_t i = 0; i < nkept[s]; i++) {
      if (marks[s][i])
        changed[s][where[s][i]] = 1;
    }
  }
  result = 0;

done:
  NQFree(table.slots);
  for (int s = 0; s < 2; s++) {
    NQFree(aside[s]);
    NQFree(kept[s]);
    NQFree(where[s]);
    NQFree(marks[s]);
  }
  NQFree(fwd);
  NQFree(bwd);
  return result;
}

int NQDiff_mark(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength, unsigned flags,
                uint8_t* aChanged, uint8_t* bChanged)
{
  int result = 0;

  if (aLength > PTRDIFF_MAX / 8 || bLength > PTRDIFF_MAX / 8)
    return -NQ_ENOMEM;
  NQMemset(aChanged, 0, aLength);
  NQMemset(bChanged, 0, bLength);

  const uint32_t* x = a;
  const uint32_t* y = b;
  ptrdiff_t nx = (ptrdiff_t)aLength, ny = (ptrdiff_t)bLength;

  if (flags & NQ_DIFF_DISCARD) {
    result = compare_kept(x, nx, y, ny, flags, aChanged, bChanged);
  } else {
    ptrdiff_t* fwd = NQMalloc(((size_t)(nx + ny) + 3) * sizeof(ptrdiff_t));
    ptrdiff_t* bwd = NQMalloc(((size_t)(nx + ny) + 3) * sizeof(ptrdiff_t));
    if (fwd && bwd) {
      /* diagonals run from -(ny + 1) to nx + 1 */
      struct DiffContext ctx = {
        x, y, aChanged, bChanged, fwd + ny + 1, bwd + ny + 1, step_limit(flags, nx + ny)
      };
      compare(&ctx, 0, nx, 0, ny, 0);
    } else {
      result = -NQ_ENOMEM;
    }
    NQFree(fwd);
    NQFree(bwd);
  }

  if (result == 0 && (flags & NQ_DIFF_COMPACT)) {
    result = compact(aChanged, a, (ptrdiff_t)aLength, bChanged, (ptrdiff_t)bLength);
    if (result == 0)
      result = compact(bChanged, b, (ptrdiff_t)bLength, aChanged, (ptrdiff_t)aLength);
  }
  return result;
}

int NQDiff_hunks(const uint8_t* aChanged, size_t aLength, const uint8_t* bChanged, size_t bLength,
                 NQDiffHunk** hunks, size_t* count)
{
  size_t capacity = 0, n = 0;
  NQDiffHunk* list = NULL;
  size_t i = 0, j = 0;

  *hunks = NULL;
  *count = 0;

  /* unchanged elements pair up in order, so walking both sequences
   * together, each run of changes between two such pairs is one hunk */
  while (i < aLength || j < bLength) {
    if (i < aLength && j < bLength && !aChanged[i] && !bChanged[j]) {
      i++;
      j++;
      continue;
    }

    NQDiffHunk hunk;
    hunk.aStart = i;
    hunk.bStart = j;
    while (i < aLength && aChanged[i])
      i++;
    while (j < bLength && bChanged[j])
      j++;
    hunk.aCount = i - hunk.aStart;
    hunk.bCount = j - hunk.bStart;
    if (hunk.aCount == 0 && hunk.bCount == 0) {
      /* the rest of one side unchanged, the other at its end: marks that
       * do not pair up; take the rest as changed */
      hunk.aCount = aLength - i;
      hunk.bCount = bLength - j;
      i = aLength;
      j = bLength;
    }

    if (n == capacity) {
      size_t grown = capacity ? capacity * 2 : 16;
      NQDiffHunk* bigger = NQRealloc(list, grown * sizeof(NQDiffHunk));
      if (!bigger) {
        NQFree(list);
        return -NQ_ENOMEM;
      }
      list = bigger;
      capacity = grown;
    }
    list[n++] = hunk;
  }

  *hunks = list;
  *count = n;
  return 0;
}

int NQDiff_computeEx(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength, unsigned flags, NQDiffHunk** hunks, size_t* count)
{
  *hunks = NULL;
  *count = 0;

  uint8_t* aChanged = NQMalloc(aLength + 1);
  uint8_t* bChanged = NQMalloc(bLength + 1);

  int result = (aChanged && bChanged) ? NQDiff_mark(a, aLength, b, bLength, flags, aChanged, bChanged) : -NQ_ENOMEM;
  if (result == 0)
    result = NQDiff_hunks(aChanged, aLength, bChanged, bLength, hunks, count);

  NQFree(aChanged);
  NQFree(bChanged);
  return result;
}

int NQDiff_compute(const uint32_t* a, size_t aLength, const uint32_t* b, size_t bLength, NQDiffHunk** hunks, size_t* count)
{
  return NQDiff_computeEx(a, aLength, b, bLength, 0, hunks, count);
}

void NQDiff_free(NQDiffHunk* hunks)
{
  NQFree(hunks);
}
