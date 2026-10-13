// Linux counterpart of proj.win32/DllPluginManager-win32.cpp.
//
// Games can ship native plugins as plugins/win32/*.dll. They are Windows code
// and cannot be loaded here, so this manager loads nothing and warns when a
// game has any: such games need the Windows player.

#include "Manager/DllPluginManager.h"

#include <strings.h>

class DllPluginManagerLinux : public DllPluginManager
{
	friend class DllPluginManager;
public:
	virtual bool loadPlugins() override
	{
		auto fileUtils = FileUtils::getInstance();
		auto dir = fileUtils->getDefaultResourceRootPath() + "plugins/win32";
		bool found = false;
		for (auto &name : fileUtils->getDirContents(dir)) {
			if (name.size() > 4 && strcasecmp(name.c_str() + name.size() - 4, ".dll") == 0) {
				CCLOG("Native plugin %s/%s is Windows-only and is not loaded.", dir.c_str(), name.c_str());
				fprintf(stderr, "player: warning: native plugin %s is Windows-only and is not loaded\n", name.c_str());
				found = true;
			}
		}
		return !found;
	}
	virtual void unloadPlugins() override {}
	virtual void updatePlugins(float dt) override {}
};

DllPluginManager *DllPluginManager::getInstance()
{
	if (!_dllPluginManager) {
		_dllPluginManager = new DllPluginManagerLinux();
	}
	return _dllPluginManager;
}
