#pragma once

#include "cwtmath.h"

#ifdef __cplusplus
extern "C" {
#endif

void meyer(const int N, const double lb, const double ub, double* phi, double* psi, double* tgrid);
void gauss(int N, int p, double lb, double ub, double* psi, double* t);
void mexhat(int N, double lb, double ub, double* psi, double* t);
void morlet(int N, double lb, double ub, double* psi, double* t);

#ifdef __cplusplus
}
#endif
