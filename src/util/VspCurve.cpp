//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// VspCurve.h:
// J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <float.h>

#include "VspCurve.h"

#include "eli/geom/curve/length.hpp"
#include "eli/geom/curve/piecewise_creator.hpp"
#include "eli/geom/intersect/minimum_distance_curve.hpp"
#include "eli/geom/intersect/specified_distance_curve.hpp"
#include "eli/geom/intersect/specified_thickness_curve.hpp"
#include "eli/geom/curve/piecewise_adaptive_airfoil_fitter.hpp"

typedef piecewise_curve_type::index_type curve_index_type;
typedef piecewise_curve_type::point_type curve_point_type;
typedef piecewise_curve_type::rotation_matrix_type curve_rotation_matrix_type;
typedef piecewise_curve_type::bounding_box_type curve_bounding_box_type;
typedef piecewise_curve_type::tolerance_type curve_tolerance_type;


typedef eli::geom::curve::piecewise_cubic_spline_creator<double, 3, curve_tolerance_type> piecewise_cubic_spline_creator_type;
typedef eli::geom::curve::piecewise_linear_creator<double, 3, curve_tolerance_type> piecewise_linear_creator_type;

typedef eli::geom::curve::piecewise_general_creator<double, 3, curve_tolerance_type> general_creator_type;
typedef typename general_creator_type::joint_data joint_data_type;

typedef eli::geom::curve::piecewise_adaptive_airfoil_fitter<double, 3, curve_tolerance_type> adaptive_fitter_type;
typedef typename adaptive_fitter_type::subdivide_method subdivide_method;

//=============================================================================//
//============================= VspCurve      =================================//
//=============================================================================//

//===== Constructor  =====//
VspCurve::VspCurve()
{
}

//===== Destructor  =====//
VspCurve::~VspCurve()
{
}

//==== Copy From Input Curve =====//
void VspCurve::Copy( VspCurve & input_crv )
{
    m_Curve = input_crv.m_Curve;
}

//==== Split at Specified Parameter and Return Remaining Curve =====//
void VspCurve::Split( double u )
{
    m_Curve.split( u );
}

//==== Append Curve To Existing Curve ====//
void VspCurve::Append( VspCurve & input_crv )
{
    curve_index_type i, nc( input_crv.GetNumSections() );

    for ( i = 0; i < nc; ++i )
    {
        piecewise_curve_type::error_code ec;
        curve_segment_type c;

        input_crv.GetCurveSegment( c, i );
        ec = m_Curve.push_back( c, input_crv.GetCurveDt( i ) );
        if ( ec != piecewise_curve_type::NO_ERRORS )
        {
            std::cerr << "Could not append curve." << std::endl;
        }
    }
}

bool VspCurve::IsClosed() const
{
    return m_Curve.closed();
}

#if 0
void octave_print( int figno, const piecewise_curve_type &pc )
{
    curve_index_type i, pp, ns;
    double tmin, tmax;

    ns = pc.number_segments();
    pc.get_parameter_min( tmin );
    pc.get_parameter_max( tmax );

    std::cout << "figure(" << figno << ");" << std::endl;

    // get control points and print
    std::cout << "cp_x=[";
    for ( pp = 0; pp < ns; ++pp )
    {
        curve_segment_type bez;
        pc.get( bez, pp );
        for ( i = 0; i <= bez.degree(); ++i )
        {
            std::cout << bez.get_control_point( i ).x();
            if ( i < bez.degree() )
            {
                std::cout << ", ";
            }
            else if ( pp < ns - 1 )
            {
                std::cout << "; ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "];" << std::endl;

    std::cout << "cp_y=[";
    for ( pp = 0; pp < ns; ++pp )
    {
        curve_segment_type bez;
        pc.get( bez, pp );
        for ( i = 0; i <= bez.degree(); ++i )
        {
            std::cout << bez.get_control_point( i ).y();
            if ( i < bez.degree() )
            {
                std::cout << ", ";
            }
            else if ( pp < ns - 1 )
            {
                std::cout << "; ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "];" << std::endl;

    std::cout << "cp_z=[";
    for ( pp = 0; pp < ns; ++pp )
    {
        curve_segment_type bez;
        pc.get( bez, pp );
        for ( i = 0; i <= bez.degree(); ++i )
        {
            std::cout << bez.get_control_point( i ).z();
            if ( i < bez.degree() )
            {
                std::cout << ", ";
            }
            else if ( pp < ns - 1 )
            {
                std::cout << "; ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "];" << std::endl;

    // initialize the t parameters
    std::vector<double> t( 129 );
    for ( i = 0; i < static_cast<curve_index_type>( t.size() ); ++i )
    {
        t[i] = tmin + ( tmax - tmin ) * static_cast<double>( i ) / ( t.size() - 1 );
    }

    // set the surface points
    std::cout << "surf_x=[";
    for ( i = 0; i < static_cast<curve_index_type>( t.size() ); ++i )
    {
        std::cout << pc.f( t[i] ).x();
        if ( i < static_cast<curve_index_type>( t.size() - 1 ) )
        {
            std::cout << ", ";
        }
    }
    std::cout << "];" << std::endl;

    std::cout << "surf_y=[";
    for ( i = 0; i < static_cast<curve_index_type>( t.size() ); ++i )
    {
        std::cout << pc.f( t[i] ).y();
        if ( i < static_cast<curve_index_type>( t.size() - 1 ) )
        {
            std::cout << ", ";
        }
    }
    std::cout << "];" << std::endl;

    std::cout << "surf_z=[";
    for ( i = 0; i < static_cast<curve_index_type>( t.size() ); ++i )
    {
        std::cout << pc.f( t[i] ).z();
        if ( i < static_cast<curve_index_type>( t.size() - 1 ) )
        {
            std::cout << ", ";
        }
    }
    std::cout << "];" << std::endl;

    std::cout << "setenv('GNUTERM', 'x11');" << std::endl;
    std::cout << "plot3(surf_x, surf_y, surf_z, '-k');" << std::endl;
    std::cout << "hold on;" << std::endl;
    std::cout << "plot3(cp_x', cp_y', cp_z', '-ok', 'MarkerFaceColor', [0 0 0]);" << std::endl;
    std::cout << "hold off;" << std::endl;
}
#endif

void VspCurve::RoundJoint( double rad, int i )
{
    m_Curve.round( rad, i );
}

void VspCurve::RoundAllJoints( double rad )
{
    m_Curve.round( rad );
}

//===== Interpolate Creates piecewise linear curves ===//
void VspCurve::InterpolateLinear( vector< vec3d > & input_pnt_vec, const vector<double> &param, bool closed_flag )
{
    // do some checking of vector lengths
    if ( closed_flag )
    {
        if ( param.size() != ( input_pnt_vec.size() + 1 ) )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }
    else
    {
        if ( param.size() != input_pnt_vec.size() )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }

    // copy points over to new type
    vector<curve_point_type> pts( input_pnt_vec.size() );
    for ( size_t i = 0; i < pts.size(); ++i )
    {
        pts[i] << input_pnt_vec[i].x(), input_pnt_vec[i].y(), input_pnt_vec[i].z();
    }

    if ( closed_flag )
    {
        pts.push_back( pts[0] );
    }

    int nseg( pts.size() - 1 );
    piecewise_linear_creator_type plc( nseg );

    // set the delta t for each curve segment
    plc.set_t0( param[0] );
    for ( curve_index_type i = 0; i < plc.get_number_segments(); ++i )
    {
        plc.set_segment_dt( param[i + 1] - param[i], i );
    }

    // set the polygon corners
    for ( curve_index_type i = 0; i < static_cast<curve_index_type>( pts.size() ); ++i )
    {
        plc.set_corner( pts[i], i );
    }

    if ( !plc.create( m_Curve ) )
    {
        std::cerr << "Failed to create linear curve. " << __LINE__ << std::endl;
    }
}

//===== Interpolate Creates PCHIP ====//
void VspCurve::InterpolatePCHIP( vector< vec3d > & input_pnt_vec, const vector<double> &param, bool closed_flag )
{
    // do some checking of vector lengths
    if ( closed_flag )
    {
        if ( param.size() != ( input_pnt_vec.size() + 1 ) )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }
    else
    {
        if ( param.size() != input_pnt_vec.size() )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }

    // copy points over to new type
    vector<curve_point_type> pts( input_pnt_vec.size() );
    for ( size_t i = 0; i < pts.size(); ++i )
    {
        pts[i] << input_pnt_vec[i].x(), input_pnt_vec[i].y(), input_pnt_vec[i].z();
    }

    // create creator for known number of segments
    int nseg( pts.size() - 1 );
    if ( closed_flag )
    {
        ++nseg;
    }
    piecewise_cubic_spline_creator_type pcsc( nseg );

    // set the delta t for each curve segment
    pcsc.set_t0( param[0] );
    for ( size_t i = 0; i < ( param.size() - 1 ); ++i )
    {
        pcsc.set_segment_dt( param[i + 1] - param[i], i );
    }

    if ( closed_flag )
    {
        pcsc.set_chip( pts.begin(), eli::geom::general::C1 );
    }
    else
    {
        pcsc.set_chip( pts.begin(), eli::geom::general::NOT_CONNECTED );
    }

    if ( !pcsc.create( m_Curve ) )
    {
        std::cerr << "Failed to create PCHIP. " << __LINE__ << std::endl;
    }
}

//===== Interpolate Creates cubic spline with either not-a-knot ends or closed ends  ====//
void VspCurve::InterpolateCSpline( vector< vec3d > & input_pnt_vec, const vector<double> &param, bool closed_flag )
{
    // do some checking of vector lengths
    if ( closed_flag )
    {
        if ( param.size() != ( input_pnt_vec.size() + 1 ) )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }
    else
    {
        if ( param.size() != input_pnt_vec.size() )
        {
            std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
            assert( false );
            return;
        }
    }

    // copy points over to new type
    vector<curve_point_type> pts( input_pnt_vec.size() );
    for ( size_t i = 0; i < pts.size(); ++i )
    {
        pts[i] << input_pnt_vec[i].x(), input_pnt_vec[i].y(), input_pnt_vec[i].z();
    }

    // create creator for known number of segments
    int nseg( pts.size() - 1 );
    if ( closed_flag )
    {
        ++nseg;
    }
    piecewise_cubic_spline_creator_type pcsc( nseg );

    // set the delta t for each curve segment
    pcsc.set_t0( param[0] );
    for ( size_t i = 0; i < ( param.size() - 1 ); ++i )
    {
        pcsc.set_segment_dt( param[i + 1] - param[i], i );
    }

    if ( closed_flag )
    {
        pcsc.set_closed_cubic_spline( pts.begin() );
    }
    else
    {
        pcsc.set_cubic_spline( pts.begin() );
    }

    if ( !pcsc.create( m_Curve ) )
    {
        std::cerr << "Failed to create CSpline. " << __LINE__ << std::endl;
    }
}

//===== Interpolate Creates cubic spline with set end slopes ====//
void VspCurve::InterpolateCSpline( vector< vec3d > & input_pnt_vec, const vec3d &start_slope, const vec3d &end_slope, const vector<double> &param )
{
    // do some checking of vector lengths
    if ( param.size() != input_pnt_vec.size() )
    {
        std::cerr << "Invalid number of points and parameters in curve interpolation " << __LINE__ << std::endl;
        assert( false );
        return;
    }

    // copy points over to new type
    vector<curve_point_type> pts( input_pnt_vec.size() );
    curve_point_type sslope, eslope;

    for ( size_t i = 0; i < pts.size(); ++i )
    {
        pts[i] << input_pnt_vec[i].x(), input_pnt_vec[i].y(), input_pnt_vec[i].z();
    }
    sslope << start_slope.x(), start_slope.y(), start_slope.z();
    eslope << end_slope.x(), end_slope.y(), end_slope.z();

    // create creator for known number of segments
    piecewise_cubic_spline_creator_type pcsc( pts.size() - 1 );

    // set the delta t for each curve segment
    pcsc.set_t0( param[0] );
    for ( size_t i = 0; i < ( param.size() - 1 ); ++i )
    {
        pcsc.set_segment_dt( param[i + 1] - param[i], i );
    }

    pcsc.set_clamped_cubic_spline( pts.begin(), sslope, eslope );

    if ( !pcsc.create( m_Curve ) )
    {
        std::cerr << "Failed to create CSpline. " << __LINE__ << std::endl;
    }
}

void VspCurve::GenFit( const vector< vector < vec3d > > & pnt_vec_vec, const vector < double > & param, bool closed_flag )
{
	curve_index_type nsegs, npt;

    nsegs = pnt_vec_vec.size();

    if ( param.size() != nsegs + 1 )
    {
        std::cerr << "Invalid number of segments and parameters in general fit. " << __LINE__ << std::endl;
        assert( false );
        return;
    }

    std::vector<typename general_creator_type::joint_data> joints(nsegs+1);
    std::vector<typename general_creator_type::index_type> max_degree(nsegs);
    std::vector<typename general_creator_type::fit_data> fit_points(nsegs);

    general_creator_type gc;
    curve_point_type p;

    for ( size_t i = 0; i < nsegs; i++ )
    {
        vec3d pt = pnt_vec_vec[i][0];
        p << pt.x(), pt.y(), pt.z();
        joints[i].set_f( p );
    }
    vec3d pt = pnt_vec_vec[nsegs-1].back();
    p << pt.x(), pt.y(), pt.z();
    joints[nsegs].set_f( p );

    joints[1].set_continuity( general_creator_type::C2 );

    // Assume each group of points has a start.  However, the last point in a group
    // does not duplicate the first point in the next group.
    // The final group's final point shall be the final point.

    for ( size_t i = 0; i < nsegs; i++ )
    {
        max_degree[i] = 3;

        npt = pnt_vec_vec[i].size();
        if ( i == nsegs - 1 ) // Don't add last point on last segment.
        {
            npt--;
        }

        for ( size_t j = 1; j < npt; j++ )
        {
            vec3d pt = pnt_vec_vec[i][j];
            p << pt.x(), pt.y(), pt.z();

            fit_points[i].add_point( p );
        }
    }

    gc.set_conditions( joints, fit_points, max_degree, closed_flag );

    // set the delta t for each curve segment
    gc.set_t0( param[0] );
    for ( size_t i = 0; i < nsegs; ++i )
    {
        gc.set_segment_dt( param[i + 1] - param[i], i );
    }

    if ( !gc.create( m_Curve ) )
    {
        std::cerr << "Failed to create general fit. " << __LINE__ << std::endl;
    }
}

void VspCurve::AdaptFit( const vector< vec3d > & uptvec, const vector< vec3d > & lptvec )
{
    vector < curve_point_type > upt( uptvec.size() );
    for ( size_t i = 0; i < uptvec.size(); i++ )
    {
        vec3d pt = uptvec[i];
        upt[i] << pt.x(), pt.y(), pt.z();
    }

    vector < curve_point_type > lpt( lptvec.size() );
    for ( size_t i = 0; i < lptvec.size(); i++ )
    {
        vec3d pt = lptvec[i];
        lpt[i] << pt.x(), pt.y(), pt.z();
    }

    double fit_tol = 1e-4;
    adaptive_fitter_type ac;

    subdivide_method sm(adaptive_fitter_type::MAX_ERROR);

    ac.set_conditions(upt.begin(), static_cast<curve_index_type>(upt.size()),
                      lpt.begin(), static_cast<curve_index_type>(lpt.size()),
                      sm, fit_tol, false);

    ac.set_max_degree( 5 );

    // set the delta t for each curve segment
    ac.set_t0( 0.0 );
    ac.set_segment_dt( 2.0, 0 );
    ac.set_segment_dt( 2.0, 1 );

    if ( !ac.create( m_Curve ) )
    {
        std::cerr << "Failed to create adaptive fit. " << __LINE__ << std::endl;
    }
}

void VspCurve::SetCubicControlPoints( const vector< vec3d > & cntrl_pts, bool closed_flag )
{
    int ncp = cntrl_pts.size();
    int nseg = ( ncp - 1 ) / 3;

    m_Curve.clear();
    m_Curve.set_t0( 0.0 );

    for ( int i = 0; i < nseg; i++ )
    {
        curve_segment_type c;
        c.resize( 3 );

        for ( int j = 0; j < 4; j++ )
        {
            int k = i * 3 + j;

            vec3d p = cntrl_pts[k];

            curve_point_type cp;
            cp << p.x(), p.y(), p.z();

            c.set_control_point( cp, j );
        }
        m_Curve.push_back( c );
    }
}

//===== Interpolate ====//
//void VspCurve::Interpolate( vector< vec3d > & ip_vec, vector< VspPntData > & data_vec, bool closed_flag )
//{
//  std::cerr << "Need to implement " << "VspCurve "<< __LINE__<< std::endl;
//
//  double small_num = 1.0e-10;
//  int ip_size = ip_vec.size();
//
//  if ( ip_size < 2 ) return;
//
//  if ( ip_size == 2 )
//  {
//      LinearInterpolate( ip_vec );
//      return;
//  }
//
//  //==== Check Data Vec Size ====//
//  if ( data_vec.size() != ip_size )
//      data_vec.resize( ip_size );
//
//  //==== Resize Control Points ====//
//  m_ControlPnts.resize( 3*(ip_size-1)+1 );
//
//  //==== Compute Tangent Direction Base On Flags =====//
//  ComputeTanDir( ip_vec, data_vec, closed_flag );
//
//  //==== First Point ====//
//  m_ControlPnts[0] = ip_vec[0];
//
//  double mag = (ip_vec[1] - ip_vec[0]).mag();
//  if ( closed_flag )
//  {
//      double mag_minus = ( ip_vec[ip_size-1] - ip_vec[ip_size-2] ).mag();
//      double skip_mag  = ( ip_vec[1] - ip_vec[ip_size-2]).mag();
//      mag = skip_mag*(mag/(mag_minus + mag  + small_num ));
//  }
//
//  m_ControlPnts[1] = ip_vec[0] + data_vec[0].m_Tan2*(mag*m_Tension);
//
//  //==== Middle Points ====//
//  for ( int i = 1 ; i < (int)ip_vec.size()-1 ; i++ )
//  {
//      double mag_minus = ( ip_vec[i]   - ip_vec[i-1] ).mag();
//      double mag_plus  = ( ip_vec[i+1] - ip_vec[i]).mag();
//      double skip_mag  = ( ip_vec[i+1] - ip_vec[i-1]).mag();
//      double mag_sum   = mag_minus + mag_plus + small_num;
//      mag_minus = skip_mag*(mag_minus/mag_sum);
//      mag_plus  = skip_mag*(mag_plus/mag_sum);
//
//      m_ControlPnts[(i-1)*3+2] = ip_vec[i] + data_vec[i].m_Tan1*(mag_minus*m_Tension);
//      m_ControlPnts[(i-1)*3+3] = ip_vec[i];
//      m_ControlPnts[(i-1)*3+4] = ip_vec[i] + data_vec[i].m_Tan2*(mag_plus*m_Tension);
//
//      if ( data_vec[i].m_Type == VspPntData::SHARP )
//      {
//          vec3d dir = m_ControlPnts[(i-1)*3+1] - ip_vec[i];
//          m_ControlPnts[(i-1)*3+2] = ip_vec[i] + dir*m_Tension;
//      }
//      if ( data_vec[i-1].m_Type == VspPntData::SHARP )
//      {
//          vec3d dir = m_ControlPnts[(i-1)*3+2] - ip_vec[i-1];
//          m_ControlPnts[(i-1)*3+1] = ip_vec[i-1] + dir*m_Tension;
//      }
//  }
//
//  //==== Last Point ====//
//  mag = ( ip_vec[ip_size-1] - ip_vec[ip_size-2] ).mag() + 1.0e-10;
//
//  if ( closed_flag )
//  {
//      double mag_plus = ( ip_vec[1] - ip_vec[0] ).mag();
//      double skip_mag  = ( ip_vec[1] - ip_vec[ip_size-2]).mag();
//      mag = skip_mag*(mag/(mag_plus + mag + small_num ));
//  }
//
//  int c_size = m_ControlPnts.size();
//  m_ControlPnts[c_size-2] = ip_vec[ip_size-1] + data_vec[ip_size-1].m_Tan1*(mag*m_Tension);
//  m_ControlPnts[c_size-1] = ip_vec[ip_size-1];
//}

//===== Compute Tangent Directions Using Flags ====//
//void VspCurve::ComputeTanDir( vector< vec3d > & pnt_vec, vector< VspPntData > & data_vec, bool closed_flag )
//{
//  int size = (int)pnt_vec.size();
//  vec3d tan, tan1, tan2;
//  for ( int i = 0 ; i < size ; i++ )
//  {
//      if ( i == 0 || i == size-1 )
//      {
//          if ( closed_flag )  tan = pnt_vec[1] - pnt_vec[size-2];
//          else if ( i == 0 )  tan = pnt_vec[1] - pnt_vec[0];
//          else                tan = pnt_vec[size-1] - pnt_vec[size-2];
//      }
//      else
//      {
//          tan = pnt_vec[i+1] - pnt_vec[i-1];
//      }
//
//      if ( data_vec[i].m_Type == VspPntData::ZERO )               tan = vec3d(0,0,0);
//      else if ( data_vec[i].m_Type == VspPntData::ZERO_X )            tan.set_x(0.0);
//      else if ( data_vec[i].m_Type == VspPntData::ZERO_Y )            tan.set_y(0.0);
//      else if ( data_vec[i].m_Type == VspPntData::ZERO_Z )            tan.set_z(0.0);
//      else if ( data_vec[i].m_Type == VspPntData::ONLY_BACK && i > 0 )
//          tan = pnt_vec[i] - pnt_vec[i-1];
//      else if ( data_vec[i].m_Type == VspPntData::ONLY_FORWARD && i < size-1 )
//          tan = pnt_vec[i+1] - pnt_vec[i];
//      else if ( data_vec[i].m_Type == VspPntData::PREDICT  )
//      {
//          tan.normalize();
//          vec3d predict_dir = tan;
//          if ( i == 0 )
//              predict_dir = pnt_vec[2] - pnt_vec[0];
//          else if ( i == size-1 )
//              predict_dir = pnt_vec[size-1] - pnt_vec[size-3];
//          predict_dir.normalize();
//          tan = tan*2.0 - predict_dir;
//      }
//      tan1 =  tan*-1.0;
//      tan2 =  tan;
//
//      if ( data_vec[i].m_Type == VspPntData::SHARP && i > 0 && i < size-1 )
//      {
//          tan1 = pnt_vec[i-1] - pnt_vec[i];
//          tan2 = pnt_vec[i+1] - pnt_vec[i];
//      }
//
//      tan1.normalize();
//      tan2.normalize();
//
//      if ( !data_vec[i].m_UseTan1 )
//          data_vec[i].m_Tan1 = tan1;
//
//      if ( !data_vec[i].m_UseTan2 )
//          data_vec[i].m_Tan2 = tan2;
//  }
//}

//===== Get Number Of Sections =====//
int VspCurve::GetNumSections() const
{
    return m_Curve.number_segments();
}

const piecewise_curve_type & VspCurve::GetCurve() const
{
    return m_Curve;
}

void VspCurve::SetCurve( const piecewise_curve_type &c )
{
    m_Curve = c;
}

void VspCurve::GetCurveSegment( curve_segment_type &c, int i ) const
{
    if ( i < GetNumSections() )
    {
        m_Curve.get( c, i );
    }
}

double VspCurve::GetCurveDt( int i ) const
{
    double dt( -1 );

    if ( i < GetNumSections() )
    {
        curve_segment_type c;

        m_Curve.get( c, dt, i );
    }

    return dt;
}

void VspCurve::AppendCurveSegment( curve_segment_type &c )
{
    m_Curve.push_back( c, 1 );
}

double VspCurve::FindDistant( double &u, const vec3d &pt, const double &d, const double &u0 ) const
{
    double dist;
    curve_point_type p;
    p << pt.x(), pt.y(), pt.z();

    dist = eli::geom::intersect::specified_distance( u, m_Curve, p, d, u0 );

    return dist;
}

double VspCurve::FindThickness( double &u1, double &u2, const vec3d &pt, const double &thick, const double &u10, const double &u20 ) const
{
    double dist;
    curve_point_type p;
    p << pt.x(), pt.y(), pt.z();

    dist = eli::geom::intersect::specified_thickness( u1, u2, m_Curve, p, thick, u10, u20 );

    return dist;
}

double VspCurve::FindNearest( double &u, const vec3d &pt ) const
{
    double dist;
    curve_point_type p;
    p << pt.x(), pt.y(), pt.z();

    dist = eli::geom::intersect::minimum_distance( u, m_Curve, p );

    return dist;
}

double VspCurve::FindNearest( double &u, const vec3d &pt, const double &u0 ) const
{
    double dist;
    curve_point_type p;
    p << pt.x(), pt.y(), pt.z();

    dist = eli::geom::intersect::minimum_distance( u, m_Curve, p, u0 );

    return dist;
}

double VspCurve::FindNearest01( double &u, const vec3d &pt ) const
{
    double dist;

    dist = FindNearest( u, pt );

    u = u / m_Curve.get_tmax();

    return dist;
}

double VspCurve::FindNearest01( double &u, const vec3d &pt, const double &u0 ) const
{
    double dist;

    dist = FindNearest( u, pt, u0 * m_Curve.get_tmax() );

    u = u / m_Curve.get_tmax();

    return dist;
}

//===== Compute Point  =====//
vec3d VspCurve::CompPnt( double u )
{
    vec3d rtn;
    curve_point_type v( m_Curve.f( u ) );

    rtn.set_xyz( v.x(), v.y(), v.z() );
    return rtn;
}

//===== Compute Tangent  =====//
vec3d VspCurve::CompTan( double u )
{
    vec3d rtn;
    curve_point_type v( m_Curve.fp( u ) );

    rtn.set_xyz( v.x(), v.y(), v.z() );
    return rtn;
}

//===== Compute Point U 0.0 -> 1.0 =====//
vec3d VspCurve::CompPnt01( double u )
{
    return CompPnt( u * m_Curve.get_tmax() );
}


//===== Compute Tan U 0.0 -> 1.0 =====//
vec3d VspCurve::CompTan01( double u )
{
    return CompTan( u * m_Curve.get_tmax() );
}

//===== Compute Length =====//
double VspCurve::CompLength( double tol )
{
    double len;
    eli::geom::curve::length( len, m_Curve, tol );

    return len;
}

//===== Tesselate =====//
void VspCurve::Tesselate( int num_pnts_u, vector< vec3d > & output )
{
    vector< double > uout;
    Tesselate( num_pnts_u, output, uout );
}

//===== Tesselate =====//
void VspCurve::Tesselate( int num_pnts_u, vector< vec3d > & output, vector< double > &uout )
{
    Tesselate( num_pnts_u, m_Curve.get_parameter_min(), m_Curve.get_parameter_max(), output, uout );
}

//===== Tesselate =====//
void VspCurve::TesselateNoCorner( int num_pnts_u, double umin, double umax, vector< vec3d > & output, vector< double > &uout )
{
    curve_index_type i;
    curve_point_type p;
    double delta;

    delta = ( umax - umin ) / ( num_pnts_u - 1 );

    uout.resize( num_pnts_u );
    for ( i = 0; i < num_pnts_u; ++i )
    {
        double u = umin + delta * i;
        uout[i] = u;
    }

    Tesselate( uout, output );
}

//===== Tesselate =====//
void VspCurve::Tesselate( int num_pnts_u, double umin, double umax, vector< vec3d > & output, vector< double > &uout )
{
    curve_index_type i;
    curve_point_type p;
    double delta;

    delta = ( umax - umin ) / ( num_pnts_u - 1 );

    uout.resize( num_pnts_u + 2 );
    uout[0] = umin;
    uout[1] = umin + TMAGIC;
    for ( i = 2; i < num_pnts_u + 1; ++i )
    {
        double u = umin + delta * ( i - 1 );
        uout[i] = u;
    }
    uout[ num_pnts_u ] = umax - TMAGIC;
    uout[ num_pnts_u + 1 ] = umax;

    Tesselate( uout, output );
}

void VspCurve::Tesselate( const vector< double > &u, vector< vec3d > & output )
{
    int num_pnts_u = u.size();
    curve_index_type i;
    curve_point_type p;

    output.resize( num_pnts_u );

    for ( i = 0; i < num_pnts_u; ++i )
    {
        p = m_Curve.f( u[i] );
        output[i].set_xyz( p.x(), p.y(), p.z() );
    }
}

//===== Offset =====//
void VspCurve::Offset( vec3d offvec )
{
    curve_point_type tr;
    tr << offvec.x(), offvec.y(), offvec.z();

    m_Curve.translate( tr );
}


//===== Offset X =====//
void VspCurve::OffsetX( double x )
{
    vec3d offvec( x, 0, 0 );
    Offset( offvec );
}

//===== Offset Y =====//
void VspCurve::OffsetY( double y )
{
    vec3d offvec( 0, y, 0 );
    Offset( offvec );
}

//===== Offset Z =====//
void VspCurve::OffsetZ( double z )
{
    vec3d offvec( 0, 0, z );
    Offset( offvec );
}

//===== Rotate About X-Axis  =====//
void VspCurve::RotateX( double ang )
{
    double cosang = cos( ang );
    double sinang = sin( ang );

    curve_rotation_matrix_type rot;
    rot <<  1, 0,      0,
        0, cosang, sinang,
        0, -sinang, cosang;

    m_Curve.rotate( rot );
}

//===== Rotate About Y-Axis  =====//
void VspCurve::RotateY( double ang )
{
    double cosang = cos( ang );
    double sinang = sin( ang );

    curve_rotation_matrix_type rot;
    rot <<  cosang, 0, -sinang,
        0,      1, 0,
        sinang, 0, cosang;
    m_Curve.rotate( rot );
}
//===== Rotate About Z-Axis  =====//
void VspCurve::RotateZ( double ang )
{
    double cosang = cos( ang );
    double sinang = sin( ang );

    curve_rotation_matrix_type rot;
    rot <<  cosang, sinang, 0,
        -sinang, cosang, 0,
        0,      0,      1;
    m_Curve.rotate( rot );
}

//==== Transform Control Points =====//
void VspCurve::Transform( Matrix4d & mat )
{
    curve_rotation_matrix_type rmat;
    double *mmat( mat.data() );
    curve_point_type trans;

    rmat << mmat[0], mmat[4], mmat[8],
         mmat[1], mmat[5], mmat[9],
         mmat[2], mmat[6], mmat[10];
    trans << mmat[12], mmat[13], mmat[14];

    m_Curve.rotate( rmat );
    m_Curve.translate( trans );
}

void VspCurve::ReflectXY()
{
    m_Curve.reflect_xy();
}

void VspCurve::ReflectXZ()
{
    m_Curve.reflect_xz();
}

void VspCurve::ReflectYZ()
{
    m_Curve.reflect_yz();
}

void VspCurve::Reflect( vec3d axis )
{
    curve_point_type a;

    a << axis.x(), axis.y(), axis.z();
    m_Curve.reflect( a );
}

void VspCurve::Reflect( vec3d axis, double d )
{
    curve_point_type a;

    a << axis.x(), axis.y(), axis.z();
    m_Curve.reflect( a, d );
}

void VspCurve::Reverse()
{
    m_Curve.reverse();
}


bool VspCurve::IsEqual( const VspCurve & crv )
{
    int ns0 = m_Curve.number_segments();
    int ns1 = crv.m_Curve.number_segments();

    if ( ns0 != ns1 )
        return false;

    // get control points and print
    int i, pp;

    for ( pp=0 ; pp < ns0 ; ++pp )
    {
      curve_segment_type bez0;
      curve_segment_type bez1;

      m_Curve.get(bez0, pp);
      crv.m_Curve.get(bez1, pp);

      if ( bez0.degree() != bez1.degree() )
          return false;

      for (i=0; i<=bez0.degree(); ++i)
      {
          curve_point_type cp0 = bez0.get_control_point(i);
          curve_point_type cp1 = bez1.get_control_point(i);
          vec3d v0( cp0.x(), cp0.y(), cp0.z() );
          vec3d v1( cp1.x(), cp1.y(), cp1.z() );

          if ( dist( v0, v1 ) > 1.0e-12 )
              return false;
      }
    }

    return true;

}

void VspCurve::GetBoundingBox( BndBox &bb ) const
{
	curve_bounding_box_type bbx;
    vec3d v3min, v3max;

    m_Curve.get_bounding_box( bbx );
    v3min.set_xyz( bbx.get_min().x(), bbx.get_min().y(), bbx.get_min().z() );
    v3max.set_xyz( bbx.get_max().x(), bbx.get_max().y(), bbx.get_max().z() );
    bb.Reset();
    bb.Update( v3min );
    bb.Update( v3max );
}
