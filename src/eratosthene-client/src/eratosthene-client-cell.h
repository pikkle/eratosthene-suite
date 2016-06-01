/*
 *  eratosthene-suite - geodetic system
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Copyright (c) 2016 EPFL CDH DHLAB
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

    /*! \file   eratosthene-client-cell.h
     *  \author Nils Hamel <n.hamel@bluewin.ch>
     *
     *  Eratosthene client - Cell module
     */

/*
    header - inclusion guard
 */

    # ifndef __ER_CLIENT_CELL__
    # define __ER_CLIENT_CELL__

/*
    header - C/C++ compatibility
 */

    # ifdef __cplusplus
    extern "C" {
    # endif

/*
    header - includes
 */

    # include "eratosthene-client-geodesy.h"
    # include <math.h>
    # include <eratosthene-include.h>

/*
    header - preprocessor definitions
 */

    /* Define pseudo-constructor */
    # define ER_CELL_C      { ER_CELL_DOWN, { 0 }, { 0 }, LE_ADDRESS_C, 0, NULL, NULL, { 0.0 } }

    /* Define cell flags */
    # define ER_CELL_DOWN   0
    # define ER_CELL_PUSH   1

    /* Define cell array size */
    # define ER_CELL_ARRAY  786432

/*
    header - preprocessor macros
 */

/*
    header - type definition
 */

/*
    header - structures
 */

    typedef struct er_cell_struct {

        le_enum_t    ce_flag;

        le_char_t    ce_addr[LE_NETWORK_BUFFER_ADDR];
        le_char_t    ce_push[LE_NETWORK_BUFFER_ADDR];

        le_address_t ce_cell;

        le_size_t    ce_size;
        le_real_t *  ce_pose;
        le_data_t *  ce_data;

        le_real_t    ce_edge[3];
        
    } er_cell_t;

/*
    header - function prototypes
 */

    /*! \brief constructor/destructor methods
     */

    er_cell_t er_cell_create( le_void_t );

    /*! \brief constructor/destructor methods
     */

    le_void_t er_cell_delete( er_cell_t * const er_cell );

    /*! \brief accessor methods
     */

    le_enum_t er_cell_get_flag( er_cell_t const * const er_cell );

    /*! \brief accessor methods
     */

    le_enum_t er_cell_get_push( er_cell_t const * const er_cell );

    /*! \brief accessor methods
     */

    le_enum_t er_cell_get_match( er_cell_t const * const er_cell, er_cell_t const * const er_push );

    /*! \brief accessor methods
     */

    le_size_t er_cell_get_size( er_cell_t const * const er_cell );

    /*! \brief accessor methods
     */

    le_real_t * er_cell_get_pose( er_cell_t const * const er_cell );

    /*! \brief accessor methods
     */

    le_data_t * er_cell_get_data( er_cell_t const * const er_cell );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_flag( er_cell_t * const er_cell, le_enum_t const er_flag );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_addr( er_cell_t * const er_cell, le_address_t const * const er_address );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_push( er_cell_t * const er_cell, le_address_t const * const er_address );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_pop( er_cell_t * const er_cell );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_swap( er_cell_t * const er_addr, er_cell_t * const er_push );

    /*! \brief mutator methods
     */

    le_void_t er_cell_set_empty( er_cell_t * const er_cell );

    /*! \brief mutator methods
     *  Frame : x_geo -> z_ogl, y_geo -> x_ogl, z_geo -> y_ogl
     *  2->1
     *  1->0
     *  0->2
     */

    le_void_t er_cell_set_query( er_cell_t * const er_cell, le_sock_t const er_socket );

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

