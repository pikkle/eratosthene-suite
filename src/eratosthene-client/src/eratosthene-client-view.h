/*
 *  eratosthene-suite - client
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Copyright (c) 2016-2020 DHLAB, EPFL
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

    /*! \file   eratosthene-client-view.h
     *  \author Nils Hamel <nils.hamel@bluewin.ch>
     *
     *  eratosthene-suite - client - view
     */

/*
    header - inclusion guard
 */

    # ifndef __ER_CLIENT_VIEW__
    # define __ER_CLIENT_VIEW__

/*
    header - C/C++ compatibility
 */

    # ifdef __cplusplus
    extern "C" {
    # endif

/*
    header - internal includes
 */

    # include "eratosthene-client-common.h"

/*
    header - external includes
 */

/*
    header - preprocessor definitions
 */

    /* define pseudo-constructor */
    # define ER_VIEW_C { 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, 0, ER_COMMON_DSPAN }

    /* define pseudo-constructor - default point of view */
    //# define ER_VIEW_D { 0.0, 0.0, LE_ADDRESS_WGS_A * 2.0, 0.0, 0.0, 1, 0, 1117584000, 1117584000, ER_COMMON_DCOMB, ER_COMMON_DSPAN }
    # define ER_VIEW_D { 7.452495, 46.945290, LE_ADDRESS_WGS_A * 1.01, 0.0, 0.0, 1, 0, 1117584000, 1117584000, ER_COMMON_DCOMB, ER_COMMON_DSPAN }

/*
    header - preprocessor macros
 */

/*
    header - type definition
 */

/*
    header - structures
 */

    /*! \struct er_view_struct
     *  \brief View structure
     *
     *  This structure contains the information describing as user point of view
     *  in the four-dimensional space. This includes the spatial position, the
     *  orientation of the view and elements related to the position in time.
     *
     *  The position and orientation of the point of view is stored through the
     *  five first fields that gives the longitude, the latitude, the radial
     *  altitude (from Earth center) and the two angles of sight.
     *
     *  The two next fields are related to the convolution and query mode. As
     *  the interface allows to browse through two different times, the mode of
     *  convolution describe how to handle them. The remote server allows the
     *  following convolution modes :
     *
     *      mode = 1 : Primary time only
     *      mode = 2 : Secondary time only
     *      mode = 3 : Primary time (logical or ) secondary time
     *      mode = 4 : Primary time (logical and) secondary time
     *      mode = 5 : Primary time (logical xor) secondary time
     *
     *  The query mode allows to specify to the remote server how the address
     *  content detection is performed along the time dimension. The following
     *  values are available :
     *
     *      query = 0 (LE_ADDRESS_NEAR)
     *      query = 1 (LE_ADDRESS_DEEP)
     *
     *  The near mode indicates the server to detects only the nearest storage
     *  structure of the data without checking if a specific cell contains
     *  actual data in it. The deep mode indicates the server to search through
     *  all storage structure (following a proximity order) until the detection
     *  of data for the cell pointed by the considered address index. Both
     *  detection mode are clamped by the comb value stored in \b vw_cmb field.
     *
     *  The two times are then part of the point of view and their value is kept
     *  in this structure through two fields. In addition to their value, a
     *  temporal range is also kept by the structure. This time range indicates
     *  the remote server to which extent element have to be considered
     *  according to the position in time. This allows to reduce or increase the
     *  view span in the time dimension.
     *
     *  The last field holds the value of the model cells query additional depth
     *  (span). This allows user to modulate the density of the displayed model
     *  by simply changing this value.
     *
     *  \var er_view_struct::vw_lon
     *  Longitude angle, in decimal degrees
     *  \var er_view_struct::vw_lat
     *  Latitude angle, in decimal degrees
     *  \var er_view_struct::vw_alt
     *  Height above WGS84 ellipsoid, in metres
     *  \var er_view_struct::vw_azm
     *  Azimuth angle, in decimal degrees
     *  \var er_view_struct::vw_gam
     *  Tilt (gamma) angle, in decimal degrees
     *  \var er_view_struct::vw_mod
     *  Times convolution mode
     *  \var er_view_struct::vw_qry
     *  Temporal query mode
     *  \var er_view_struct::vw_tia
     *  Primary time value
     *  \var er_view_struct::vw_tib
     *  Secondary time value
     *  \var er_view_struct::vw_cmb
     *  Temporal range (comb)
     *  \var er_view_struct::vw_spn
     *  Cells query additional depth
     */

    typedef struct er_view_struct {

        le_real_t vw_lon;
        le_real_t vw_lat;
        le_real_t vw_alt;
        le_real_t vw_azm;
        le_real_t vw_gam;

        le_enum_t vw_mod;
        le_enum_t vw_qry;
        le_time_t vw_tia;
        le_time_t vw_tib;
        le_time_t vw_cmb;

        le_size_t vw_spn;

    } er_view_t;


/*
    header - function prototypes
 */

    /*! \brief constructor/destructor methods
     *
     *  This function creates and returns a view structure. It initialises the
     *  structure fields using default values and returns it.
     *
     *  \return Returns the created view structure
     */

    er_view_t er_view_create( le_void_t );

    /*! \brief constructor/destructor methods
     *
     *  This function deletes the view structure provided as parameter. It
     *  simply clears the structure fields using default values.
     *
     *  \param er_view View structure
     */

    le_void_t er_view_delete( er_view_t * const er_view );

    /*! \brief accessor methods
     *
     *  This function compares the content of the two provided view structures
     *  and checks for the identity of the fields used to trigger a model update
     *  procedure (longitude, latitude, altitude, convolution and query modes,
     *  times and range).
     *
     *  If all the considered fields are identical, the function returns the
     *  \b _LE_TRUE value, _LE_FALSE otherwise. This function is used to detect
     *  motion of the point of view.
     *
     *  \param er_viewa View structure
     *  \param er_viewb View structure
     *
     *  \return Returns _LE_TRUE on pseudo-identity, _LE_FALSE otherwise
     */

    le_enum_t er_view_get_equal( er_view_t const * const er_viewa, er_view_t const * const er_viewb );

    /*! \brief accessor methods
     *
     *  This function returns the longitude value contained in the provided view
     *  structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view longitude, in decimal degrees
     */

    le_real_t er_view_get_lon( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the latitude value contained in the provided view
     *  structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view latitude, in decimal degrees
     */

    le_real_t er_view_get_lat( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the radial altitude value contained in the
     *  provided view structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view height above WGS84 ellipsoid, in metres
     */

    le_real_t er_view_get_alt( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the azimuth angle value contained in the view
     *  structure provided as parameter.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view azimuth angle, in decimal degrees
     */

    le_real_t er_view_get_azm( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the tilt (gamma) angle value contained in the view
     *  structure provided as parameter.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view tilt (gamma) angle, in decimal degrees
     */

    le_real_t er_view_get_gam( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function copies the point of view position of the provided view
     *  structure to the provided position array. In addition to the copy, it
     *  also converts the two parametric angles from degrees to radian.
     *
     *  \param er_view View structure
     *  \param er_pose Position array
     */

    le_void_t er_view_get_pose( er_view_t const * const er_view, le_real_t * const er_pose );

    /*! \brief accessor methods
     *
     *  This function computes and return the inertia factor that is used to
     *  modulate the point of view motion. In addition to the position
     *  coordinates, the function also checks the keyboard modifiers to adapt
     *  the inertia value.
     *
     *  As the point of view get closer to the ground surface, the motion has
     *  to be reduced in order to keep relevant displacement speed. In addition,
     *  if left-control key is pressed, the inertia is augmented to speed up
     *  the point of view. If the left-shift key is pressed, the inertia is
     *  decreased to allow fine tuning of the point of view.
     *
     *  \param er_view     View structure
     *  \param er_modifier Keyboard modifier bit-field
     *
     *  \return Returns the computed inertia value
     */

    le_real_t er_view_get_inertia( er_view_t const * const er_view, le_enum_t const er_modifier );

    /*! \brief accessor methods
     *
     *  This function returns the times convolution mode value contained in the
     *  provided view structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view times convolution mode
     */

    le_enum_t er_view_get_mode( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the cells query mode value contained in the
     *  provided view structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns the view cells query mode
     */

    le_enum_t er_view_get_query( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function creates and returns an address structure initialised using
     *  the provided view structure.
     *
     *  The two times of the view structure are respectively copied in the
     *  address structure depending on the structure times convolution mode. The
     *  first time is copied as the mode value is different of 2 (second time
     *  only). The second time is copied as the mode value is not 1 (first time
     *  only).
     *
     *  In addition, the function also sets the address additional depth (span)
     *  using the view structure corresponding value. The value of the temporal
     *  range is also packed in the provided address structure.
     *
     *  The time convolution and cells query modes are also assigned to the
     *  returned address structure.
     *
     *  This function is usually used by the model update process to initialise
     *  the enumeration address structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns initialised address structure
     */

    le_address_t er_view_get_times( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This structure returns the time value contained in the provided view
     *  structure. According to the provided index, the function returns the
     *  first or the second time contained in the view structure.
     *
     *  \param er_view View structure
     *  \param er_time View structure time index
     *
     *  \return Returns view time
     */

    le_time_t er_view_get_time( er_view_t const * const er_view, le_enum_t const er_time );

    /*! \brief accessor methods
     *
     *  This function returns the temporal range value stored in the provided
     *  view structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns view structure temporal range
     */

    le_time_t er_view_get_comb( er_view_t const * const er_view );

    /*! \brief accessor methods
     *
     *  This function returns the span value of the provided view structure.
     *
     *  \param er_view View structure
     *
     *  \return Returns view structure span value
     */

    le_size_t er_view_get_span( er_view_t const * const er_view );

    /*! \brief mutator methods
     *
     *  This function allows to set the spatial and temporal position of the
     *  provided view structure using the provided parameters.
     *
     *  \param er_view View structure
     *  \param er_lon  Longitude value (in decimal degree)
     *  \param er_lat  Latitude value (in decimal degree)
     *  \param er_alt  Altitude value (in metres)
     *  \param er_azm  Azimuth angle value (in decimal degree)
     *  \param er_gam  Tilt angle value (in decimal degree)
     *  \param er_tia  First time value (in seconds)
     *  \param er_tib  Second time value (in seconds)
     *  \param er_cmb  Temporal comb value (in seconds)
     */

    le_void_t er_view_set( er_view_t * const er_view, le_real_t const er_lon, le_real_t const er_lat, le_real_t const er_alt, le_real_t const er_azm, le_real_t const er_gam, le_time_t const er_tia, le_time_t const er_tib, le_time_t const er_cmb );

    /*! \brief mutator methods
     *
     *  This function updates the position of the point of view contained in the
     *  provided view structure. It uses the provided x and y motion value that
     *  are usually linked to mouse motion on the user screen.
     *
     *  It uses the x motion amplitude value to move the point of view along the
     *  direction defined by the azimuth angle. The y motion amplitude is used
     *  to move the point of view along the perpendicular direction to the
     *  azimuth direction.
     *
     *  \param er_view   View structure
     *  \param er_xvalue Motion amplitude (pro-grade)
     *  \param er_yvalue Motion amplitude (ortho-grade)
     */

    le_void_t er_view_set_plan( er_view_t * const er_view, le_real_t const er_xvalue, le_real_t const er_yvalue );

    /*! \brief mutator methods
     *
     *  This function adds the provided motion amplitude value to the radial
     *  altitude of the view structure provided as parameter.
     *
     *  After height modification, the function checks its value and correct it
     *  as it goes beyond the authorised range.
     *
     *  \param er_view  View structure
     *  \param er_value Motion amplitude
     */

    le_void_t er_view_set_alt( er_view_t * const er_view, le_real_t const er_value );

    /*! \brief mutator methods
     *
     *  This function adds the provided motion amplitude value to the azimuth
     *  angle of the provided view structure.
     *
     *  \param er_view  View structure
     *  \param er_value Motion amplitude
     */

    le_void_t er_view_set_azm( er_view_t * const er_view, le_real_t const er_value );

    /*! \brief mutator methods
     *
     *  This function adds the provided motion amplitude value to the tilt angle
     *  of the provided view structure.
     *
     *  In addition, the function checks the updated value of the angle and
     *  correct it as it goes beyond the authorised range.
     *
     *  \param er_view  View structure
     *  \param er_value Motion amplitude
     */

    le_void_t er_view_set_gam( er_view_t * const er_view, le_real_t const er_value );

    /*! \brief mutator methods
     *
     *  This function sets the times convolution mode of the provided view
     *  structure.
     *
     *  \param er_view View structure
     *  \param er_mode Times convolution mode
     */

    le_void_t er_view_set_mode( er_view_t * const er_view, le_enum_t const er_mode );

    /*! \brief mutator methods
     *
     *  This function sets the cells query mode of the provided view structure.
     *
     *  \param er_view  View structure
     *  \param er_query Cells query mode
     */

    le_void_t er_view_set_query( er_view_t * const er_view, le_enum_t const er_query );

    /*! \brief mutator methods
     *
     *  This function is used for alignment of the two times of the provided
     *  view structure.
     *
     *  If the mode is one, the second time is aligned on the first one. If the
     *  mode is two, the first time is aligned on the second one.
     *
     *  \param er_view View structure
     */

    le_void_t er_view_set_times( er_view_t * const er_view );

    /*! \brief mutator methods
     *
     *  This function updates the value of the provided view structure of the
     *  primary or secondary time depending on the structure mode value.
     *
     *  Considering the active time, the function uses the provided value to
     *  change the position in time. The temporal range of the provided view
     *  structure is also used to adapt the temporal motion speed.
     *
     *  \param er_view  View structure
     *  \param er_value Motion amplitude and direction (through its sign)
     */

    le_void_t er_view_set_time( er_view_t * const er_view, le_real_t const er_value );

    /*! \brief mutator methods
     *
     *  This function updates the temporal range value of the provided view
     *  structure using the provided value.
     *
     *  The provided value, in floating point format, is multiplied with the
     *  temporal range value before clamping the result. The clamped value is
     *  then assigned as the new temporal range.
     *
     *  \param er_view  View structure
     *  \param er_value Modification factor
     */

    le_void_t er_view_set_comb( er_view_t * const er_view, le_real_t er_value );

    /*! \brief mutator methods
     *
     *  This function updates the span value of the provided view structure. It
     *  adds the provided value, that can be negative or positive, to the span
     *  value of the view structure before to clamp the result in the predefined
     *  range.
     *
     *  \param er_view  View structure
     *  \param er_value Span modification value
     */

    le_void_t er_view_set_span( er_view_t * const er_view, le_size_t const er_value );

/*
    header - C/C++ compatibility
 */

    # ifdef __cplusplus
    }
    # endif

/*
    header - inclusion guard
 */

    # endif

