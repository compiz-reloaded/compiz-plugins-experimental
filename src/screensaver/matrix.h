#ifndef MATRIX_H
#define MATRIX_H

#include <string.h>
#include <compiz-core.h>
#include "vector.h"

class Matrix
{
public:
	static const Matrix identity;

	Matrix() {
	    for (int i=0; i<16;i++) {
		m[i] = 0;
	    }
	}

	Matrix( const CompTransform* mat ) { memcpy( m, mat->m, sizeof(m) ); }
	Matrix( const Matrix& mat ) { memcpy( m, mat.m, sizeof(m) ); }
	Matrix( const float* mat ) { memcpy( m, mat, sizeof(m) ); }

	const float& operator[]( int i ) const { return m[i]; }
	float& operator[]( int i ) { return m[i]; }

	Matrix& operator*=( const Matrix& rhs ) { Matrix res; *this = *this * rhs; return *this; }
	friend Matrix operator*( const Matrix& lhs, const Matrix& rhs );
	friend Vector operator*( const Matrix& mat, const Vector& vect );

	friend Matrix interpolate( const Matrix& from, const Matrix& to, float position );

	Matrix& rotate( float angle, float x, float y, float z )
	{
		matrixRotate( (CompTransform*)m, angle, x, y, z );
		return *this;
	}

	Matrix& rotate( float angle, const Vector& vect ) { return rotate( angle, vect[x], vect[y], vect[z] ); }

	Matrix& scale( float x, float y, float z )
	{
		matrixScale( (CompTransform*)m, x, y, z );
		return *this;
	}

	Matrix& scale( const Vector& vect ) { return scale( vect[x], vect[y], vect[z] ); }

	Matrix& translate( float x, float y, float z )
	{
		matrixTranslate( (CompTransform*)m, x, y, z );
		return *this;
	}

	Matrix& translate( const Vector& vect ) { return translate( vect[x], vect[y], vect[z] ); }

	float m[16];
};

#endif
