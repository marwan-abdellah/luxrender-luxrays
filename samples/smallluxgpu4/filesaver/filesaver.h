/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef _FILESAVER_H
#define	_FILESAVER_H

#include "slg.h"
#include "renderengine.h"

#include "luxrays/utils/core/randomgen.h"
#include "luxrays/utils/sampler/sampler.h"
#include "luxrays/utils/film/film.h"
#include "luxrays/utils/sdl/bsdf.h"

namespace slg {

//------------------------------------------------------------------------------
// Scene FileSaver render engine
//------------------------------------------------------------------------------

class FileSaverRenderEngine : public RenderEngine {
public:
	FileSaverRenderEngine(RenderConfig *cfg, luxrays::utils::Film *flm, boost::mutex *flmMutex);

	RenderEngineType GetEngineType() const { return FILESAVER; }

protected:
	virtual void StartLockLess();
	virtual void StopLockLess() { }

	virtual void BeginEditLockLess() { }
	virtual void EndEditLockLess(const EditActionList &editActions) { SaveScene(); }

	virtual void UpdateFilmLockLess() { }
	virtual void UpdateCounters() { }

private:
	void SaveScene();

	std::string directoryName;
};

}

#endif	/* _FILESAVER_H */
