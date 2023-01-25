//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

//******************************************************************************
//
//   3D Vector Double Class
//
//   J.R. Gloudemans - 7/7/93
//******************************************************************************

//!  A class for representing 3D vectors.
/*!
  vec3d is typically used to describe coordinate points and vectors in 3D space.
  All 3 elements in the vector are of type double.
*/

#include "Defines.h"
#include <cfloat> //For DBL_EPSILON
#include "Vec3d.h"
#include "math.h"  // For a lot of usefull math functions
#include <climits> //For DBL_MAX if we decide to keep it
// #include "boost/multiprecision/cpp_bin_float.hpp"
/*! so squaring doesn't break limit.*/
const double doublelimitsquareroot = sqrt(DBL_MAX);

// Due to floating point errors and such, maybe using long double is advisable.
// It is compiler dependent, but some compiler/architecture combos will use 80-bit or 128-bit precision when available.
// If it is helpful, there are libraries that will do quadruple precision or octuple precision floating point math ex boost::multiprecision or MPFR or GMP
// ex boost::multiprecision::cpp_bin_float_oct octoupleprecisionfloat;
// boost::multiprecision::mpf_float and boost::multiprecision::mpfr_float are ~2x faster but require library additions that are worth considering.

// TODO scan for magic numbers and make them constants or #define them or something. Pick one and stick to it.
/*!Constants for demoing lack of magic numbers*/
const double close_enough1 = 1e-10;
/*!Constants for demoing lack of magic numbers*/
const double close_enough2 = 1.2e-11;

/*! Example of double divide handling NAN and INF so less error try/catch needs to happen in other functions*/

double safe_divide(const double numerator, const double denominator)
{
    double ret = numerator / denominator;
    if (std::isfinite(ret))
    {
        return ret;
    }
    switch (fpclassify(ret))
    {
    case FP_NAN:
        return 0.0; // TODO figure out what a nan should result in. It will likely be 0/0 so either 0 or 1.
        break;
    case FP_INFINITE:
        return std::copysign(doublelimitsquareroot, ret); // DBL_MIN is greater in magnitude than double max.
        break;
    }
}

/*! Example of fully bounding double divide handling NAN and INF so less error try/catch needs to happen in other functions*/

double bounded_divide(const double numerator, const double denominator, const double bound)
{
    double ret = safe_divide(numerator, denominator);
    ret = std::copysign(std::fmin(std::abs(ret), bound), ret);
    return ret;
}

using std::cout;
using std::endl;
using std::vector;

//****** Constructor:  vec3d x() ******//
/*!
    Initialize a vec3d object with X, Y, and Z set to 0.
  */
vec3d::vec3d()
{
    v[0] = v[1] = v[2] = 0.0;
}

//****** Constructor:  vec3d x(1.,0.,3.) ******//
/*!
    Initialize a vec3d object with input X, Y, and Z values.
  */
vec3d::vec3d(double xx, double yy, double zz)
{
    v[0] = xx;
    v[1] = yy;
    v[2] = zz;
}

//******* vec3d x = y ******//
/*!
    Initialize a vec3d with another vec3d. Equivalent to the "=" operator.
  */
vec3d::vec3d(const vec3d &a)
{
    v[0] = a.v[0];
    v[1] = a.v[1];
    v[2] = a.v[2];
}

/*!
    Initialize a vec3d with another type with three doubles from void x y and z functions? TODO
  */
vec3d::vec3d(const threed_point_type &a)
{
    v[0] = a.x();
    v[1] = a.y();
    v[2] = a.z();
}

/*!
    Initialize a vec3d with a length 3 double array.
  */
vec3d::vec3d(const double a[3])
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
}

/*!
    Initialize a vec3d with a length 3 float array.
  */
vec3d::vec3d(const float a[3])
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
}

/*!
    Initialize a vec3d with a std::vector using the first three indicies.
    \warning If the vector has fewer than 3 elements, it will read out of bounds unless the compliler flags it. TODO Consider at() instead.
  */
vec3d::vec3d(const std::vector<double> &a)
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
}

//****** Equals:  x = y ******
/*!
    Set a vec3d with another vec3d. Equivalent to the "=" operator.
  */
vec3d &vec3d::operator=(const vec3d &a)
{
    if (this == &a)
    {
        return *this;
    }

    v[0] = a.v[0];
    v[1] = a.v[1];
    v[2] = a.v[2];
    return *this;
}

/*!
    Set a vec3d with a vec2d. Pads the vector with a 0 for the z value. Equivalent to the "=" operator.
  */
vec3d &vec3d::operator=(const vec2d &a)
{
    v[0] = a.v[0];
    v[1] = a.v[1];
    v[2] = 0.0;
    return *this;
}

//******* x = 35. ******//
/*!
    Set a vec3d with a single double. Sets all values to the double.
  */
vec3d &vec3d::operator=(double a)
{
    v[0] = v[1] = v[2] = a;
    return *this;
}

/*!
    Set a vec3d with another type with a three doubles from void x y and z functions? TODO
  */
vec3d &vec3d::operator=(const threed_point_type &a)
{
    v[0] = a.x();
    v[1] = a.y();
    v[2] = a.z();
    return *this;
}

//******* Set Point Values *******//
/*!
    Set all three elements of the vec3d vector
    \code{.cpp}
    //==== Test Vec3d ====//
    vec3d a();                                // Default Constructor

    a.set_xyz( 2.0, 4.0, 6.0 );
    \endcode
    \param [in] x New X value
    \param [in] y New Y value
    \param [in] z New Z value
    \return Updated vec3d
  */
vec3d &vec3d::set_xyz(double xx, double yy, double zz)
{
    v[0] = xx;
    v[1] = yy;
    v[2] = zz;
    return *this;
}

/*!
    Set a vec3d with a std::vector.
    \warning If the vector has fewer than 3 elements, it will read out of bounds unless the compliler flags it. TODO Consider at() instead.
  */
vec3d &vec3d::set_vec(const std::vector<double> &a)
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
    return *this;
}

/*!
    Set a vec3d with a double array.
  */
vec3d &vec3d::set_arr(const double a[])
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
    return *this;
}

/*!
    Set a vec3d with a float array.
  */
vec3d &vec3d::set_arr(const float a[])
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
    return *this;
}

/*!
    Set x point value with double.
  */
vec3d &vec3d::set_x(double xx)
{
    v[0] = xx;
    return *this;
}

/*!
    Set y point value with double.
  */
vec3d &vec3d::set_y(double yy)
{
    v[1] = yy;
    return *this;
}

/*!
    Set z point value with double.
  */
vec3d &vec3d::set_z(double zz)
{
    v[2] = zz;
    return *this;
}

/*!
    Set vector with the reflection of the given vector about the x-axis.
  */
vec3d &vec3d::set_refx(const vec3d &a)
{
    v[0] = -a[0];
    v[1] = a[1];
    v[2] = a[2];
    return *this;
}

/*!
    Set vector with the reflection of the given vector about the y-axis.
  */
vec3d &vec3d::set_refy(const vec3d &a)
{
    v[0] = a[0];
    v[1] = -a[1];
    v[2] = a[2];
    return *this;
}

/*!
    Set vector with the reflection of the given vector about the z-axis.
  */
vec3d &vec3d::set_refz(const vec3d &a)
{
    v[0] = a[0];
    v[1] = a[1];
    v[2] = -a[2];
    return *this;
}

////******* Transform *******//
// vec3d vec3d::transform(float mat[4][4])
//{
//    return( vec3d( (mat[0][0]*v[0] + mat[1][0]*v[1] + mat[2][0]*v[2] + mat[3][0]),
//                   (mat[0][1]*v[0] + mat[1][1]*v[1] + mat[2][1]*v[2] + mat[3][1]),
//                   (mat[0][2]*v[0] + mat[1][2]*v[1] + mat[2][2]*v[2] + mat[3][2]) ) );
//
//}
//
////******* Transform *******//
// vec3d vec3d::transform(double mat[4][4])
//{
//    return( vec3d( (mat[0][0]*v[0] + mat[1][0]*v[1] + mat[2][0]*v[2] + mat[3][0]),
//                   (mat[0][1]*v[0] + mat[1][1]*v[1] + mat[2][1]*v[2] + mat[3][1]),
//                   (mat[0][2]*v[0] + mat[1][2]*v[1] + mat[2][2]*v[2] + mat[3][2]) ) );
//
//}

/*!
    Vector addition. X = A + B
  */
vec3d operator+(const vec3d &a, const vec3d &b)
{
    vec3d ret(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2]);
    return ret;
}

/*!
    Vector subtraction. X = A - B
  */
vec3d operator-(const vec3d &a, const vec3d &b)
{
    vec3d ret(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2]);
    return ret;
}

/*!
    Vector multiplication with scalar double. X = A * b
  */
vec3d operator*(const vec3d &a, double b)
{
    vec3d ret(a.v[0] * b, a.v[1] * b, a.v[2] * b);
    return ret;
}

/*!
    Scalar double multiplication with vector. X = a * B
  */
vec3d operator*(double b, const vec3d &a)
{
    vec3d ret(a.v[0] * b, a.v[1] * b, a.v[2] * b);
    return ret;
}

/*!
    Element-wise vector multiplication. (Hadamard product) X = A .* B
  */
vec3d operator*(const vec3d &a, const vec3d &b)
{
    vec3d ret(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2]);
    return ret;
}

/*!
    Vector division with scalar double. X = A / b where b is not within machine epsilon of zero
    \warning if b is negative and less than machine epsilon it will return the 0 vector //TODO
  */
vec3d operator/(const vec3d &a, double b)
{
    vec3d ret;
    // if (b != 0.0)
    if (!(b <= DBL_EPSILON && b >= 0.0))
    // TODO possibly add || b>=-DBL_EPSILON && b<=0.0 inside the ! statement so very small negative don't cause divide by 0 errors? Looks like they should blow up where positive values get set to the zero vector without looking at double formatting.
    {
        ret.set_xyz(a.v[0] / b, a.v[1] / b, a.v[2] / b);
    }
    else
    {
        ret.set_xyz(0.0, 0.0, 0.0);
    }

    return ret;
}

/*!
    Vector addition. A = A + B
  */
vec3d &vec3d::operator+=(const vec3d &b)
{
    v[0] += b.v[0];
    v[1] += b.v[1];
    v[2] += b.v[2];

    return *this;
}

/*!
    Vector subtraction. A = A - B
  */
vec3d &vec3d::operator-=(const vec3d &b)
{
    v[0] -= b.v[0];
    v[1] -= b.v[1];
    v[2] -= b.v[2];

    return *this;
}

/*!
    Vector multiplication by scalar double. A = A * b
  */
vec3d &vec3d::operator*=(double b)
{
    v[0] *= b;
    v[1] *= b;
    v[2] *= b;

    return *this;
}

/*!
    Vector addition via array of doubles. A = A + B
  */
vec3d &vec3d::operator+=(double b[])
{
    v[0] += b[0];
    v[1] += b[1];
    v[2] += b[2];

    return *this;
}

/*!
    Vector subtraction via array of doubles. A = A - B
  */
vec3d &vec3d::operator-=(double b[])
{
    v[0] -= b[0];
    v[1] -= b[1];
    v[2] -= b[2];

    return *this;
}

/*!
    Vector addition via array of floats. A = A + B
  */
vec3d &vec3d::operator+=(float b[])
{
    v[0] += b[0];
    v[1] += b[1];
    v[2] += b[2];

    return *this;
}

/*!
    Vector subtraction via array of floats. A = A - B
  */
vec3d &vec3d::operator-=(float b[])
{
    v[0] -= b[0];
    v[1] -= b[1];
    v[2] -= b[2];

    return *this;
}

/*!
    Checking if the elements of a vector are equal to all of the elements of another vector.
  */
bool operator==(const vec3d &a, const vec3d &b)
{
    if (a.v[0] == b.v[0] &&
        a.v[1] == b.v[1] &&
        a.v[2] == b.v[2])
    {
        return true;
    }
    return false;
}

/*!
    Check if it is not true that the elements of a vector are equal to all of the elements of another vector.
  */
bool operator!=(const vec3d &a, const vec3d &b)
{
    return !(a == b);
}

/*!
    Allow dangling negative sign in front of vectors to behave as left scalar multiplication by -1.0.
  */
vec3d operator-(const vec3d &in)
{
    return -1.0 * in;
}

//******* cout << a ******//
// ostream& operator<< (ostream& out, const vec3d& a)
//{
//    return ( out << "  " << a.v[0] << "  " <<
//       a.v[1] << "  " << a.v[2] << "  " ) ;
//}

/*!
  Compute distance between points. (Euclidean distance)
   */
double dist(const vec3d &a, const vec3d &b)
{

    double xx = a.v[0] - b.v[0];
    double yy = a.v[1] - b.v[1];
    double zz = a.v[2] - b.v[2];
    return (sqrt(xx * xx + yy * yy + zz * zz));
}

/*!
  Compute distance between points, but leave off taking the square root.
  This is faster for the case where you just need to compare a large set of distances with eachother, but doesn't return a relevant distance value.
   */
double dist_squared(const vec3d &a, const vec3d &b)
{
    double xx = a.v[0] - b.v[0];
    double yy = a.v[1] - b.v[1];
    double zz = a.v[2] - b.v[2];
    return (xx * xx + yy * yy + zz * zz);
}

/*!
  Computes the length of the given vector as a scalar double. (Euclidean distance) x = A.mag()
   */
double vec3d::mag() const
{
    return (sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

/*!
  Scales the length of a vector to a length of 1. (Euclidean distance)
  This vector is also called the vector normal, and the process of creating it is called normalizing or normalization.
  */
void vec3d::normalize()
{

    double length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= DBL_EPSILON)
    {
        v[0] = 0.0;
        v[1] = 0.0;
        v[2] = 0.0;
    }
    else
    {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

/*!
  Returns the index of the vector element that has the highest absolute value.
  */
int vec3d::major_comp() const
{
    int i = 0;
    double c = std::abs(v[i]);

    if (std::abs(v[1]) > c)
    {
        i = 1;
        c = std::abs(v[i]);
    }

    if (std::abs(v[2]) > c)
    {
        i = 2;
    }
    return i;
}

/*!
  Returns the index of the vector element that has the smallest absolute value.
  */
int vec3d::minor_comp() const
{
    int i = 0;
    double c = std::abs(v[i]);

    if (std::abs(v[1]) < c)
    {
        i = 1;
        c = std::abs(v[i]);
    }

    if (std::abs(v[2]) < c)
    {
        i = 2;
        c = std::abs(v[i]);
    }
    return i;
}

/*!
  Computes the Dot Product of two vectors and returns a scalar double. x = A.B or A * B or dot(A, B)
  This represents |A||B|Cos(theta) where theta is the angle between A and B.
  */
double dot(const vec3d &a, const vec3d &b)
{
    return (a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]);
}

/*!
  Computes the Cross Product of two vectors and returns the vector.  X = A x B or cross(A, B)
  The cross product of two vectors in 3d space is perpendicular (normal) to the input vectors.
  Note that A x B != B x A, cross products are not commutative, specifically they are anticommutative.
  */
vec3d cross(const vec3d &a, const vec3d &b)
{
    return vec3d(a.v[1] * b.v[2] - a.v[2] * b.v[1],
                 a.v[2] * b.v[0] - a.v[0] * b.v[2],
                 a.v[0] * b.v[1] - a.v[1] * b.v[0]);
}

/*!
  Computes the Angle Between Vectors
  TODO What are these cases? They look like they are results of unchecked division which should be done before computation.
  */
double angle(const vec3d &a, const vec3d &b)
{
    double angle = dot(a, b) / (a.mag() * b.mag());

    if (angle >= -1.0 && angle <= 1.0)
    {
        return (acos(angle));
    }
    else if (angle < 0.0)
    {
        return PI;
    }
    else
    {
        return (0.0);
    }
}

/*!
  Computes the Signed Angle Between Vectors with respect to a given reference vector.
  TODO, needs more explanation on why it is useful. Seems like it is an alternative to 2*PI modular arithmetic?
  */
double signed_angle(const vec3d &a, const vec3d &b, const vec3d &ref)
{
    double ang = angle(a, b);
    vec3d c = cross(a, b);

    double d = dot(c, ref);
    if (d < 0)
    {
        ang = -ang;
    }

    return ang;
}

// v1.Normalize();
// v2.Normalize();
// Vec3 c=v1.CrossProduct(v2);
// angle=std::atan2(c.Magnitude(),v1.Dot(v2));
// angle=c.Dot(Vec3(1.f,0.f,0.f)) < 0.f ? -angle : angle;
// std::cout<<"angle:"<<angle<<std::endl;
// if (angle>M_PI_2+0.0001f||angle<-M_PI_2-0.0001f)
//   return true;
//
//
//   double cosine = dot(a, b)/(a.mag()*b.mag());
//   if (cosine > 1) cosine = 1;
//   else if (cosine < -1) cosine = -1;
//
//   if ((data[0]*v[1] - data[1]*v[0]) < 0)
//     return -acos(cosine);
//   else
//     return acos(cosine);
// }

/*!
  Compute the Cosine of the Angle Between Vectors
  This is used in a lot of vector math applications rather than just the angle.
  This can also be helpful for detecting 0 crossings or when comparing angles as relative size and symmetry about 0 is preserved
  This computes faster than angle alone due to the vector math.
  */
double cos_angle(const vec3d &a, const vec3d &b)
{
    double angle = dot(a, b) / (a.mag() * b.mag());

    if (angle < -1.0)
    {
        return -1.0;
    }
    else if (angle > 1.0)
    {
        return 1.0;
    }

    return angle;
}

/*!
  Radius of Circle Passing Through 3 Points
  TODO Figure out why this is returning 1e6 for small positive values of denom
  If it is desired to cap values at 1.0e6, more checks are needed.
  */
double radius_of_circle(const vec3d &p1, const vec3d &p2, const vec3d &p3)
{
    vec3d a = p1 - p3;
    vec3d b = p2 - p3;

    double denom = 2.0 * cross(a, b).mag();

    // if (denom == 0.0)
    if (denom <= DBL_EPSILON)
    {
        return (1.0e06);
    }

    else
    {
        return ((a.mag() * b.mag() * (a - b).mag()) / denom);
    }
}

/*!
  Center And Radius of Circle Passing Through 3 Points
  TODO Figure out why this is returning 1e6 for small positive values of denom
  If it is desired to cap values at 1.0e6, more checks are needed.
  */
void center_of_circle(vec3d &p1, vec3d &p2, vec3d &p3, vec3d &center)
{
    vec3d a = p1 - p3;
    vec3d b = p2 - p3;

    double temp = cross(a, b).mag();
    double denom = 2.0 * temp * temp;

    // if (denom == 0.0)
    if (denom <= DBL_EPSILON)
    {
        center = p1 + vec3d(1.0e06, 1.0e06, 1.0e06);
    }

    else
    {

        double a_mag = a.mag();
        double a_sqr = a_mag * a_mag;
        double b_mag = b.mag();
        double b_sqr = b_mag * b_mag;
        double ab_dot = dot(a, b);
        center = a * (b_sqr * (a_sqr - ab_dot)) + b * (a_sqr * (b_sqr - ab_dot));
        center = center * (1.0 / denom) + p3;
    }
}

/*!
  Distance Between a Point And a Plane
  \warning NORM SHOULD BE NORMALIZED
  */
double dist_pnt_2_plane(vec3d &org, vec3d &norm, vec3d &pnt)
{
    double d = dot((pnt - org), norm);

    return (std::abs(d));
}

/*!
  Distance between a line defined by two points and a point.
  TODO division issue
    */
double dist_pnt_2_line(const vec3d &line_pt1, const vec3d &line_pt2, const vec3d &pnt)
{
    vec3d A_B = pnt - line_pt1;
    vec3d C_B = line_pt2 - line_pt1;

    double denom = C_B.mag();

    if (denom < DBL_EPSILON)
    {
        return (A_B.mag());
    }

    return (cross(A_B, C_B).mag() / denom);
}

/*!
  Distance Between Point And Line Segment
  */

/*
double dist_pnt_2_line_seg(vec3d& line_pt1, vec3d& line_pt2, vec3d& pnt)
{
  vec3d p_ln1 = pnt - line_pt1;
  vec3d ln2_ln1 = line_pt2 - line_pt1;

  if ( cos_angle( p_ln1, ln2_ln1 ) < 0.0 )
    return ( dist(pnt, line_pt1) );

  vec3d p_ln2 = pnt - line_pt2;
  vec3d ln1_ln2 = line_pt1 - line_pt2;

  if ( cos_angle( p_ln2, ln1_ln2 ) < 0.0 )
    return ( dist(pnt, line_pt2) );

  double denom = ln2_ln1.mag();

  if (denom < 0.0 )
    return( dist(pnt, line_pt1 ));

  return( cross(p_ln1, ln2_ln1).mag()/denom );
}
*/

/*!
  Project Point Onto Line Segment
  */
vec3d proj_pnt_on_line_seg(const vec3d &line_pt1, const vec3d &line_pt2, const vec3d &pnt)
// TODO division issue?
/*
{
    vec3d p_ln1 = pnt - line_pt1;

    if (p_ln1.mag() <= close_enough1)
    {
        return line_pt1;
    }

    vec3d ln2_ln1 = line_pt2 - line_pt1;

    if (cos_angle(p_ln1, ln2_ln1) <= 0.0)
    {
        return (line_pt1);
    }

    vec3d p_ln2 = pnt - line_pt2;

    if (p_ln2.mag() <= close_enough1)
    {
        return line_pt2;
    }

    vec3d ln1_ln2 = line_pt1 - line_pt2;

    if (cos_angle(p_ln2, ln1_ln2) <= 0.0)
    {
        return (line_pt2);
    }

    double denom = ln2_ln1.mag();

    if (denom <= 0.0)
    {
        return (line_pt1);
    }

    double numer = cos_angle(p_ln1, ln2_ln1) * p_ln1.mag();

    return (line_pt1 + ln2_ln1 * (numer / denom));
}
*/
// TODO division issue? proposed version: re-use ln2_ln1 in the subsequent calc by reversing operand p_ln2 to ln2_p as mag() and cos_angle() are commutative
// re-use values that are being calculated twice

{
    vec3d p_ln1 = pnt - line_pt1;
    double p1_ln1_mag = p_ln1.mag();

    if (p1_ln1_mag <= close_enough1)
    {
        return line_pt1;
    }

    vec3d ln2_p = line_pt2 - pnt;

    if (ln2_p.mag() <= close_enough1)
    {
        return line_pt2;
    }

    vec3d ln2_ln1 = line_pt2 - line_pt1;

    if (cos_angle(p_ln1, ln2_ln1) <= 0.0)
    {
        return (line_pt1);
    }

    double cosang = cos_angle(ln2_p, ln2_ln1);

    if (cosang <= 0.0)
    {
        return (line_pt2);
    }

    double denom = ln2_ln1.mag();

    if (denom <= close_enough1)
    {
        return (line_pt1);
    }

    double numer = cosang * p1_ln1_mag;

    return (line_pt1 + ln2_ln1 * (numer / denom));
}

/*!
  Project Point Onto Ray
  */
vec3d proj_pnt_on_ray(const vec3d &line_pt1, const vec3d &line_pt2, const vec3d &pnt)
/*
{
    // TODO Is this even doing what you want it to do? Just going to cut out unused stuff and not compute for very small divisors.
    vec3d p_ln1 = pnt - line_pt1;
    vec3d ln2_ln1 = line_pt2 - line_pt1;

    vec3d p_ln2 = pnt - line_pt2;
    vec3d ln1_ln2 = line_pt1 - line_pt2;

    double denom = ln2_ln1.mag();

    if (denom <= close_enough2)
    {
        return (line_pt1);
    }

    double numer = cos_angle(p_ln1, ln2_ln1) * p_ln1.mag();

    return (line_pt1 + ln2_ln1 * (numer / denom));
}
*/
{
    vec3d p_ln1 = pnt - line_pt1;
    vec3d ln2_ln1 = line_pt2 - line_pt1;

    double denom = ln2_ln1.mag();

    if (denom <= close_enough2)
    {
        return (line_pt1);
    }

    double numer = cos_angle(p_ln1, ln2_ln1) * p_ln1.mag();

    return (line_pt1 + ln2_ln1 * (numer / denom));
}

/*!
  Project Pnt Onto Line
  */
vec3d proj_pnt_on_line(const vec3d &line_pt1, const vec3d &line_pt2, const vec3d &pnt)
// TODO do optimizations
/*
{
    vec3d p_ln1 = pnt - line_pt1;
    vec3d ln2_ln1 = line_pt2 - line_pt1;

    vec3d p_ln2 = pnt - line_pt2;
    vec3d ln1_ln2 = line_pt1 - line_pt2;

    double denom = ln2_ln1.mag();

    if (std::abs(denom) <= close_enough2)
    {
        return (line_pt1);
    }

    double p_ln1_mag = p_ln1.mag();
    if (std::abs(p_ln1_mag) <= close_enough2)
    {
        return (line_pt1);
    }

    double numer = cos_angle(p_ln1, ln2_ln1) * p_ln1_mag;

    return (line_pt1 + ln2_ln1 * (numer / denom));
}
*/
{
    vec3d p_ln1 = pnt - line_pt1;
    double p1_ln1_mag = p_ln1.mag();

    if (p1_ln1_mag <= close_enough1)
    {
        return line_pt1;
    }

    vec3d ln2_p = line_pt2 - pnt;

    if (ln2_p.mag() <= close_enough1)
    {
        return line_pt2;
    }

    vec3d ln2_ln1 = line_pt2 - line_pt1;

    double denom = ln2_ln1.mag();

    if (denom <= close_enough1)
    {
        return (line_pt1);
    }

    double numer = cos_angle(p_ln1, ln2_ln1) * p1_ln1_mag;

    return (line_pt1 + ln2_ln1 * (numer / denom));
}

/*!
  Project Point To Plane
  */
vec3d proj_pnt_to_plane(vec3d &org, vec3d &plane_ln1, vec3d &plane_ln2, vec3d &pnt)
//======= NOT TESTED !!!!!!!!!!!!!!! ===//
// TODO Looks not the easiest way. Going to redo it. I am assuming org is a point on the plane as that seems to be a common thing.
/*
{
    vec3d normal = cross(plane_ln1, plane_ln2);

    vec3d proj_pnt = proj_pnt_on_ray(org, org + normal, pnt);

    vec3d proj_vec = org - proj_pnt;

    return (pnt + proj_vec);
}
*/
{
    // get unit normal of plane
    vec3d perpendicular = cross(plane_ln1, plane_ln2);
    vec3d normal = perpendicular / perpendicular.mag();
    // Create a vector from a point on the plane to the point we want to project onto the plane.
    // Use dot product being |A|*|B| * Cos(angle) and the normals magnitude being one to make |B| * Cos(angle).
    // This value is the signed distance from the plane, so we just run the distance along the normal vector, and subtract that from the point.
    vec3d proj_vec = normal * dot(normal, pnt - org);

    return (pnt - proj_vec);
}

/*! Compute the projection of a vector onto a given plane given by the normal of the plane*/

vec3d proj_vec_to_plane(const vec3d &vec, const vec3d &norm)
{
    double ca = cos_angle(vec, norm);

    if (std::abs(ca) == 1.0)
    {
        return vec3d();
    }

    vec3d along = ca * norm;

    return vec - along;
}

/*!
  Find The Point On Line AB nearest to Line CD TODO this description is only partially true.
  What this does is checks if 2 rays intersect at all, and as a side effect gives the intersection point into a parameter fed into the function.
  */

int ray_ray_intersect(vec3d &A, vec3d &B, vec3d &C, vec3d &D, vec3d &int_pnt1, vec3d &int_pnt2)
// Why 2 intersection points? You will only ever have one?
/*
{
    //======= NOT TESTED !!!!!!!!!!!!!!! ===//
    vec3d line1 = B - A;
    vec3d line2 = C - D;

    vec3d normal = cross(line1, line2);

    if (normal.mag() <= 0.0)
    {
        //===== Parallel Lines =====//
        return (0);
    }
    else
    {
        double t = 0.0;
        if (plane_ray_intersect(A, line1, normal, C, line2, t))
        {
            int_pnt2 = C + (line2 * t);
        }
        else
        {
            // What UP?
            cout << " RAY RAY INTERSECT - WHAT UP 1? " << endl;
        }
        if (plane_ray_intersect(C, line2, normal, A, line1, t))
        {
            int_pnt1 = A + (line1 * t);
        }
        else
        {
            // What UP?
            cout << " RAY RAY INTERSECT - WHAT UP 2? " << endl;
        }
    }
    return (1);
}
*/
// TODO This looks more complicated than it needs to be. My suggestion is the following where I am interpreting inputs as follows:
// A is the start of the ray, B is another point in the ray, C is the start of the other ray, D is a point in the ray

{
    //======= NOT TESTED !!!!!!!!!!!!!!! ===//
    vec3d line1 = B - A;
    vec3d line2 = D - C;

    vec3d normal = cross(line1, line2);

    if (normal.mag() <= 0.0)
    {
        //===== Parallel Lines =====//
        return (0);
    }
    vec3d AminusC = A - C;
    if (dot(cross(line1, line2), AminusC) == 0.0) // TODO could do absolute value and check less than given value
    {
        // The cross product of the vectors line1 and line2 would be perpendicular to the plane defined by the lines if they are on the same line.
        // Any of the vectors AC, AD, BC,BD are perpendicular to the normal vector if and only if the lines are coplanar
        // If the dot product of two non-zero vectors is zero, the vectors are perpendicular.
        // If two lines are not parallel and are coplanar, they intersect.
        // If all you want is if the lines intersect, return 1; here, but I am following the ray description.

        // Solve system of equations for point vector form vectors of different variables. If both are >=0 return 1;
        Eigen::Matrix3d systemtosolve;
        Eigen::Vector3d constantstosolveto;
        systemtosolve << line1[0], -line2[0], 0, line1[1], -line2[1], 0, line1[2], -line2[2], 0;
        constantstosolveto << -AminusC[0], -AminusC[1], -AminusC[2];
        Eigen::Vector3d result = systemtosolve.colPivHouseholderQr().solve(constantstosolveto);
        if (result(0) >= 0.0 && result(1) >= 0.0)
        {
            int_pnt1 = A + result(0) * line1;
            int_pnt2 = C + result(1) * line2;
            return 1;
        }
    }
    else
    {
        return 0;
    }
}

/*!
  Triangle - Line Segment Intersection
  A - Base Point on Triangle
  B - Vector for one   Side of Tri
  C - Vector for other Side of Tri
  D - Base Point for Line Seg
  E - Vector for Line Seg
  u - ???
  w - ???
  t - ???
  */

int tri_seg_intersect(vec3d &A, vec3d &B, vec3d &C, vec3d &D, vec3d &E,
                      double &u, double &w, double &t)
{
    double zero = -1.0e-08; // TODO your own single function defined magic number? really?
    double one = 1.0 - zero;

    vec3d cs = cross(B, C);
    double denom = dot(cs, E);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    t = (dot(cs, A) - dot(cs, D)) / denom;

    if ((t < zero) || (t > one))
    {
        return (0);
    }

    cs = cross(C, E);
    denom = dot(cs, B);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    u = (dot(cs, D) - dot(cs, A)) / denom;

    if ((u < zero) || (u > one))
    {
        return (0);
    }

    cs = cross(B, E);
    denom = dot(cs, C);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    w = (dot(cs, D) - dot(cs, A)) / denom;

    if ((w < zero) || (w > one))
    {
        return (0);
    }

    if ((w + u) > one)
    {
        return (0);
    }

    return (1);
}

// TODO need way more details about what this is doing.

/*!
 Triangle - Line Segment Intersection
 A - Base Point on Triangle
 B - Vector for one   Side of Tri
 C - Vector for other Side of Tri
 D - Base Point for Ray
 E - Vector for Ray
 u - ???
 w - ???
 t - ???
*/
int tri_ray_intersect(vec3d &A, vec3d &B, vec3d &C, vec3d &D, vec3d &E,
                      double &u, double &w, double &t)
{
    double zero = -1.0e-08;
    double one = 1.0 - zero;

    vec3d cs = cross(B, C);
    double denom = dot(cs, E);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    t = (dot(cs, A) - dot(cs, D)) / denom;

    cs = cross(C, E);
    denom = dot(cs, B);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    u = (dot(cs, D) - dot(cs, A)) / denom;

    if ((u < zero) || (u > one))
    {
        return (0);
    }

    cs = cross(B, E);
    denom = dot(cs, C);

    // if ( std::abs(denom) == 0.0 ) return(0);
    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    w = (dot(cs, D) - dot(cs, A)) / denom;

    if ((w < zero) || (w > one))
    {
        return (0);
    }

    if ((w + u) > one)
    {
        return (0);
    }

    return (1);
}

// TODO need way more details about what this is doing.
/*!
  Plane - Ray Intersection
  A - Base Point on Plane
  B - Vector for one   Side of Plane
  C - Vector for other Side of Plane
  D - Base Point for Ray
  E - Vector for Ray
  t - ???
  */
int plane_ray_intersect(vec3d &A, vec3d &B, vec3d &C, vec3d &D, vec3d &E, double &t)
{
    vec3d cs = cross(B, C);
    double denom = dot(cs, E);

    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    t = (dot(cs, A) - dot(cs, D)) / denom;

    return (1);
}
// TODO need way more details about what this is doing.
/*!
  Plane - Ray Intersection
  Plane - Line Segment Intersection
  orig - Orig of Plane
  norm - Normal of Plane
  D - Base Point for Ray
  E - Vector for Ray
  t - ???
  */
int plane_ray_intersect(vec3d &orig, vec3d &norm, vec3d &D, vec3d &E, double &t)
{
    double denom = dot(norm, E);

    if (std::abs(denom) <= DBL_EPSILON)
    {
        return (0);
    }

    t = (dot(norm, orig) - dot(norm, D)) / denom;

    return (1);
}

/*!
  Signed Volume Of Tetrahedron Defined By Three Vectors From a Common Point
  */
// Note that the signed area of a tetrahedron is 1/6 the signed area of the parallelepiped, which has an area equal to the determinant of the vectors.
double tetra_volume(vec3d &A, vec3d &B, vec3d &C)
{
    double determ = A.v[0] * B.v[1] * C.v[2] + B.v[0] * C.v[1] * A.v[2] + C.v[0] * A.v[1] * B.v[2] - C.v[0] * B.v[1] * A.v[2] - B.v[0] * A.v[1] * C.v[2] - A.v[0] * C.v[1] * B.v[2];

    return (determ / 6.0);
}

/*!
  Area of triangle defined by three points
  */
// Do not use naive Heron's formula.  Instead, use an algorithm that is stable in the case of slivers
// and degenerate triangles http://http.cs.berkeley.edu/~wkahan/Triangle.pdf
double area(vec3d &A, vec3d &B, vec3d &C)
{
    double a = (B - A).mag();
    double b = (C - A).mag();
    double c = (C - B).mag();

    // Place in increasing order a, b, c.
    if (a < b)
    {
        std::swap(a, b);
    }
    if (a < c)
    {
        std::swap(a, c);
    }
    if (b < c)
    {
        std::swap(b, c);
    }

    if (c - (a - b) < 0.0)
    {
        // Not a real triangle.
        return 0.0;
    }

    return 0.25 * sqrt((a + (b + c)) * (c - (a - b)) * (c + (a - b)) * (a + (b - c)));
}

// dist3D_Segment_to_Segment based on code by Dan Sunday
// http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
//
// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

/*!
  Distance from line segment defined by two points to another line segment defined by two points
  */
double dist3D_Segment_to_Segment(vec3d &S1P0, vec3d &S1P1, vec3d &S2P0, vec3d &S2P1)
{
    double SMALL_NUM = 0.0000001; // TODO magic number. Standardize.

    vec3d u = S1P1 - S1P0;
    vec3d v = S2P1 - S2P0;
    vec3d w = S1P0 - S2P0;
    double a = dot(u, u); // always >= 0
    double b = dot(u, v);
    double c = dot(v, v); // always >= 0
    double d = dot(u, w);
    double e = dot(v, w);
    double D = a * c - b * b; // always >= 0
    double sc, sN, sD = D;    // sc = sN / sD, default sD = D >= 0
    double tc, tN, tD = D;    // tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < SMALL_NUM) // the lines are almost parallel
    {
        sN = 0.0; // force using point P0 on segment S1
        sD = 1.0; // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else // get the closest points on the infinite lines
    {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0) // sc < 0 => the s=0 edge is visible
        {
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if (sN > sD) // sc > 1 => the s=1 edge is visible
        {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) // tc < 0 => the t=0 edge is visible
    {
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0)
        {
            sN = 0.0;
        }
        else if (-d > a)
        {
            sN = sD;
        }
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD) // tc > 1 => the t=1 edge is visible
    {
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0)
        {
            sN = 0;
        }
        else if ((-d + b) > a)
        {
            sN = sD;
        }
        else
        {
            sN = (-d + b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (std::abs(sN) < SMALL_NUM ? 0.0 : sN / sD);
    tc = (std::abs(tN) < SMALL_NUM ? 0.0 : tN / tD);

    // get the difference of the two closest points
    vec3d dP = w + (u * sc) - (v * tc); // = S1(sc) - S2(tc)

    return dP.mag(); // return the closest distance
}

// dist3D_Segment_to_Segment based on code by Dan Sunday
// http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
//
// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
//

/*!
  Distance from line segment defined by two points to another line segment defined by two points
  Also has extra parameters Lt, Ln, St, Sn which TODO needs descriptions.
  */
double dist3D_Segment_to_Segment(vec3d &S1P0, vec3d &S1P1, vec3d &S2P0, vec3d &S2P1,
                                 double *Lt, vec3d *Ln, double *St, vec3d *Sn)
{
    double SMALL_NUM = 0.0000001;

    vec3d u = S1P1 - S1P0;
    vec3d v = S2P1 - S2P0;
    vec3d w = S1P0 - S2P0;
    double a = dot(u, u); // always >= 0
    double b = dot(u, v);
    double c = dot(v, v); // always >= 0
    double d = dot(u, w);
    double e = dot(v, w);
    double D = a * c - b * b; // always >= 0
    double sc, sN, sD = D;    // sc = sN / sD, default sD = D >= 0
    double tc, tN, tD = D;    // tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < SMALL_NUM) // the lines are almost parallel
    {
        sN = 0.0; // force using point P0 on segment S1
        sD = 1.0; // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    }
    else // get the closest points on the infinite lines
    {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0) // sc < 0 => the s=0 edge is visible
        {
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if (sN > sD) // sc > 1 => the s=1 edge is visible
        {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) // tc < 0 => the t=0 edge is visible
    {
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0)
        {
            sN = 0.0;
        }
        else if (-d > a)
        {
            sN = sD;
        }
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD) // tc > 1 => the t=1 edge is visible
    {
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0)
        {
            sN = 0;
        }
        else if ((-d + b) > a)
        {
            sN = sD;
        }
        else
        {
            sN = (-d + b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (std::abs(sN) < SMALL_NUM ? 0.0 : sN / sD);
    tc = (std::abs(tN) < SMALL_NUM ? 0.0 : tN / tD);

    // get the difference of the two closest points
    vec3d dP = w + (u * sc) - (v * tc); // = S1(sc) - S2(tc)

    *Ln = S1P0 + u * sc;
    *Sn = S2P0 + v * tc;

    *Lt = sc;
    *St = tc;

    return dP.mag(); // return the closest distance
}

/*!
  Find Nearest Points On 2 Line Segs and return distance
  TODO need more details
  */
double nearSegSeg(const vec3d &L0, const vec3d &L1, const vec3d &S0, const vec3d &S1,
                  double *Lt, vec3d *Ln, double *St, vec3d *Sn)
{
    vec3d u = L1 - L0;
    vec3d v = S1 - S0;
    vec3d w = L0 - S0;

    double a = dot(u, u);
    double b = dot(u, v);
    double c = dot(v, v);
    double d = dot(u, w);
    double e = dot(v, w);

    double D = a * c - b * b;
    double sc = 0.0;
    double sN = 0.0;
    double sD = D;
    double tc = 0.0;
    double tN = 0.0;
    double tD = D;

    // compute the line parameters of the two closest points
    if (D < 0.0000001) // the lines are almost parallel
    {
        sN = 0.0;
        tN = e;
        tD = c;
        sD = c;
    }
    else // get the closest points on the infinite lines
    {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0) // sc < 0 => the s=0 edge is visible
        {
            sN = 0.0;
            tN = e;
            tD = c;
        }
        else if (sN > sD) // sc > 1 => the s=1 edge is visible
        {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0) // tc < 0 => the t=0 edge is visible
    {
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0)
        {
            sN = 0.0;
        }
        else if (-d > a)
        {
            sN = sD;
        }
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD) // tc > 1 => the t=1 edge is visible
    {
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0)
        {
            sN = 0;
        }
        else if ((-d + b) > a)
        {
            sN = sD;
        }
        else
        {
            sN = (-d + b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = sN / sD;
    tc = tN / tD;

    *Ln = L0 + u * sc;
    *Sn = S0 + v * tc;

    *Lt = sc;
    *St = tc;

    return dist(*Ln, *Sn);
}

/*!
  Compute the square of the distance between a point and a line?
  TODO Does the logic of this make sense? I am having a hard time seeing a reasonable physical interpretation.
  */

double pointLineDistSquared(const vec3d &X0, const vec3d &X1, const vec3d &X2, double *t)
{
    vec3d X10 = X1 - X0;
    vec3d X21 = X2 - X1;

    double denom = dist_squared(X2, X1);

    if (denom < 1e-9) // was 1e-9
    {
        *t = 0.0;
    }
    else
    {
        *t = -dot(X10, X21) / denom;
    }

    vec3d Xon = X1 + X21 * (*t);

    return dist_squared(Xon, X0);
}

/*!
  Computes the location of a point a that is projected along a line given by two points a distance a multiple of the original line length.
  */

vec3d point_on_line(const vec3d &lp0, const vec3d &lp1, const double &t)
{
    vec3d s10 = lp1 - lp0;
    return lp0 + s10 * t;
}

/*!
  Compute the square of the distance between a point and a line segment?
  TODO Does the logic of this make sense? I am having a hard time seeing a reasonable physical interpretation.
  */
double pointSegDistSquared(const vec3d &p, const vec3d &sp0, const vec3d &sp1, double *t)
{
    double dSqr = pointLineDistSquared(p, sp0, sp1, t);

    if (*t < 0)
    {
        *t = 0;
        dSqr = dist_squared(p, sp0);
    }
    else if (*t > 1)
    {
        *t = 1;
        dSqr = dist_squared(p, sp1);
    }

    return dSqr;
}

/*!
  Mapping something to a plane
  TODO, needs some sort of explanation
  */

vec2d MapToPlane(const vec3d &P, const vec3d &B, const vec3d &e0, const vec3d &e1)
{
    vec2d result;
    vec3d BmP = B - P;
    vec3d zero;
    vec3d me1 = zero - e1;
    double a = dot(e0, e0);
    double b = dot(e0, e1);
    double c = dot(e1, e1);
    double d = dot(e0, BmP);
    double e = dot(e1, BmP);

    double s = 0;
    double t = 0;
    double denom = a * c - b * b;
    if (denom)
    {
        s = (b * e - c * d) / denom;
        t = (b * d - a * e) / denom;
    }

    result.set_xy(s, t);

    return result;
}

/*!
  Map something from a plane to somewhere
  TODO, needs an explanation
  */

vec3d MapFromPlane(const vec2d &uw, const vec3d &B, const vec3d &e0, const vec3d &e1)
{
    vec3d result = B + e0 * uw.x() + e1 * uw.y();
    return result;
}

/*!
  Returns the sign of where the point is with respect to a plane, with positive being in the direction of the normal and negative against the normal.
  */

int plane_half_space(vec3d &planeOrig, vec3d &planeNorm, vec3d &pnt)
{
    double od = dot(planeNorm, planeOrig);
    double pd = dot(planeNorm, pnt);

    if (pd > od)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

// pint12 = p1 + s*(p2-p1)
// pint34 = p3 + t*(p4-p3)

/*!
  Check if two lines intersect?
  TODO figure out what this is actually doing and add comments or fix it.
  */

bool line_line_intersect(vec3d &p1, vec3d &p2, vec3d &p3, vec3d &p4, double *s, double *t)
{
    vec3d p13 = p1 - p3;
    vec3d p43 = p4 - p3;
    // dot product of a vector with itself is just its magnitude squared
    double d4343 = dot(p43, p43);
    if (d4343 < DBL_EPSILON)
    {
        return false;
    }

    vec3d p21 = p2 - p1;
    double d2121 = dot(p21, p21);
    if (d2121 < DBL_EPSILON)
    {
        return false;
    }

    double d1343 = dot(p13, p43);
    double d4321 = dot(p43, p21);
    double d1321 = dot(p13, p21);

    double denom = d2121 * d4343 - d4321 * d4321;
    if (std::abs(denom) < DBL_EPSILON)
    {
        return false;
    }

    double numer = d1343 * d4321 - d1321 * d4343;

    *s = numer / denom;
    *t = (d1343 + d4321 * (*s)) / d4343;

    return true;
}

/*!
  Rotate a point p by angle theta in radians around an arbitrary axis r
  Return the rotated point.
  Positive angles are anticlockwise looking down the axis
  towards the origin.
  Assume right hand coordinate system.
  */
vec3d RotateArbAxis(const vec3d &p, double theta, const vec3d &axis) // Radians
{
    vec3d q(0, 0, 0);
    double costheta, sintheta;

    vec3d r = axis;
    r.normalize();
    costheta = cos(theta);
    sintheta = sin(theta);

    q[0] += (costheta + (1 - costheta) * r[0] * r[0]) * p[0];
    q[0] += ((1 - costheta) * r[0] * r[1] - r[2] * sintheta) * p[1];
    q[0] += ((1 - costheta) * r[0] * r[2] + r[1] * sintheta) * p[2];

    q[1] += ((1 - costheta) * r[0] * r[1] + r[2] * sintheta) * p[0];
    q[1] += (costheta + (1 - costheta) * r[1] * r[1]) * p[1];
    q[1] += ((1 - costheta) * r[1] * r[2] - r[0] * sintheta) * p[2];

    q[2] += ((1 - costheta) * r[0] * r[2] - r[1] * sintheta) * p[0];
    q[2] += ((1 - costheta) * r[1] * r[2] + r[0] * sintheta) * p[1];
    q[2] += (costheta + (1 - costheta) * r[2] * r[2]) * p[2];

    return (q);
}

/*!
  Find the area of a 2D (XY) polygon
  */
double poly_area(const vector<vec3d> &pnt_vec)
// TODO this algorithm computes double the area of the polygon. Not sure why it is not being divided by 2.
// Also, I seems to go out of bounds as vectors start at 0 and size already gives one more than max index
// Ex in the case of 4 points, 0 to 3, 3 will pass and compute pnt_vec[3+1], which is out of bounds. pnt_vec.at(x) checks for this.
{
    if (pnt_vec.size() < 3)
    {
        return 0.0;
    }

    double total_area = 0.0;
    for (int i = 0; i < (int)(pnt_vec.size() - 1); i++)
    {
        total_area += pnt_vec[i].x() * pnt_vec[i + 1].y() - pnt_vec[i + 1].x() * pnt_vec[i].y();
    }

    if (dist(pnt_vec[0], pnt_vec.back()) > 0.0000001) // TODO magic number
    {
        total_area += pnt_vec.back().x() * pnt_vec[0].y() - pnt_vec[0].x() * pnt_vec.back().y();
    }

    return std::abs(total_area);
}

/*!
  Compute the Barycentric Weights of a point, p, inside of the triangle with vertices v0,v1, and v2
  Assumed counter-clockwise of v0, v1, v2 defines normal
  */

vec3d BarycentricWeights(const vec3d &v0, const vec3d &v1, const vec3d &v2, const vec3d &p)
{

    vec3d weights;

    vec3d n = cross(v1 - v0, v2 - v0);
    vec3d na = cross(v2 - v1, p - v1);
    vec3d nb = cross(v0 - v2, p - v2);
    vec3d nc = cross(v1 - v0, p - v0);
    double n_mag = n.mag();
    double denom = n_mag * n_mag;
    double tol = 0.000001; // TODO magic number

    if (n_mag < tol)
    {
        return weights;
    }

    weights.set_x(dot(n, na) / denom);
    weights.set_y(dot(n, nb) / denom);
    weights.set_z(dot(n, nc) / denom);

    return weights;
}

/*!
  Bilinear Interpolation
  p0 is lower left corner of rectangle
  p1 is upper right corner of rectangle
  p is a point in the rectangle
  weights will be filled in with the interpolation weights in order of a counter clockwise definition of
  a rectangle
  */
void BilinearWeights(const vec3d &p0, const vec3d &p1, const vec3d &p, std::vector<double> &weights)
{

    double w0, w1, w2, w3, dx, dy, dx0p, dx1p, dy0p, dy1p, denom;
    double zero_tol = 1e-12; // TODO magic number

    dx = (p1.x() - p0.x());
    dy = (p1.y() - p0.y());
    denom = dx * dy;

    if (std::abs(denom) < zero_tol)
    {
        return;
    }

    dx0p = p.x() - p0.x();
    dx1p = p1.x() - p.x();
    dy0p = p.y() - p0.y();
    dy1p = p1.y() - p.y();

    w0 = dx1p * dy1p / denom;
    w1 = dx0p * dy1p / denom;
    w2 = dx1p * dy0p / denom;
    w3 = dx0p * dy0p / denom;

    weights.resize(4);
    weights[0] = w0;
    weights[1] = w1;
    weights[2] = w3;
    weights[3] = w2;
}

/*!
  Computes the minimum distance between two triangles defined by their points
  The first three points are the points of one triangle, and the next three are the points of the other.
  */
double tri_tri_min_dist(vec3d &v0, vec3d &v1, vec3d &v2, vec3d &v3, vec3d &v4, vec3d &v5)
{
    double d;
    double min_dist = 1.0e12; // TODO magic number

    d = pnt_tri_min_dist(v0, v1, v2, v3);
    min_dist = std::min(d, min_dist);
    d = pnt_tri_min_dist(v0, v1, v2, v4);
    min_dist = std::min(d, min_dist);
    d = pnt_tri_min_dist(v0, v1, v2, v5);
    min_dist = std::min(d, min_dist);

    d = pnt_tri_min_dist(v3, v4, v5, v0);
    min_dist = std::min(d, min_dist);
    d = pnt_tri_min_dist(v3, v4, v5, v1);
    min_dist = std::min(d, min_dist);
    d = pnt_tri_min_dist(v3, v4, v5, v2);
    min_dist = std::min(d, min_dist);

    return min_dist;
}

/*!
  Computes the distance between a point and the nearest side of a triangle.
  */

double pnt_tri_min_dist(vec3d &v0, vec3d &v1, vec3d &v2, vec3d &pnt)
{
    vec2d uw = MapToPlane(pnt, v0, v1 - v0, v2 - v0);
    vec3d plnpnt = MapFromPlane(uw, v0, v1 - v0, v2 - v0);

    //====  Check If Point In Tri ====//
    if (uw[0] >= 0.0 && uw[0] <= 1.0 && uw[1] >= 0.0 && uw[1] <= 1.0 &&
        uw[0] + uw[1] <= 1.0)
    {
        return dist(pnt, plnpnt);
    }
    double t;
    double d01 = pointSegDistSquared(pnt, v0, v1, &t);
    double d12 = pointSegDistSquared(pnt, v1, v2, &t);
    double d20 = pointSegDistSquared(pnt, v2, v0, &t);

    if (d01 < d12 && d01 < d20)
        return sqrt(d01);
    else if (d12 < d20)
        return sqrt(d12);
    else
        return sqrt(d20);
}

/*!
Add vector message string function to the std namespace.
*/

namespace std
{
    string to_string(const vec3d &v)
    {
        return "x: " + std::to_string(v.x()) +
               " y: " + std::to_string(v.y()) +
               " z: " + std::to_string(v.z());
    }
}

/*!
  Spherical linear interpolation between direction vectors.
  Intermediate vectors follow great circle path with constant velocity.
  */
vec3d slerp(const vec3d &a, const vec3d &b, const double &t)
{
    vec3d an = a / a.mag(); // TODO just call the normalize function
    vec3d bn = b / b.mag(); // TODO just call the normalize function

    double dp = dot(an, bn);

    double theta = 0.0;
    if (dp >= -1.0 && dp <= 1.0)
    {
        theta = acos(dp);
    }

    // Initialize retvec as a-direction.
    vec3d retvec = an;

    // If vectors are not parallel, interpolate between them.
    if (std::abs(theta) > 1.0e-6) // TODO magic number
    {
        // Drop division by sin(theta) because .normalize() will scale
        double coeff1 = sin((1.0 - t) * theta); // implied  / sin(theta)
        double coeff2 = sin(t * theta);         // implied  / sin(theta)

        retvec = coeff1 * an + coeff2 * bn;
        retvec.normalize();
    }

    return retvec;
}
