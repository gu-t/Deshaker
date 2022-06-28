#include <vd2/plugin/vdplugin.h>
#include <vd2/plugin/vdinputdriver.h>

///////////////////////////////////////////////////////////////////////////////

extern const VDPluginInfo mocrap_plugin;
extern const VDPluginInfo avi_plugin;

const VDPluginInfo *const kPlugins[]={
	&mocrap_plugin,
	&avi_plugin,
	NULL
};

extern "C" const VDPluginInfo *const * VDXAPIENTRY VDGetPluginInfo() {
	return kPlugins;
}
