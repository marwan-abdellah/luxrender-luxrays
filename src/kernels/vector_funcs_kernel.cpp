#include <string>
namespace luxrays { namespace ocl {
std::string KernelSource_vector_funcs = 
"#line 2 \"vector_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *\n"
" *                                                                         *\n"
" *   This file is part of LuxRays.                                         *\n"
" *                                                                         *\n"
" *   LuxRays is free software; you can redistribute it and/or modify       *\n"
" *   it under the terms of the GNU General Public License as published by  *\n"
" *   the Free Software Foundation; either version 3 of the License, or     *\n"
" *   (at your option) any later version.                                   *\n"
" *                                                                         *\n"
" *   LuxRays is distributed in the hope that it will be useful,            *\n"
" *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *\n"
" *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *\n"
" *   GNU General Public License for more details.                          *\n"
" *                                                                         *\n"
" *   You should have received a copy of the GNU General Public License     *\n"
" *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *\n"
" *                                                                         *\n"
" *   LuxRays website: http://www.luxrender.net                             *\n"
" ***************************************************************************/\n"
"\n"
"float SphericalTheta(const float3 v) {\n"
"	return acos(clamp(v.z, -1.f, 1.f));\n"
"}\n"
"\n"
"float SphericalPhi(const float3 v) {\n"
"	float p = atan2(v.y, v.x);\n"
"	return (p < 0.f) ? p + 2.f * M_PI_F : p;\n"
"}\n"
"\n"
"void CoordinateSystem(const float3 v1, float3 *v2, float3 *v3) {\n"
"	if (fabs(v1.x) > fabs(v1.y)) {\n"
"		float invLen = 1.f / sqrt(v1.x * v1.x + v1.z * v1.z);\n"
"		(*v2).x = -v1.z * invLen;\n"
"		(*v2).y = 0.f;\n"
"		(*v2).z = v1.x * invLen;\n"
"	} else {\n"
"		float invLen = 1.f / sqrt(v1.y * v1.y + v1.z * v1.z);\n"
"		(*v2).x = 0.f;\n"
"		(*v2).y = v1.z * invLen;\n"
"		(*v2).z = -v1.y * invLen;\n"
"	}\n"
"\n"
"	*v3 = cross(v1, *v2);\n"
"}\n"
; } }
