#ifndef UTILITY_H_
#define UTILITY_H_
#include <vector>
#include <math.h>
#include <bits/stdc++.h> 

namespace Utility {
    // Cholesky decomposition of a positive definite matrix. Pseudo code can be found here:
    // http://www.mosismath.com/Cholesky/Cholesky.html
    bool choleskyDecomposition(const std::vector<std::vector<double>> &matIn, std::vector<std::vector<double>>& matOut) {
        std::size_t maxOrder(matIn.size());
        for (std::size_t i = 0; i < maxOrder; i++)
        {
            for (std::size_t j = 0; j <= i; j++)
            {
                if (i > j)
                {
                    matOut[i][j] = matIn[i][j];
                    for (std::size_t k = 0; k < j; k++)
                        matOut[i][j] = matOut[i][j] - matOut[i][k] * matOut[j][k];
                    // Avoid division by zero
                    if (matOut[j][j] < 1.e-8)
                        return false;
                    else
                        matOut[i][j] = matOut[i][j] / matOut[j][j];
                }
                if (i == j)
                {
                    matOut[i][i] = matIn[i][i];
                    for (std::size_t k = 0; k < j; k++)
                    {
                        matOut[i][i] = matOut[i][i] - (matOut[i][k] * matOut[i][k]);
                    }
                    // Avoid negative roots
                    if (matOut[i][i] < 0)
                        return false;
                    else
                        matOut[i][i] = std::sqrt(matOut[i][i]);
                }
            }
        }
        return true;
    }

    // solving a system of A * x = b for vector x. A is assumed to be in lower triangular form.
    // Code adapted from: https://stackoverflow.com/questions/22237322/solve-ax-b-a-lower-triangular-matrix-in-c
    bool backwardSubstitution(const std::vector<std::vector<double>>& a,
                                        const std::vector<double>& b,
                                        std::vector<double>& x)
    {
        double s;
        size_t maxOrder = a.size();
        // solve lower triangular matrix
        for (std::size_t i = 0; i < maxOrder; i++)
        {
            s = 0.0;
            for (std::size_t j = 0; j < i; j++)
            {
                s = s + a[i][j] * x[j];
            }
            x[i] = (b[i] - s) / a[i][i];
        }
        return true;
    }
}

#endif /* UTILITY_H_ */