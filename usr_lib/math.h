#pragma once

#include <stddef.h>
#define PI 3.141592654

double pow(double x, double y);
float powf(float x, float y);
#ifdef sse
long double powl(long double x, long double y);
#endif
int isinf(double x);
int isnan(double x);

double trunc(double x);
double floor(double x);
double ceil(double x);
double fabs(double x);
double sin(double x);
double cos(double x);
