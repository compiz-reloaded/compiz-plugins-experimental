#include "vector.h"

const Vector Vector::null( 0.0, 0.0, 0.0 );

Vector operator^( const Vector& lhs, const Vector& rhs )
{
	Vector res;

	res[0] = lhs[1]*rhs[2] - lhs[2]*rhs[1];
	res[1] = lhs[2]*rhs[0] - lhs[0]*rhs[2];
	res[2] = lhs[0]*rhs[1] - lhs[1]*rhs[0];

	return res;
}
