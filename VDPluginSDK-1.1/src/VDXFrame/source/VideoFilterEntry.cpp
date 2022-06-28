//	VDXFrame - Helper library for VirtualDub plugins
//	Copyright (C) 2008 Avery Lee
//
//	The plugin headers in the VirtualDub plugin SDK are licensed differently
//	differently than VirtualDub and the Plugin SDK themselves.  This
//	particular file is thus licensed as follows (the "zlib" license):
//
//	This software is provided 'as-is', without any express or implied
//	warranty.  In no event will the authors be held liable for any
//	damages arising from the use of this software.
//
//	Permission is granted to anyone to use this software for any purpose,
//	including commercial applications, and to alter it and redistribute it
//	freely, subject to the following restrictions:
//
//	1.	The origin of this software must not be misrepresented; you must
//		not claim that you wrote the original software. If you use this
//		software in a product, an acknowledgment in the product
//		documentation would be appreciated but is not required.
//	2.	Altered source versions must be plainly marked as such, and must
//		not be misrepresented as being the original software.
//	3.	This notice may not be removed or altered from any source
//		distribution.

#include "stdafx.h"
#include <vd2/VDXFrame/VideoFilterEntry.h>

int g_VFVAPIVersion;
VDXFilterDefinition **g_VDXRegisteredFilters;
int g_VDXRegisteredFilterCount;

VDXFilterDefinition *VDXGetVideoFilterDefinition(int index);

int VDXVideoFilterModuleInit2(struct VDXFilterModule *fm, const VDXFilterFunctions *ff, int& vdfd_ver, int& vdfd_compat, int ver_compat_target) {
	int def_count = 0;

	while(VDXGetVideoFilterDefinition(def_count))
		++def_count;

	g_VDXRegisteredFilters = (VDXFilterDefinition **)malloc(sizeof(VDXFilterDefinition *) * def_count);
	if (!g_VDXRegisteredFilters)
		return 1;

	memset(g_VDXRegisteredFilters, 0, sizeof(VDXFilterDefinition *) * def_count);

	for(int i=0; i<def_count; ++i)
	    g_VDXRegisteredFilters[i] = ff->addFilter(fm, VDXGetVideoFilterDefinition(i), sizeof(VDXFilterDefinition));

	g_VFVAPIVersion = vdfd_ver;
    vdfd_ver        = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat     = ver_compat_target;
	
	return 0;
}

void VDXVideoFilterModuleDeinit(struct VDXFilterModule *fm, const VDXFilterFunctions *ff) {
	if (g_VDXRegisteredFilters) {
		for(int i=g_VDXRegisteredFilterCount-1; i>=0; --i) {
			VDXFilterDefinition *def = g_VDXRegisteredFilters[i];

			if (def)
				ff->removeFilter(def);
		}

		free(g_VDXRegisteredFilters);
		g_VDXRegisteredFilters = NULL;
	}
}

int VDXGetVideoFilterAPIVersion() {
	return g_VFVAPIVersion;
}
