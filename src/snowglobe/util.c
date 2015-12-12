//a collection of utility methods

#include "snowglobe-internal.h"
#include <math.h>
#include <float.h>



float randf (float x) { //return random number in range [0,x)
	return rand()/(((double)RAND_MAX + 1) / x);
}
float minimum (float x, float y) {
	return ((x) < (y) ? (x) : (y));
}
float maximum (float x, float y) {
	return ((x) > (y) ? (x) : (y));
}
float symmDistr() { //returns number in range [-1, 1] with bias towards 0, symmetric about 0.
	float x = 2*randf(1)-1;
	return x*(1-cbrt(1-fabsf(x)));
}
