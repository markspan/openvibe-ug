/*************************************************************************************/
/* spline.hpp                                                                      */
/*************************************************************************************/

/*************************************************************************************/
/* spline  subroutines                                                               */
/*************************************************************************************/
#pragma once

int SplineTables(int order, double* pot, double* scd);
int SplineCoef(int n, double** xyz, const double* values, const double* table, double* coef);
double SplineInterp(int n, double** xyz, const double* table, const double* coef, double xx, double yy, double zz);
