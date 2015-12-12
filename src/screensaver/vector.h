#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <compiz-core.h>

typedef enum
{
	x,
	y,
	z
} VectorCoordsEnum;

class Vector
{
public:
	static const Vector null;

        Vector()
	{
	    for(int i = 0; i<3; i++) {
		v[i] = 0;
	    }
	}
	Vector( float x, float y, float z ) { v[0] = x; v[1] = y; v[2] = z; }
	Vector( const float* vect ) { v[0] = vect[0]; v[1] = vect[1]; v[2] = vect[2]; }
	Vector( const Vector& vect ) { v[0] = vect[0]; v[1] = vect[1]; v[2] = vect[2]; }

	float norm() { return sqrt( (*this) * (*this) ); }
	Vector& normalize()
	{
		float n = norm();
		if( n == 0.0 )
			v[x] = v[y] = v[z] = 1.0;
		else *this /= n;
		return *this;
	}

	Vector toScreenSpace( CompScreen* s ) const
	{
		Vector res;
		res[0] = v[0]/s->width - 0.5;
		res[1] = 0.5 - v[1]/s->height;
		res[2] = v[2];
		return res;
	}

	Vector toCoordsSpace( CompScreen *s ) const
	{
		Vector res;
		res[0] = ( v[0] + 0.5 )*s->width;
		res[1] = ( 0.5 - v[1] )*s->height;
		res[2] = v[2];
		return res;
	}

	float& operator[]( int i ) { return v[i]; }
	const float& operator[]( int i ) const { return v[i]; }
	float& operator[]( VectorCoordsEnum c ) { return v[(int)c]; }
	const float& operator[]( VectorCoordsEnum c ) const { return v[(int)c]; }

	Vector& operator+=( const Vector& rhs )
	{
		v[0] += rhs[0];
		v[1] += rhs[1];
		v[2] += rhs[2];
		return *this;
	}

	friend Vector operator+( const Vector& lhs, const Vector& rhs )
	{
		Vector res;
		res[0] = lhs[0] + rhs[0];
		res[1] = lhs[1] + rhs[1];
		res[2] = lhs[2] + rhs[2];
		return res;
	}

	Vector& operator-=( const Vector& rhs )
	{
		v[0] -= rhs[0];
		v[1] -= rhs[1];
		v[2] -= rhs[2];
		return *this;
	}

	friend Vector operator-( const Vector& lhs, const Vector& rhs )
	{
		Vector res;
		res[0] = lhs[0] - rhs[0];
		res[1] = lhs[1] - rhs[1];
		res[2] = lhs[2] - rhs[2];
		return res;
	}

	friend Vector operator-( const Vector& vect )
	{
		Vector res;
		res[0] = -vect[0];
		res[1] = -vect[1];
		res[2] = -vect[2];
		return res;
	}

	Vector& operator*=( const float k )
	{
		v[0] *= k;
		v[1] *= k;
		v[2] *= k;
		return *this;
	}

	friend float operator*( const Vector& lhs, const Vector& rhs )
	{
		return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
	}

	friend Vector operator*( const float k, const Vector& vect )
	{
		Vector res;
		res[0] = k * vect[0];
		res[1] = k * vect[1];
		res[2] = k * vect[2];
		return res;
	}

	friend Vector operator*( const Vector& v, const float k ) { return k*v; }

	Vector& operator/=( const float k )
	{
		v[0] /= k;
		v[1] /= k;
		v[2] /= k;
		return *this;
	}

	friend Vector operator/( const Vector& vect, const float k )
	{
		Vector res;
		res[0] = vect[0] / k;
		res[1] = vect[1] / k;
		res[2] = vect[2] / k;
		return res;
	}

	Vector& operator^=( const Vector& vect )
	{
		*this = *this ^ vect;
		return *this;
	}

	friend Vector operator^( const Vector& lhs, const Vector& rhs );

	float v[3];
};

typedef Vector Point;

#endif
