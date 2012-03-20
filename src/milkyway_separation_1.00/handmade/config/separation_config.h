/*
 *  Copyright (c) 2008-2010 Travis Desell, Nathan Cole
 *  Copyright (c) 2008-2010 Boleslaw Szymanski, Heidi Newberg
 *  Copyright (c) 2008-2010 Carlos Varela, Malik Magdon-Ismail
 *  Copyright (c) 2008-2011 Rensselaer Polytechnic Institute
 *  Copyright (c) 2010-2011 Matthew Arsenault
 *
 *  This file is part of Milkway@Home.
 *
 *  Milkway@Home is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SEPARATION_CONFIG_H_
#define _SEPARATION_CONFIG_H_

#include "milkyway_config.h"

#define SEPARATION_VERSION_MAJOR 1
#define SEPARATION_VERSION_MINOR 00
#define SEPARATION_VERSION       1.00

#define SEPARATION_CRLIBM 0
#define SEPARATION_OPENCL 0

#define NVIDIA_OPENCL 0
#define AMD_OPENCL 0

#define SEPARATION_GRAPHICS 0

#define HAVE_SSE2 0
#define HAVE_SSE3 0
#define HAVE_SSE4 0
#define HAVE_SSE41 0
#define HAVE_AVX 0




#define ENABLE_CRLIBM SEPARATION_CRLIBM

#define ENABLE_OPENCL SEPARATION_OPENCL


#if SEPARATION_OPENCL
  #define SEPARATION_SPECIAL_STR " OpenCL"
#else
  #define SEPARATION_SPECIAL_STR ""
#endif

#if SEPARATION_CRLIBM
  #define SEPARATION_SPECIAL_LIBM_STR " crlibm"
#elif SEPARATION_USE_CUSTOM_MATH
   #define SEPARATION_SPECIAL_LIBM_STR " custom"
#else
  #define SEPARATION_SPECIAL_LIBM_STR ""
#endif /* SEPARATION_CRLIBM */

#define SEPARATION_PROJECT_NAME "milkyway_separation"


#endif /* _SEPARATION_CONFIG_H_ */

