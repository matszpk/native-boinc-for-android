/* Copyright 2010 Matthew Arsenault, Travis Desell, Dave Przybylo,
Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
Magdon-Ismail and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SEPARATION_CONFIG_H_
#define _SEPARATION_CONFIG_H_

#include "milkyway_config.h"

#define SEPARATION_VERSION_MAJOR 0
#define SEPARATION_VERSION_MINOR 88
#define SEPARATION_VERSION       0.88

#define SEPARATION_CRLIBM 0
#define SEPARATION_FDLIBM 0
#define SEPARATION_OPENCL 0
#define SEPARATION_CAL 0

#define NVIDIA_OPENCL 1
#define AMD_OPENCL 0

#define SEPARATION_INLINE_KERNEL 0
#define SEPARATION_GRAPHICS 0




#define ENABLE_CRLIBM SEPARATION_CRLIBM
#define ENABLE_FDLIBM SEPARATION_FDLIBM

#define ENABLE_OPENCL SEPARATION_OPENCL


#if SEPARATION_OPENCL
  #define SEPARATION_SPECIAL_STR " OpenCL"
#elif SEPARATION_CAL
  #define SEPARATION_SPECIAL_STR " CAL++"
#else
  #define SEPARATION_SPECIAL_STR ""
#endif

#if SEPARATION_CRLIBM
  #define SEPARATION_SPECIAL_LIBM_STR " crlibm"
#elif SEPARATION_FDLIBM
  #define SEPARATION_SPECIAL_LIBM_STR " fdlibm"
#elif SEPARATION_USE_CUSTOM_MATH
   #define SEPARATION_SPECIAL_LIBM_STR " custom"
#else
  #define SEPARATION_SPECIAL_LIBM_STR ""
#endif /* SEPARATION_CRLIBM */

#define SEPARATION_SYSTEM_NAME "Linux"
#define SEPARATION_APP_NAME "milkywayathome_client separation"


#endif /* _SEPARATION_CONFIG_H_ */

