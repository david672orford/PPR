/* Matrix and vector arithmetic.
 * By Richard W.M. Jones <rich@annexia.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include "../nonppr_misc/c2lib/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "matvec.h"

float identity_matrix[16] = {
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 0, 1
};

float zero_vec[4] = { 0, 0, 0, 1 };

float *
new_identity_matrix (pool pool)
{
  float *m = new_matrix (pool);
  make_identity_matrix (m);
  return m;
}

float *
new_zero_vec (pool pool)
{
  float *v = new_vec (pool);
  make_zero_vec (v);
  return v;
}

/* This code is taken from Mesa 3.0. I have exchanged degrees for radians. */
void
make_rotation_matrix (float angle,
					  float x, float y, float z,
					  float *m)
{
  /* This function contributed by Erich Boleyn (erich@uruk.org) */
  float mag, s, c;
  float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

  s = sin(angle);
  c = cos(angle);

  mag = sqrt ( x*x + y*y + z*z );

  if (mag == 0.0) {
	/* generate an identity matrix and return */
	make_identity_matrix (m);
	return;
  }

  x /= mag;
  y /= mag;
  z /= mag;

#define M(row,col)	m[col*4+row]

  /*
   *	 Arbitrary axis rotation matrix.
   *
   *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
   *  like so:	Rz * Ry * T * Ry' * Rz'.  T is the final rotation
   *  (which is about the X-axis), and the two composite transforms
   *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
   *  from the arbitrary axis to the X-axis then back.	They are
   *  all elementary rotations.
   *
   *  Rz' is a rotation about the Z-axis, to bring the axis vector
   *  into the x-z plane.  Then Ry' is applied, rotating about the
   *  Y-axis to bring the axis vector parallel with the X-axis.	 The
   *  rotation about the X-axis is then performed.	Ry and Rz are
   *  simply the respective inverse transforms to bring the arbitrary
   *  axis back to it's original orientation.  The first transforms
   *  Rz' and Ry' are considered inverses, since the data from the
   *  arbitrary axis gives you info on how to get to it, not how
   *  to get away from it, and an inverse must be applied.
   *
   *  The basic calculation used is to recognize that the arbitrary
   *  axis vector (x, y, z), since it is of unit length, actually
   *  represents the sines and cosines of the angles to rotate the
   *  X-axis to the same orientation, with theta being the angle about
   *  Z and phi the angle about Y (in the order described above)
   *  as follows:
   *
   *  cos ( theta ) = x / sqrt ( 1 - z^2 )
   *  sin ( theta ) = y / sqrt ( 1 - z^2 )
   *
   *  cos ( phi ) = sqrt ( 1 - z^2 )
   *  sin ( phi ) = z
   *
   *  Note that cos ( phi ) can further be inserted to the above
   *  formulas:
   *
   *  cos ( theta ) = x / cos ( phi )
   *  sin ( theta ) = y / sin ( phi )
   *
   *  ...etc.  Because of those relations and the standard trigonometric
   *  relations, it is pssible to reduce the transforms down to what
   *  is used below.  It may be that any primary axis chosen will give the
   *  same results (modulo a sign convention) using thie method.
   *
   *  Particularly nice is to notice that all divisions that might
   *  have caused trouble when parallel to certain planes or
   *  axis go away with care paid to reducing the expressions.
   *  After checking, it does perform correctly under all cases, since
   *  in all the cases of division where the denominator would have
   *  been zero, the numerator would have been zero as well, giving
   *  the expected result.
   */

  xx = x * x;
  yy = y * y;
  zz = z * z;
  xy = x * y;
  yz = y * z;
  zx = z * x;
  xs = x * s;
  ys = y * s;
  zs = z * s;
  one_c = 1.0F - c;

  M(0,0) = (one_c * xx) + c;
  M(0,1) = (one_c * xy) - zs;
  M(0,2) = (one_c * zx) + ys;
  M(0,3) = 0.0F;

  M(1,0) = (one_c * xy) + zs;
  M(1,1) = (one_c * yy) + c;
  M(1,2) = (one_c * yz) - xs;
  M(1,3) = 0.0F;

  M(2,0) = (one_c * zx) - ys;
  M(2,1) = (one_c * yz) + xs;
  M(2,2) = (one_c * zz) + c;
  M(2,3) = 0.0F;

  M(3,0) = 0.0F;
  M(3,1) = 0.0F;
  M(3,2) = 0.0F;
  M(3,3) = 1.0F;

#undef M
}

void
make_translation_matrix (float x, float y, float z, float *m)
{
  make_identity_matrix (m);
  m[12] = x;
  m[13] = y;
  m[14] = z;
}

void
make_scaling_matrix (float x, float y, float z, float *m)
{
  make_identity_matrix (m);
  m[0] = x;
  m[5] = y;
  m[10] = z;
}

/* The next two functions are from the matrix FAQ by <hexapod@netcom.com>, see:
 * http://www.flipcode.com/documents/matrfaq.html
 */

/* Quickly convert 3 Euler angles to a rotation matrix. */
void
matrix_euler_to_rotation (float angle_x, float angle_y, float angle_z,
						  float *mat)
{
  float A		= cos(angle_x);
  float B		= sin(angle_x);
  float C		= cos(angle_y);
  float D		= sin(angle_y);
  float E		= cos(angle_z);
  float F		= sin(angle_z);

  float AD		=	A * D;
  float BD		=	B * D;

  mat[0]  =	  C * E;
  mat[4]  =	 -C * F;
  mat[8]  =	 -D;
  mat[1]  = -BD * E + A * F;
  mat[5]  =	 BD * F + A * E;
  mat[9]  =	 -B * C;
  mat[2]  =	 AD * E + B * F;
  mat[6]  = -AD * F + B * E;
  mat[10] =	  A * C;

  mat[12]  =  mat[13] = mat[14] = mat[3] = mat[7] = mat[11] = 0;
  mat[15] =	 1;
}

static inline float
clamp (float v, float low, float high)
{
  /* This is not very efficient ... */
  v -= low;
  while (v > high - low) v -= high - low;
  while (v < 0) v += high - low;
  v += low;
  return v;
}

/* Convert a rotation matrix to 3 Euler angles. */
void
matrix_rotation_to_euler (const float *mat,
						  float *angle_x, float *angle_y, float *angle_z)
{
  float C, trx, try;

  *angle_y = -asin( mat[8]);		/* Calculate Y-axis angle */
  C			  =	 cos( *angle_y );

  if ( fabs( C ) > 0.005 )			   /* Gimball lock? */
	{
	  trx	   =  mat[10] / C;			 /* No, so get X-axis angle */
	  try	   = -mat[9]  / C;

	  *angle_x	= atan2( try, trx );

	  trx	   =  mat[0] / C;			 /* Get Z-axis angle */
	  try	   = -mat[4] / C;

	  *angle_z	= atan2( try, trx );
	}
  else								   /* Gimball lock has occurred */
	{
	  trx	   = mat[5];				 /* And calculate Z-axis angle */
	  try	   = mat[1];

	  *angle_z	= atan2( try, trx );
	}

  /* Clamp all angles to range */
  *angle_x = clamp (*angle_x, 0, 2 * M_PI);
  *angle_y = clamp (*angle_y, 0, 2 * M_PI);
  *angle_z = clamp (*angle_z, 0, 2 * M_PI);
}

/* This code is taken from Mesa 3.0.
 */
void
matrix_multiply (const float *a, const float *b,
				 float *product)
{
   /* This matmul was contributed by Thomas Malik */
  int i;

#define A(row,col)	a[(col<<2)+row]
#define B(row,col)	b[(col<<2)+row]
#define P(row,col)	product[(col<<2)+row]

   /* i-te Zeile */
   for (i = 0; i < 4; i++) {
	 float ai0=A(i,0),	ai1=A(i,1),	 ai2=A(i,2),  ai3=A(i,3);
	 P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
	 P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
	 P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
	 P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
   }

#undef A
#undef B
#undef P
}

/* Multiply matrix by vector. */
void
matrix_vec_multiply (const float *m, const float *v,
					 float *result)
{
  result[0] = m[0]*v[0] + m[1]*v[1] + m[2]*v[2] + m[3]*v[3];
  result[1] = m[4]*v[0] + m[5]*v[1] + m[6]*v[2] + m[7]*v[3];
  result[2] = m[8]*v[0] + m[9]*v[1] + m[10]*v[2] + m[11]*v[3];
  result[3] = m[12]*v[0] + m[13]*v[1] + m[14]*v[2] + m[15]*v[3];
}

/* Compute the magnitude of a vector. */
float
vec_magnitude (const float *v)
{
  return sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

/* Compute the magnitude of a 2D vector. */
float
vec_magnitude2d (const float *v)
{
  return sqrt (v[0]*v[0] + v[1]*v[1]);
}

/* Normalize a vector. */
void
vec_normalize (const float *v, float *r)
{
  float w = vec_magnitude (v);
  r[0] = v[0] / w;
  r[1] = v[1] / w;
  r[2] = v[2] / w;
}

/* Normalize a 2D vector. */
void
vec_normalize2d (const float *v, float *r)
{
  float w = vec_magnitude2d (v);
  r[0] = v[0] / w;
  r[1] = v[1] / w;
}

void
vec_unit_normal_to_side (float *side, float *normal)
{
  float n[3] = { side[0], side[1], side[2] };
  vec_normalize (n, normal);
}

/* Compute the dot product of two vectors. */
float
vec_dot_product (const float *v1, const float *v2)
{
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

/* Compute the magnitude of vector v1 in direction vector v2.
 * If v1 == v2, this returns 1. If v1 = -v2, this returns -1.
 * If v1 is perpendicular to v2 this returns 0.
 */
float
vec_magnitude_in_direction (const float *v1, const float *v2)
{
  static float v1n[3], v2n[3];

  /* Normalize both vectors. */
  vec_normalize (v1, v1n);
  vec_normalize (v2, v2n);

  /* Return their dot product. */
  return vec_dot_product (v1n, v2n);
}

/* Compute angle between two vectors. */
float
vec_angle_between (const float *v1, const float *v2)
{
  return acos (vec_magnitude_in_direction (v1, v2));
}

/* Scale a vector. */
void
vec_scale (const float *a, float n, float *r)
{
  r[0] = a[0] * n;
  r[1] = a[1] * n;
  r[2] = a[2] * n;
}

/* Add two vectors. */
void
vec_add (const float *a, const float *b, float *r)
{
  r[0] = a[0] + b[0];
  r[1] = a[1] + b[1];
  r[2] = a[2] + b[2];
}

/* Subtract two vectors. */
void
vec_subtract (const float *a, const float *b, float *r)
{
  r[0] = a[0] - b[0];
  r[1] = a[1] - b[1];
  r[2] = a[2] - b[2];
}

/* Calculate midpoint. */
void
point_midpoint (const float *p1, const float *p2, float *mp)
{
  mp[0] = (p1[0] + p2[0]) / 2;
  mp[1] = (p1[1] + p2[1]) / 2;
  mp[2] = (p1[2] + p2[2]) / 2;
}

/* Calculate midpoint (in 2D). */
void
point_midpoint2d (const float *p1, const float *p2, float *mp)
{
  mp[0] = (p1[0] + p2[0]) / 2;
  mp[1] = (p1[1] + p2[1]) / 2;
}

/* Distance between two points. */
float
point_distance_to_point (const float *p1, const float *p2)
{
  float v[3];

  vec_subtract (p1, p2, v);
  return vec_magnitude (v);
}

/* Cross product of vectors v and w.
 * The cross product is a vector:
 *
 *	 v x w = |v| |w| sin t n^
 *
 * where t is the angle between a and
 * b, and n^ is a normal vector perpendicular
 * to a and b such that a,b,n^ form a
 * right-handed set.
 */
void
vec_cross_product (const float *v, const float *w, float *r)
{
  r[0] = v[1]*w[2] - v[2]*w[1];
  r[1] = v[2]*w[0] - v[0]*w[2];
  r[2] = v[0]*w[1] - v[1]*w[0];
}

/* Distance between two points. */
float
point_distance (const float *p, const float *q)
{
  double x = p[0] - q[0];
  double y = p[1] - q[1];
  double z = p[2] - q[2];
  return sqrt (x*x + y*y + z*z);
}

/* Distance from a point to a plane. */
float
point_distance_to_plane (const float *plane, const float *point)
{
  float a = plane[0];
  float b = plane[1];
  float c = plane[2];
  float d = plane[3];
  float x = point[0];
  float y = point[1];
  float z = point[2];
  float t = (a*x + b*y + c*z + d) / - (a*a + b*b + c*c);
  float t2 = t*t;
  float dist = sqrt (t2*a*a + t2*b*b + t2*c*c);
  /* Don't lose the sign of t. */
  if (t < 0) return dist; else return -dist;
}

/* Return true if point_distance_to_plane > 0. */
int
point_is_inside_plane (const float *plane, const float *point)
{
  float a = plane[0];
  float b = plane[1];
  float c = plane[2];
  float d = plane[3];
  float x = point[0];
  float y = point[1];
  float z = point[2];
  float t = (a*x + b*y + c*z + d) / - (a*a + b*b + c*c);

  return t < 0;

#if 0
  float t2 = t*t;
  float dist = sqrt (t2*a*a + t2*b*b + t2*c*c);
  /* Don't lose the sign of t. */
  if (t < 0) return dist; else return -dist;
#endif
}

/* See: http://www.greuer.de/efaq.html */
void
point_footprint_on_line (const float *point,
						 const float *line_point, const float *line_vector,
						 float *footprint)
{
  float t;
  float s[3];

  vec_subtract (point, line_point, s);
  t = vec_dot_product (s, line_vector) /
	  vec_dot_product (line_vector, line_vector);

  vec_scale (line_vector, t, s);
  vec_add (line_point, s, footprint);
}

float
point_distance_to_line (const float *point,
						const float *line_point, const float *line_vector)
{
#if 0
  float footprint[3];

  point_footprint_on_line (point, line_point, line_vector, footprint);
  return point_distance_to_point (point, footprint);
#else
  float u[3], p[3], prod[3], dist;

  /* Normalize vector u along the line. */
  vec_normalize (line_vector, u);

  /* The distance is given by | (p-a) x u | where p is the
   * point, a is a point on the line and x is the cross product.
   */
  vec_subtract (point, line_point, p);
  vec_cross_product (p, u, prod);
  dist = vec_magnitude (prod);

  return dist;
#endif
}

float
point_distance_to_line_segment (const float *point,
								const float *line_point0,
								const float *line_point1)
{
  float t;
  float s[3], v[3];

  vec_subtract (line_point1, line_point0, v);
  vec_subtract (point, line_point0, s);
  t = vec_dot_product (s, v) /
	  vec_dot_product (v, v);

  if (t >= 0 && t <= 1)
	{
	  float footprint[3];

	  vec_scale (v, t, s);
	  vec_add (line_point0, s, footprint);
	  return point_distance_to_point (point, footprint);
	}
  else if (t < 0)
	return point_distance_to_point (point, line_point0);
  /* else t > 1 */
  return point_distance_to_point (point, line_point1);
}

int
point_lies_in_face (const float *points, int nr_points, const float *point)
{
  float sum = point_face_angle_sum (points, nr_points, point);
  return fabs (sum - 2*M_PI) < 1e-5;
}

/* See: http://astronomy.swin.edu.au/pbourke/geometry/insidepoly/ */
float
point_face_angle_sum (const float *points, int nr_points, const float *point)
{
  float sum = 0;
  int i, next;

  for (i = 0, next = 1; i < nr_points; ++i, ++next)
	{
	  float p1[3], p2[3], m1, m2, costheta;

	  if (next == nr_points) next = 0;

	  p1[0] = points[i*3] - point[0];
	  p1[1] = points[i*3+1] - point[1];
	  p1[2] = points[i*3+2] - point[2];
	  p2[0] = points[next*3] - point[0];
	  p2[1] = points[next*3+1] - point[1];
	  p2[2] = points[next*3+2] - point[2];

	  m1 = vec_magnitude (p1);
	  m2 = vec_magnitude (p2);

	  if (m1 * m2 < 1e-5)
		return 2 * M_PI;
	  else
		costheta = (p1[0]*p2[0] + p1[1]*p2[1] + p1[2]*p2[2]) / (m1*m2);

	  sum += acos (costheta);
	}

  return sum;
}

float
point_distance_to_face (const float *points, int nr_points,
						const float *plane, const float *point, int *edge)
{
  float my_plane_coeffs[4];
  float a, b, c, d, tq, q[3], dist;
  int e, i, next;

  /* Calculate plane coefficients, if necessary. */
  if (plane == 0)
	{
	  plane = my_plane_coeffs;
	  plane_coefficients (&points[0], &points[3], &points[6], (float *) plane);
	}

  a = plane[0];
  b = plane[1];
  c = plane[2];
  d = plane[3];

  /* q is the coordinate of the point of intersection of a
   * normal line from the point to the plane. It may or may
   * not be on the bounded face (we'll work that out in the
   * moment). tq is the parameter of point q.
   */
  tq = - (a*point[0] + b*point[1] + c*point[2] + d) / (a*a + b*b + c*c);
  q[0] = point[0] + tq*a;
  q[1] = point[1] + tq*b;
  q[2] = point[2] + tq*c;

  /* Is q on the bounded face? */
  if (point_lies_in_face (points, nr_points, q))
	{
	  /* Compute the distance from the point to the plane. */
	  float t2 = tq*tq;

	  dist = sqrt (t2*a*a + t2*b*b + t2*c*c);

	  if (edge) *edge = -1;

	  return tq < 0 ? dist : -dist;
	}

  /* Find the closest edge. */
  e = -1;
  dist = 0;

  for (i = 0, next = 1; i < nr_points; ++i, ++next)
	{
	  float d;

	  if (next == nr_points) next = 0;

	  d = point_distance_to_line_segment (point,
										  &points[i*3], &points[next*3]);

	  if (e == -1 || d < dist)
		{
		  dist = d;
		  e = i;
		}
	}

  if (edge) *edge = e;

  return tq < 0 ? dist : -dist;
}

/* Compute the four coefficients of a plane which
 * uniquely specify that plane, given three points
 * (not colinear) on the plane. Most of the variables
 * in this function are redundant (optimized out?),
 * but they get it into the same form as in
 * Lansdown, p. 178.
 */
void
plane_coefficients (const float *p, const float *q, const float *r,
					float *co)
{
  float x2 = p[0];
  float y2 = p[1];
  float z2 = p[2];
  float x1 = q[0];
  float y1 = q[1];
  float z1 = q[2];
  float x3 = r[0];
  float y3 = r[1];
  float z3 = r[2];
  float xa = x1 + x2;
  float xb = x2 + x3;
  float xc = x3 + x1;
  float ya = y1 + y2;
  float yb = y2 + y3;
  float yc = y3 + y1;
  float za = z1 + z2;
  float zb = z2 + z3;
  float zc = z3 + z1;

  co[0] = (y1-y2) * za + (y2-y3) * zb + (y3-y1) * zc;
  co[1] = (z1-z2) * xa + (z2-z3) * xb + (z3-z1) * xc;
  co[2] = (x1-x2) * ya + (x2-x3) * yb + (x3-x1) * yc;
  co[3] = - (co[0]*x1 + co[1]*y1 + co[2]*z1);
}

void
plane_translate_along_normal (const float *plane, float distance,
							  float *new_plane)
{
  float w = vec_magnitude (plane);

  new_plane[0] = plane[0];
  new_plane[1] = plane[1];
  new_plane[2] = plane[2];
  new_plane[3] = plane[3] - w * distance;
}

void
face_translate_along_normal (const float *points, int nr_points,
							 const float *plane, float distance,
							 float *new_points, float *new_plane)
{
  float w = vec_magnitude (plane), nv[3];
  int i;

  new_plane[0] = plane[0];
  new_plane[1] = plane[1];
  new_plane[2] = plane[2];
  new_plane[3] = plane[3] - w * distance;

  vec_scale (plane, distance / w, nv);
  for (i = 0; i < nr_points; ++i)
	{
	  new_points[i*3] = points[i*3] + nv[0];
	  new_points[i*3+1] = points[i*3+1] + nv[1];
	  new_points[i*3+2] = points[i*3+2] + nv[2];
	}
}

/* All these quaternion functions are modified from the matrix FAQ again. */
void
quaternion_normalize (const float *a, float *r)
{
  float w = vec_magnitude (a);
  r[0] = a[0] / w;
  r[1] = a[1] / w;
  r[2] = a[2] / w;
  r[3] = a[3] / w;
}

void
quaternion_conjugate (const float *a, float *r)
{
  r[0] = -a[0];
  r[1] = -a[1];
  r[2] = -a[2];
  r[3] = a[3];
}

float
quaternion_magnitude (const float *a)
{
  return sqrt (a[3]*a[3] + a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

void
quaternion_multiply (const float *a, const float *b, float *r)
{
  float va[3], vb[3], vc[3];

  r[3] = vec_dot_product (a, b);

  vec_cross_product (a, b, va);
  vec_scale (a, b[3], vb);
  vec_scale (b, a[3], vc);
  vec_add (va, vb, va);
  vec_add (va, vc, r);

  quaternion_normalize (r, r);
}

void
quaternion_to_rotation_matrix (const float *q, float *mat)
{
  float X = q[0];
  float Y = q[1];
  float Z = q[2];
  float W = q[3];

  float xx		= X * X;
  float xy		= X * Y;
  float xz		= X * Z;
  float xw		= X * W;

  float yy		= Y * Y;
  float yz		= Y * Z;
  float yw		= Y * W;

  float zz		= Z * Z;
  float zw		= Z * W;

  mat[0]  = 1 - 2 * ( yy + zz );
  mat[4]  =		2 * ( xy - zw );
  mat[8]  =		2 * ( xz + yw );

  mat[1]  =		2 * ( xy + zw );
  mat[5]  = 1 - 2 * ( xx + zz );
  mat[9]  =		2 * ( yz - xw );

  mat[2]  =		2 * ( xz - yw );
  mat[6]  =		2 * ( yz + xw );
  mat[10] = 1 - 2 * ( xx + yy );

  mat[3]  = mat[7] = mat[11] = mat[12] = mat[13] = mat[14] = 0;
  mat[15] = 1;
}

void
make_quaternion_from_axis_angle (const float *axis, float angle,
								 float *q)
{
  double sin_a = sin (angle / 2);
  double cos_a = cos (angle / 2);

  q[0] = axis[0] * sin_a;
  q[1] = axis[1] * sin_a;
  q[2] = axis[2] * sin_a;
  q[3] = cos_a;

  quaternion_normalize (q, q);
}

int
collision_moving_sphere_and_face (const float *p0, const float *p1,
								  float radius,
								  const float *points, int nr_points,
								  const float *plane,
								  float *collision_point)
{
  float my_plane_coeffs[4], raised_plane[4], t, v[3], quot;
  float raised_points[3 * nr_points];

  /* Get the plane coefficients. */
  if (plane == 0)
	{
	  plane = my_plane_coeffs;
	  plane_coefficients (&points[0], &points[3], &points[6], (float *) plane);
	}

  /* Raise the plane up by the distance of one radius. Then we can
   * just test for the intersection of the ray and the face.This is
   * something of an approximation, but hopefully will give us good
   * results in practice. If not, then we may need to rework this
   * code. (XXX)
   */
  face_translate_along_normal (points, nr_points, plane, radius,
							   raised_points, raised_plane);

  /* Get the intersection point of the ray and the plane, as a
   * parameter, t.
   */
  vec_subtract (p1, p0, v);
  quot = raised_plane[0]*v[0] + raised_plane[1]*v[1] + raised_plane[2]*v[2];
  if (fabs (quot)
#if 0
	  < 1e-5
#else
	  == 0
#endif
	  )
	{
	  /* The path of the sphere is nearly parallel to the plane. Don't
	   * count this as a collision at all.
	   */
	  return 0;
	}

  t = - (raised_plane[0]*p0[0] + raised_plane[1]*p0[1] + raised_plane[2]*p0[2]
		 + raised_plane[3]) / quot;
  if (t < 0 || t > 1)
	{
	  /* There is no collision. */
	  return 0;
	}

  /* Calculate the actual point of collision. NOTE: This is the centre
   * of the sphere, NOT the point where the sphere and plane touch.
   */
  vec_scale (v, t, v);
  vec_add (p0, v, collision_point);

  /* Is the collision point actually within the bounded convex polygon
   * which defines the face? If not, then no collision actually
   * occurred. Note that we have to translate (ie. raise) the points
   * list too.
   */
  return point_lies_in_face (raised_points, nr_points, collision_point);
}
