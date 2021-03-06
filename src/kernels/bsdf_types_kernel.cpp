#include <string>
namespace luxrays { namespace ocl {
std::string KernelSource_bsdf_types = 
"#line 2 \"bsdf_types.cl\"\n"
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
"typedef enum {\n"
"	NONE     = 0,\n"
"	DIFFUSE  = 1,\n"
"	GLOSSY   = 2,\n"
"	SPECULAR = 4,\n"
"	REFLECT  = 8,\n"
"	TRANSMIT = 16\n"
"} BSDFEventType;\n"
"\n"
"typedef int BSDFEvent;\n"
"\n"
"typedef struct {\n"
"	// The incoming direction. It is the eyeDir when fromLight = false and\n"
"	// lightDir when fromLight = true\n"
"	Vector fixedDir;\n"
"	Point hitPoint;\n"
"	UV hitPointUV;\n"
"	Normal geometryN;\n"
"	Normal shadeN;\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	// passThroughEvent can be stored here in a path state even before to of\n"
"	// BSDF initialization (while tracing the next path vertex ray)\n"
"	float passThroughEvent;\n"
"#endif\n"
"	unsigned int materialIndex;\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"	unsigned int triangleLightSourceIndex;\n"
"#endif\n"
"\n"
"	Frame frame;\n"
"\n"
"	// This will be used for BiDir\n"
"	//bool fromLight;\n"
"} BSDF;\n"
; } }
