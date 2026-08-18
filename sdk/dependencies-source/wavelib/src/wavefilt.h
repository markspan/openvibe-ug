/*
Copyright (c) 2014, Rafat Hussain
Copyright (c) 2016, Holger Nahrstaedt
*/
#pragma once

#include <stdio.h>
#include "conv.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif


int filtlength(const char* name);

int filtcoef(const char* name, double* lp1, double* hp1, double* lp2, double* hp2);

void copy_reverse(const double* in, const int N, double* out);
void qmf_even(const double* in, const int N, double* out);
void qmf_wrev(const double* in, const int N, double* out);
void copy(const double* in, const int N, double* out);
#ifdef __cplusplus
}
#endif
