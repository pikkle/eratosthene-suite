/*
 *  eratosthene-suite - address
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Copyright (c) 2016-2018 DHLAB, EPFL
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

    /*! \file   eratosthene-address.h
     *  \author Nils Hamel <nils.hamel@bluewin.ch>
     *
     *  eratosthene-suite - address
     */

    /*! \mainpage eratosthene-suite
     *
     *  \section overview Overview
     *
     *  The _eratosthene-suite_ offers a front-end to the eratosthene indexation
     *  server implemented in the _liberatosthene_ library. It consists in a
     *  softwares suite offering front-end solutions to the functionalities of
     *  the eratosthene library.
     *
     *  The principal software implements the server itself allowing to simply
     *  create instances and maintain services. The suite also offers a
     *  front-end software for the data injection in the available servers. It
     *  also offers a graphical software allowing to browse the 4D worlds
     *  offered by the available servers.
     *
     *  Other tools are also available as the isometric projection creation and
     *  the address and time conversion softwares.
     *
     *  \section project Eratosthene Project
     *
     *  The Eratosthene Project was initially imagined to answer the question of
     *  very large point-based models of cities and environments. Indeed, mapping
     *  a whole city at the level of the centimetre induces very large and heavy
     *  models that are difficult to manipulate, explore and exploit. Moreover,
     *  considering the time dimension implies a quick increase of the size of
     *  the models, making them almost impossible to handle as a single object.
     *
     *  The Eratosthene Project is designed to answer this question by
     *  implementing an space-time indexation formalism able to handle, store
     *  and access very large 4D models of cities and environments. The
     *  indexation formalism is used to implement a server able to manage,
     *  store and broadcast very large 4D models distributed all over the earth
     *  surface and covering large period of time. In addition to the server,
     *  the Eratosthene Project also implements a graphical client able to
     *  communicate with the server and to offer a convenient way of browsing
     *  the models in both space and time.
     *
     *  The space-time indexation formalism is designed to efficiently manage
     *  the storage of the incoming 4D models and to offer and simple and
     *  efficient ways of addressing queries to the server allowing models
     *  broadcasting. The space-time indexation also allows to perform queries
     *  to the server in a intuitive way giving the possibility to both address
     *  the questions of the earth structures levels of detail and time
     *  management. This allows to explore the entire earth surface in terms of
     *  4D structures from large mountain chains in 2000 to very small details
     *  of cities in 1850.
     *
     *  \section license Copyright and License
     *
     *  **eratosthene-suite** - Nils Hamel <br >
     *  Copyright (c) 2016-2018 DHLAB, EPFL
     *
     *  This program is licensed under the terms of the GNU GPLv3.
     */


/*
    header - inclusion guard
 */

    # ifndef __ER_ADDRESS__
    # define __ER_ADDRESS__

/*
    header - C/C++ compatibility
 */

    # ifdef __cplusplus
    extern "C" {
    # endif

/*
    header - internal includes
 */

/*
    header - external includes
 */

    # include <stdio.h>
    # include <stdlib.h>
    # include <common-include.h>
    # include <eratosthene-include.h>

/*
    header - preprocessor definitions
 */

    /* trigonometric conversion factors */
    # define ER_ADDRESS_D2R ( LE_PI / 180.0 )
    # define ER_ADDRESS_R2D ( 180.0 / LE_PI )

    /* display format specifier */
    # define ER_ADDRESS_P   "%" _LE_REAL_P ", %" _LE_REAL_P ", %" _LE_REAL_P "\n"

/*
    header - preprocessor macros
 */

/*
    header - type definition
 */

/*
    header - structures
 */

/*
    header - function prototypes
 */

    /*! \brief space conversion methods
     *
     *  This function, that expects the standard main function parameters, reads
     *  the provided geographic coordinates and the scale. It then computes the
     *  spatial index obtained using the geographic coordinates. Considering the
     *  provided scale, it computes the geographic coordinates back from the
     *  spatial index.
     *
     *  It then displays on the standard output the original coordinates, the
     *  computed spatial index and the coordinates obtained by converting back
     *  the spatial index.
     *
     *  The provided coordinates have to be given in decimal degrees in the
     *  ranges : [-180,+180], [-90,+90]. The heights have to be given in metres
     *  above the WGS84 ellipsoid.
     *
     *  \param argc Main function parameters
     *  \param argv Main function parameters
     */

    le_void_t er_address_space( int argc, char ** argv );

    /*! \brief time conversion methods
     *
     *  This function, that expects the standard main function parameters, reads
     *  the provided date and converts it into a UNIX timestamps. After the
     *  conversion, the function displays the result on the standard output.
     *
     *  The date has to be provided following the pattern : YYYY-MM-DD-hh-mm-ss.
     *
     *  \param argc Main function parameters
     *  \param argv Main function parameters
     */

    le_void_t er_address_time( int argc, char ** argv );

    /*! \brief main function
     *
     *  The main function searches in the arguments and parameters the switches
     *  that are used to determine the execution mode :
     *
     *      ./*-address --time/-t, --space/-s [execution switch]
     *                  --date/-d [date specification]
     *                  --scale/-s [index scale specification]
     *                  --longitude/-l
     *                  --latitude/-a
     *                  --altitude/-h [geographic coordinates specification]
     *
     *  If the '--space' switch is found, the main function invokes the spatial
     *  index computation function. It expects the 'scale' parameters and a
     *  geographic position through its coordinates.
     *
     *  If the '--time' switch is found, the main function invokes the date
     *  conversion function. It expects a date specification according to the
     *  specialised function format.
     *
     *  \param argc Main function parameters
     *  \param argv Main function parameters
     *
     *  \return Standard exit code
     */

    int main( int argc, char ** argv );

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

