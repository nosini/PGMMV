// Linux counterpart of proj.win32/stdafx.h, force-included into every player
// C++ source like the Windows projects do.
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <iostream>
#include <string>
#include <strings.h>

// Windows CRT and SDK names used by the player sources.
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define ARRAYSIZE(a) CC_ARRAYSIZE(a)

#include "cocos2d.h"

#include "ImGUI/IMGUIGLViewImpl.h"
#include "ImGUI/ImGuiLayer.h"
#include "ImGUI/imgui.h"
#include "vlc/vlc.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "External/gif/gif_lib.h"
#include "External/SSPlayer/SS6Player.h"
#include "External/SSPlayer/SS6PlayerData.h"
#include "External/collision/CollisionComponent.hpp"
#include "External/collision/CollisionUtils.hpp"

#endif //__STDAFX_H__
