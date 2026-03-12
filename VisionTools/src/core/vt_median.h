///////////////////////////////////////////////////////////////////////////
//
// NAME
//  vt_median.h -- templated function for median computation
//
// DESCRIPTION
//  The templated VisMedian function computes the median
//  (or other order statisticelement) of a list of objects
//  using the expected linear time randomized select algorithm
//  [Introduction to Algorithms, Cormen, Leiserson & Rivest, 1992, p. 187]
//
//  The algorithm is re-written slightly, and all of the code for
//  Randomized-Partition and Partition is folded in.
//
//  Note that this routine **destructively** re-orders the inputs
//
//  The first (1997) version of the code used an STL-like
//  function object that looked like:
//      struct FloatCompare {  // compare floats
//          int operator()(float& x, float& y) { return x <= y; }
//      }
//  float l[4] = {3, 4, 2, 1};
//  float med = VisMedian(FloatCompare(), l, 4, 4/2);
//
//  The new version doesn't require a fuction object for
//  built-in numeric types that have a > operator, and uses
//  and a defined operator> function, as shown in
//  the commented out self-test code at the end of this file.
//
// DESIGN
//  In the future, I want to add code to do a "weighted median", so
//  that we can combined the benefits of feathering and median filtering.
//
// HISTORY
//  10-Sep-97  Richard Szeliski (szeliski) at Microsoft
//  Created.
//
//  2012-12-05  Richard Szeliski (szeliski) at Microsoft
//  Changed to operator> and added WeightedMedian variant.
//
//  2014-10-19  Richard Szeliski (szeliski) at Microsoft
//  Fixed (again) integer overflow bug in random number generation using double precision float
//
// Copyright © Microsoft Corp.
//
///////////////////////////////////////////////////////////////////////////

#pragma once

// #define PRINT_TRACE

template <class TElement>
static TElement VtMedian(TElement *l, int n, int m = -1)
{
    if (m < 0)
        m = n/2;
    if (n <= 1)
        return l[0];

    // Use the expected linear time algorithm from Leiserson's book...
    int r = static_cast<int>(rand() * static_cast<double>(n) / (static_cast<double>(RAND_MAX)+1));
    //assert(0 <= r && r < n);
    TElement p = l[r];        // pivot
    l[r] = l[0];
    l[0] = p;

    // Re-arrange the array into lower and upper "halves"
    int j = n;
    for (int i = -1; i < j; ) {
        do i++; while (p > l[i]);
        do j--; while (l[j] > p);
        if (i < j) {
            TElement t = l[i];
            l[i] = l[j];
            l[j] = t;
        }
    }

    // Recurse on the proper part
    j += 1;
    if (m < j)
        return VtMedian(l, j, m);
    else
        return VtMedian(&l[j], n-j, m-j);
}

template <class T1, class T2>
static T1 VtWeightedtMedianWTarget(T1 *l, T2 *w, int n, T2 m)
{
#ifdef PRINT_TRACE
    printf("l = "); for (int k = 0; k < n; k++) printf("%d%c", l[k], (k < n-1) ? ',' : '\n');
    printf("w = "); for (int k = 0; k < n; k++) printf("%d%c", int(w[k]), (k < n-1) ? ',' : '\n');
#endif
    if (n <= 1)
        return l[0];

    // Use the expected linear time algorithm from Leiserson's book...
    int r = static_cast<int>(rand() * static_cast<double>(n) / (static_cast<double>(RAND_MAX) + 1));
    // assert(0 <= r && r < n);
    T1 p = l[r];        // pivot
#ifdef PRINT_TRACE
    printf(" exchange p = %d, l[%d] = %d, l[%d] = %d\n", p, 0, l[0], r, l[r]);
#endif
    l[r] = l[0];
    l[0] = p;
    T2 t2 = w[r];        // pivot
    w[r] = w[0];
    w[0] = t2;

    // Re-arrange the array into lower and upper "halves"
    int i = -1, j = n;
    T2 lSum = 0;
    while (i < j) {
        i++;
        while (p > l[i])
        {
            lSum += w[i];
            i++;
        }
        j--;
        while (l[j] > p)
        {
            j--;
        }
        if (i < j) {
#ifdef PRINT_TRACE
            printf(" exchange p = %d, l[%d] = %d, l[%d] = %d\n", p, i, l[i], j, l[j]);
#endif
            T1 t1 = l[i];
            l[i] = l[j];
            l[j] = t1;
            T2 t2 = w[i];
            w[i] = w[j];
            w[j] = t2;
            lSum += w[i];
        }
    }

    // Recurse on the proper part
    if (i == j)
        lSum += w[i];   // haven't yet added this in
    j += 1;
    if (m < lSum)
        return VtWeightedtMedianWTarget(l, w, j, m);
    else
        return VtWeightedtMedianWTarget(&l[j], &w[j], n-j, m - lSum);
}

template <class T1, class T2>
static T1 VtWeightedtMedian(T1 *l, T2 *w, int n, float frac = 0.5f)
{
    // Median with a fraction of total weights
    T2 totalWt = 0;
    for (int k = 0; k < n; k++)
        totalWt += w[k];
    return VtWeightedtMedianWTarget(l, w, n, T2(totalWt * frac));
}

#if 0   // Self-test code, enable if you want to try it

static void VtMedianTest()
{
    // Sample self-test code
    int sampleList1[7] = {5, 2, 9, 9, 1, 4, 6};
    int answer1 = VtMedian(sampleList1, 7, 3);
    assert(answer1 == 5);   // answer should be 5

    // Median of a structured list
    struct MTpair
    {
        int val;
        float xtra;
        inline int operator>(MTpair& y) { return val > y.val; }
    };
    MTpair mtList2[7] = {{5, 2}, {2, 1}, {9, 0}, {9, 7}, {1, 3}, {4, 2}, {6, 1}};
    MTpair answer2 = VtMedian(mtList2, 7, 3);
    assert(answer2.val == 5);   // answer should be 5

    // Weighted median with uniform weights
    int sampleList3[7] = {5, 2, 9, 9, 1, 4, 6};
    float weightList3[7] = {1, 1, 1, 1, 1, 1, 1};
    int answer3 = VtWeightedtMedian(sampleList3, weightList3, 7);
    assert(answer3 == 5);   // answer should be 5

    // Weighted median with non-uniform weights
    int sampleList4[7] = {5, 2, 9, 9, 1, 4, 6};
    float weightList4[7] = {1, 2, 1, 1, 3, 2, 1};
    int answer4 = VtWeightedtMedian(sampleList4, weightList4, 7);
    assert(answer4 == 4);   // answer should be 4
}

static int CompareInt(const void *p1, const void *p2)
{
    // Sort in increasing order of mean
    const int& i1 = *(int *) p1;
    const int& i2 = *(int *) p2;
    int  s = (i1 < i2) ? -1 : (i1 > i2) ? 1 : 0;
    return s;
}

static void VtMedianTestExhaustive()
{
    // Test all possible assigments against qsort
    const int maxLen = 5;
    const int maxRep = 3;
    static int vals[maxLen];
    static int reps[maxLen];
    static int expd[maxLen*maxRep];

    // Test all list lengths
    for (int len = 1, nLen = maxLen, nRep = maxRep; len <= maxLen; len++, nLen *= maxLen, nRep *= maxRep)
    {
        // Test all exponential patterns
        for (int t = 0; t < nLen; t++)
        {
            for (int r = 1; r < nRep; r++)
            {
                // Fill in the test list and repetition list
                int se = 0;
                for (int k = 0, t2 = t, r2 = r; k < len; k++)
                {
                    vals[k] = t2 % maxLen;
                    reps[k] = r2 % maxRep;
                    t2 /= maxLen;
                    r2 /= maxRep;

                    // Fill in the expanded list
                    for (int l = 0; l < reps[k]; l++, se++)
                        expd[se] = vals[k];
                }

                // Compute the weighted median (destructive)
                int median1 = VtWeightedtMedian(vals, reps, len, 0.5f);

                // Sort the expanded list
                qsort(expd, se, sizeof(int), CompareInt);
                int median2 = expd[se / 2];
                assert(median1 == median2);     // NOTE:  will only trap in DEBUG mode
            }
        }
    }
}
#endif