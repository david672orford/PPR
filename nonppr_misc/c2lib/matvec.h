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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#ifndef MATVEC_H
#define MATVEC_H

#include <pool.h>
#include <math.h>

/* Notes:
 *
 * This library only handles 4x4 matrices and 4-vectors, for
 * basic 3D graphics use. A matrix is just an array float[16]
 * and a vector is just an array float[4]. You can either allocate
 * these in a pool using ``new_matrix'' and ``new_vec'', or
 * else you can allocate them statically.
 *
 * All matrices are stored in COLUMN-MAJOR ORDER! This is for
 * compatibility with OpenGL, but it is the OPPOSITE of the
 * normal C row-major ordering, so beware. Matrix elements in
 * column-major order are as follows:
 *
 *   M = | m0  m4  m8  m12 |
 *       | m1  m5  m9  m13 |
 *       | m2  m6  m10 m14 |
 *       | m3  m7  m11 m15 |
 *
 * Some of these functions have been inlined. I only inline
 * functions based on evidence from profiling the code as a
 * whole or where the function is so simple that the overhead
 * of calling it is larger than the inlined code. Excessive
 * inlining can itself cause performance problems, particularly
 * on modern processors which are very good at making jumps
 * and function calls.
 *
 */

/* Function: new_matrix - allocate a new matrix or vector
 * Function: new_vec
 *
 * @code{new_matrix} allocates a new 4x4 matrix of floats
 * in @code{pool}.
 *
 * @code{new_vec} allocates a new 4-vector of floats in
 * @code{pool}.
 *
 * You may use these functions to allocate matrices and
 * vectors dynamically, or you may allocate them statically.
 * The other matrix and vector functions available do not
 * distriguish between dynamically and statically allocated
 * variables.
 *
 * Note: All matrices are stored in COLUMN-MAJOR ORDER!
 * This is for compatibility with OpenGL, but it is the
 * OPPOSITE of the natural C row-major ordering, so beware.
 *
 * See also: @ref{new_identity_matrix(3)}, @ref{new_zero_vec(3)}.
 */
#define new_matrix(pool) ((float *) pmalloc ((pool), sizeof (float) * 16))
#define new_vec(pool) ((float *) pmalloc ((pool), sizeof (float) * 4))

/* Variable: identity_matrix - identity matrix and zero vector
 * Variable: zero_vec
 * Function: new_identity_matrix
 * Function: new_zero_vec
 * Function: make_identity_matrix
 * Function: make_zero_vec
 *
 * The @code{identity_matrix} variable contains a read-only
 * copy of the identity matrix. The @code{zero_vec} variable
 * contains a read-only copy of the zero vector.
 *
 * Use @code{new_identity_matrix} to allocate a new
 * identity matrix variable in @code{pool}. Use @code{new_zero_vec}
 * to similarly allocate a new zero vector.
 *
 * Use @code{make_identity_matrix} to copy the identity
 * matrix over an existing matrix @code{m}. Use @code{make_zero_vec}
 * to similarly copy the zero vector over an existing vector @code{v}.
 *
 * See also: @ref{new_matrix(3)}, @ref{new_vec(3)}.
 */
extern float identity_matrix[16];
extern float zero_vec[4];
extern float *new_identity_matrix (pool);
extern float *new_zero_vec (pool);
#define make_identity_matrix(m) memcpy (m, identity_matrix, sizeof(float)*16);
#define make_zero_vec(v) memcpy (v, zero_vec, sizeof (float) * 4);

extern void make_rotation_matrix (float angle,
				  float x, float y, float z,
				  float *m);

extern void make_translation_matrix (float x, float y, float z, float *m);

extern void make_scaling_matrix (float x, float y, float z, float *m);

extern void matrix_euler_to_rotation (float angle_x, float angle_y,
				      float angle_z,
				      float *mat);
extern void matrix_rotation_to_euler (const float *mat,
				      float *angle_x, float *angle_y,
				      float *angle_z);

extern void matrix_multiply (const float *a, const float *b,
			     float *product);

extern void matrix_vec_multiply (const float *m, const float *v,
				 float *result);

/* Function: vec_magnitude - calculate magnitude (length) of a vector
 * Function: vec_magnitude2d
 *
 * @code{vec_magnitude} calculates the magnitude (length) of a
 * 3D vector. @code{vec_magnitude2d} calculates the magnitude
 * of a 2D vector.
 *
 * See also: @ref{vec_normalize(3)}, @ref{vec_normalize2d(3)}.
 */
extern float vec_magnitude (const float *v);
extern float vec_magnitude2d (const float *v);

/* Function: vec_normalize - normalize a vector
 * Function: vec_normalize2d
 *
 * These two functions normalize respectively a 3D or 2D
 * vector @code{v}. The original vector @code{v} is not touched,
 * and the result is placed in vector @code{r}.
 *
 * To normalize a vector in-place (ie. modifying the original
 * vector), do:
 *
 * @code{vec_normalize (v, v);}
 *
 * See also: @ref{vec_magnitude(3)}, @ref{vec_magnitude2d(3)}.
 */
extern void vec_normalize (const float *v, float *r);
extern void vec_normalize2d (const float *v, float *r);

extern void vec_unit_normal_to_side (float *side, float *normal);

/* Function: vec_dot_product - calculate the dot product of two vectors
 *
 * @code{vec_dot_product} calculates the dot product of two
 * vectors @code{v1} and @code{v2} and returns it. The dot
 * product is formally defined as:
 *
 * @code{|v1| |v2| cos theta}
 *
 * where @code{theta} is the angle between the two vectors.
 *
 * One interesting consequence of this is that if both @code{v1}
 * and @code{v2} are already normalized, then the dot product
 * is 1 if the vectors are parallel and running in the same
 * direction, and 0 if the vectors are
 * perpendicular.
 *
 * See also: @ref{vec_magnitude_in_direction(3)}, @ref{vec_angle_between(3)}
 */
extern float vec_dot_product (const float *v1, const float *v2);

/* Function: vec_magnitude_in_direction - calculate relative direction of two vectors
 *
 * If @code{v1} and @code{v2} are parallel and point in the
 * same direction, then @code{vec_magnitude_in_direction} returns +1.
 * If @code{v1} and @code{v2} are perpendicular, this returns
 * 0. If @code{v1} and @code{v2} are parallel and point in
 * opposite directions to each other, this returns -1.
 * For other vectors, this function returns the cosine of
 * the angle between the vectors.
 *
 * See also: @ref{vec_dot_product(3)}, @ref{vec_angle_between(3)}
 */
extern float vec_magnitude_in_direction (const float *v1, const float *v2);

/* Function: vec_angle_between - calculate the angle between two vectors
 *
 * This function returns the angle between two vectors
 * @code{v1} and @code{v2}.
 *
 * See also: @ref{vec_dot_product(3)}, @ref{vec_magnitude_in_direction(3)}
 */
extern float vec_angle_between (const float *v1, const float *v2);

extern void vec_cross_product (const float *v, const float *w, float *r);

extern void vec_scale (const float *a, float n, float *r);
extern void vec_add (const float *a, const float *b, float *r);
extern void vec_subtract (const float *a, const float *b, float *r);

float point_distance_to_point (const float *p1, const float *p2);

extern void point_midpoint (const float *p1, const float *p2, float *mp);
extern void point_midpoint2d (const float *p1, const float *p2, float *mp);
extern float point_distance (const float *p, const float *q);

/* Function: point_distance_to_plane - distance from point to plane
 * Function: point_is_inside_plane
 *
 * @code{point_distance_to_plane} calculates the (shortest) distance from
 * the point @code{point} to the plane @code{plane}. This distance is
 * positive if the point is "inside" the plane -- that is, if the
 * normal vector drawn from the plane points towards the point. It
 * is negative if the point is "outside" the plane. It is zero if the
 * point lies on the plane.
 *
 * @code{point_is_inside_plane} returns true if the point is strictly
 * inside the plane, and false if the point lies on the plane or is
 * outside. It is much faster to compute this than to use
 * @code{point_distance_to_plane} and take the sign of the result.
 *
 * See also: @ref{plane_coefficients(3)}, @ref{point_distance_to_face(3)}.
 */
extern float point_distance_to_plane (const float *plane, const float *point);
extern int point_is_inside_plane (const float *plane, const float *point);

extern void point_footprint_on_line (const float *point, const float *line_point, const float *line_vector, float *footprint);

/* Function: point_distance_to_line - shortest distance from a point to a line
 *
 * Given a @code{point} and a line, expressed as @code{line_point}
 * and parallel @code{line_vector}, compute the shortest distance
 * from the point to the line.
 *
 * See also: @ref{point_distance_to_plane(3)}, @ref{point_distance_to_face(3)},
 * @ref{point_distance_to_line_segment(3)}.
 */
extern float point_distance_to_line (const float *point, const float *line_point, const float *line_vector);

/* Function: point_distance_to_line_segment - shortest distance from a point to a line segment
 *
 * Given a @code{point} and a line segment from @code{line_point0} to
 * @code{line_point1}, compute the shortest distance from the
 * point to the line segment.
 *
 * See also: @ref{point_distance_to_line(3)}.
 */
extern float point_distance_to_line_segment (const float *point, const float *line_point0, const float *line_point1);

/* Function: point_lies_in_face - does a point lie on the interior of a bounded convex polygon
 * Function: point_face_angle_sum
 *
 * Take a bounded convex polygon (a "face") and a point. The function
 * @code{point_lies_in_face} returns true iff the point is both
 * (a) coplanar with the face, and
 * (b) lies inside the edges of the face.
 *
 * In order to do this, @code{point_lies_in_face} calls
 * @code{point_face_angle_sum} which works out the sum of
 * the interior angles. If conditions (a) and (b) are both
 * satisfied then the sum of the interior angles will be
 * very close to 2.PI.
 *
 * The face is expressed as a flat list of points (3-vectors).
 *
 * See also: @ref{plane_coefficients(3)}, @ref{point_distance_to_face(3)}.
 */
extern int point_lies_in_face (const float *points, int nr_points, const float *point);
extern float point_face_angle_sum (const float *points, int nr_points, const float *point);

/* Function: point_distance_to_face - distance from point to bounded convex polygon (face)
 *
 * Given a point and a bounded convex polygon (a "face"), the
 * function @code{point_distance_to_face} calculates the distance
 * from the point to the face. There are two importance cases to
 * consider here:
 *
 * (a) The point is directly above or below the face. In other words,
 * a line dropped from the point perpendicular to the face intersects
 * the face within the boundary of the polygon. In this case, the
 * function returns the shortest distance from the point to the
 * intersection (and is essentially equivalent to
 * @code{point_distance_to_plane}).
 *
 * (b) The point is not directly above or below the face. In this case
 * the function works out the distance to the nearest edge of the face.
 *
 * The face is specified as a list of points and a plane (ie.
 * plane coefficients). If @code{plane} is @code{NULL}, then the
 * function calls @ref{plane_coefficients(3)} on your behalf. If
 * the face is fixed, and you will call this function lots of
 * times, then it is a good idea to calculate the plane coefficients
 * once only and cache them.
 *
 * Returns: The distance of the point from the face. The distance
 * will be positive if the point is above the face (ie. inside
 * the plane: see @ref{point_distance_to_plane(3)}), or negative
 * otherwise.
 *
 * If @code{edge} is not @code{NULL}, then it is set to one of
 * the following values:
 *
 * @code{*edge == -1} if the point is directly above or below
 * the face, corresponding to case (a) above.
 *
 * @code{*edge == 0 .. nr_points-1} if the point is closest to
 * that particular edge, corresponding to case (b) above.
 *
 * See also: @ref{point_distance_to_plane(3)},
 * @ref{plane_coefficients(3)}, @ref{point_lies_in_face(3)},
 * @ref{point_distance_to_line(3)}.
 */
extern float point_distance_to_face (const float *points, int nr_points, const float *plane, const float *point, int *edge);

/* Function: plane_coefficients - calculate the coefficient form for a plane
 *
 * Given three points, not colinear, which naturally form a plane, calculate
 * the 4-vector form for the plane coefficients. The three points
 * are passed as @code{p}, @code{q} and @code{r}. The coefficients
 * are returned in vector @code{co}.
 *
 * The four coefficients returned are respectively @code{a}, @code{b},
 * @code{c} and @code{d} in the standard plane equation:
 *
 * @code{a x + b y + c z + d = 0}
 *
 * (Note that many texts use @code{- d}, so be warned).
 *
 * The normal (perpendicular) vector to the plane may be derived
 * immediately: it is just @code{(a, b, c)}. Note that the normal
 * vector is not normalized!
 *
 * Planes are unbounded: they stretch off to infinity in all directions.
 * If what you really want are bounded convex polygons, then you
 * need to use a c2lib "face".
 */
extern void plane_coefficients (const float *p, const float *q, const float *r, float *co);

/* Function: plane_translate_along_normal - translate a plane or face some distance in the direction of the normal
 * Function: face_translate_along_normal
 *
 * Given an existing @code{plane} (expressed as plane coefficients),
 * produce a new plane @code{new_plane} which has been translated
 * by @code{distance} units along the direction of the normal. The
 * new plane is also returned as plane coefficients.
 *
 * @code{face_translate_along_normal} is similar, except that it also
 * translates a list of points by the same distance.
 *
 * See also: @ref{plane_coefficients(3)}.
 */
extern void plane_translate_along_normal (const float *plane, float distance, float *new_plane);
extern void face_translate_along_normal (const float *points, int nr_points, const float *plane, float distance, float *new_points, float *new_plane);

extern void quaternion_normalize (const float *a, float *r);
extern void quaternion_conjugate (const float *a, float *r);
extern float quaternion_magnitude (const float *a);
extern void quaternion_multiply (const float *a, const float *b, float *r);
extern void quaternion_to_rotation_matrix (const float *q, float *mat);
extern void make_quaternion_from_axis_angle (const float *axis, float angle,
					     float *q);

/* Function: collision_moving_sphere_and_face - detect collision between a moving sphere and a fixed face
 *
 * This function detects collisions between a sphere which is moving
 * at constant speed along a linear path and a fixed bounded
 * convex polygon ("face").
 *
 * The centre of the sphere moves from point @code{p0} to point @code{p1}.
 * The sphere has radius @code{radius}.
 *
 * The face is described by the list of bounding points, and the plane
 * coefficients of the plane of the face (you may pass @code{plane}
 * as @code{NULL} in which case the function works out the plane
 * coefficients for you, although this is generally less efficient).
 *
 * Returns: If there was a collision, this function returns true and
 * sets the collision point in @code{collision_point}. Note that the
 * collision point is the position of the centre of the sphere at
 * the point of collision, NOT the place where the sphere touches
 * the face. If there was no collision, this function returns false.
 */
extern int collision_moving_sphere_and_face (const float *p0, const float *p1, float radius, const float *points, int nr_points, const float *plane, float *collision_point);

#endif /* MATVEC_H */
