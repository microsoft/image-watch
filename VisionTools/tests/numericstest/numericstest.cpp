// test_numerics.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "vtnumerics.h"
#include <iostream>
#include <random>

using namespace vt;

class CRand
{
public:
    CRand(int seed = 1) : engine(seed) {}
    double DRand()
    {
        return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
    }
    void Seed(int seed)
    {
        engine.seed(seed);
    }
private:
    std::mt19937 engine;
};

CRand g_rnd;
unsigned int g_numerics_errors = 0xffffffff;

template <class T>
CMtx<T> RandMatrix(int m,int n)
{
#pragma warning(disable: 4305)  // OK to truncate doubles if T is float here
#pragma warning(disable: 4838)  // OK to truncate doubles if T is float here
    T d[]= {
        0.582792, 0.423496, 0.515512, 0.333951, 0.432907, 0.225950, 0.579807, 0.760365, 0.529823, 0.640526,
        0.209069, 0.379818, 0.783329, 0.680846, 0.461095, 0.567829, 0.794211, 0.059183, 0.602869, 0.050269,
        0.415375, 0.304999, 0.874367, 0.015009, 0.767950, 0.970845, 0.990083, 0.788862, 0.438659, 0.498311,
        0.213963, 0.643492, 0.320036, 0.960099, 0.726632, 0.411953, 0.744566, 0.267947, 0.439924, 0.933380,
        0.683332, 0.212560, 0.839238, 0.628785, 0.133773, 0.207133, 0.607199, 0.629888, 0.370477, 0.575148,
        0.451425, 0.043895, 0.027185, 0.312685, 0.012863, 0.383967, 0.683116, 0.092842, 0.035338, 0.612395,
        0.608540, 0.015760, 0.016355, 0.190075, 0.586918, 0.057581, 0.367568, 0.631451, 0.717634, 0.692669,
        0.084079, 0.454355, 0.441828, 0.353250, 0.153606, 0.675645, 0.699213, 0.727509, 0.478384, 0.554842,
        0.121047, 0.450754, 0.715883, 0.892842, 0.273102, 0.254769, 0.865603, 0.232350, 0.804872, 0.908398,
        0.231894, 0.239313, 0.049754, 0.078384, 0.640815, 0.190887, 0.843869, 0.173900, 0.170793, 0.994295
    };
#pragma warning(default: 4305)
#pragma warning(default: 4838)

    if (m * n <= (int)((sizeof d) / (sizeof d[0]))) 
    {
        CMtx<T> M(m, n);
        M.Init(d, m*n);
        return M;
    }
    else {
        CMtx<T> M(m, n);
        for (int i=0; i<m; ++i)
            for (int j=0; j<n; ++j)
                M(i, j) = (T)g_rnd.DRand();
        return M;
    }
}

template <typename T>
T MaxAbs(CMtx<T> const &M)
{
    int i,j;
    M.MaxAbs(i,j);
    return M(i,j);
}

#undef small

#ifndef VT_GCC
bool test_round(const float tests[], const int expected[], int count,
    CFloatRoundMode::RoundMode rm, const char* desc)
{
    CFloatRoundMode roundMode(rm);

    bool success = true;
    std::cout << "Test " << desc;
    for (int i = 0; i < count; i++)
    {
        int result = F2I(tests[i]);
        if (result != expected[i])
        {
            ++ g_numerics_errors;
            if (success)
            {
                std::cout << " FAILED" << std::endl;
            }
            std::cout << "  FAILED case " << i << ":" << tests[i] <<
                "rounded to " << result << ", expected " << expected[i] <<
                std::endl;
            success = false;
        }
    }
    if (success)
    {
        std::cout << " PASSED." << std::endl;
    }
    return success;
}
#endif

int main( int argc, char **argv)
{
    g_rnd.Seed(890234);
    g_numerics_errors = 0;

    // test vector
    if(1)
    {
        std::cout << " ************ Testing vector " << std::endl;

        double d[] = { 1,2,3,4,5,6,6,3,1,5,6,1,2 };

        CVec<double> v( sizeof(d)/sizeof(double));
        v.Init(d, v.Size());

        CVec<double> u= v;

        u[2]= 123;

        std::cout << "V = ";
        v.Dump();
        std::cout << "U = ";
        u.Dump();

        v= u-u;
        std::cout << "V = ";
        v.Dump();
    }

    //test complex
    if(1)
    {
        std::cout << " ************ Testing complex " << std::endl;

        CMtx<Complex<double> > A(3,3);
        A(0,0)= Complex<double>(1,2);
        A(0,1)= Complex<double>(3,7);
        A(0,2)= Complex<double>(5,3);
        A(1,0)= Complex<double>(2,4);
        A(1,1)= Complex<double>(4,1);
        A(1,2)= Complex<double>(1,7);
        A(2,0)= Complex<double>(8,3);
        A(2,1)= Complex<double>(5,4);
        A(2,2)= Complex<double>(1,8);

        std::cout << "A = ";
        A.Dump();
        std::cout << "A*A = ";
        (A*A).Dump();
        //std::cout << "det(A) = " << A.det() << std::endl;

        A.GetCol(0).Dump();
        (A(0,0)*A.GetCol(0)).Dump();
    }

#ifndef VT_GCC
    // test floating point rounding
    if(1)
    {
        std::cout << " ************ Testing Floating Point Rounding" << std::endl;

        const int kNumTests = 4;
        float tests[kNumTests] = { 1.2f, 1.8f, -1.2f, -1.8f };
        bool success = true;

        // RM_NEAREST
        {
            int expected[kNumTests] = { 1, 2, -1, -2 };
            success &= test_round(tests, expected, 4,
                CFloatRoundMode::RM_NEAREST, "CFloatRoundMode(RM_NEAREST)");
        }

        // RM_UP
        {
            int expected[kNumTests] = { 2, 2, -1, -1 };
            success &= test_round(tests, expected, 4,
                CFloatRoundMode::RM_UP, "CFloatRoundMode(RM_UP)");
        }

        // RM_DOWN
        {
            CFloatRoundMode roundMode(CFloatRoundMode::RM_DOWN);

            int expected[kNumTests] = { 1, 1, -2, -2 };
            success &= test_round(tests, expected, 4,
                CFloatRoundMode::RM_DOWN, "CFloatRoundMode(RM_DOWN)");
        }

        // RM_TRUNCATE
        {
            CFloatRoundMode roundMode(CFloatRoundMode::RM_TRUNCATE);

            int expected[kNumTests] = { 1, 1, -1, -1 };
            success &= test_round(tests, expected, 4,
                CFloatRoundMode::RM_TRUNCATE, "CFloatRoundMode(RM_TRUNCATE)");
        }
    }
#endif

    std::cout << std::endl << g_numerics_errors << " numerics errors."
              << std::endl;

    return g_numerics_errors;
}
