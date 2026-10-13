/****************************************************************************
Copyright (c) 2011      Laschweinski
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_LINUX

#include "platform/linux/CCFileUtils-linux.h"
#include "platform/linux/CCApplication-linux.h"
#include "platform/CCCommon.h"
#include "base/ccMacros.h"
#include "base/ccUTF8.h"
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#ifdef USE_AGTK
#include <dirent.h>
#include <strings.h>
#include <algorithm>
#endif

#ifndef CC_RESOURCE_FOLDER_LINUX
#define CC_RESOURCE_FOLDER_LINUX ("/Resources/")
#endif

using namespace std;

#define DECLARE_GUARD std::lock_guard<std::recursive_mutex> mutexGuard(_mutex)

NS_CC_BEGIN

#ifdef USE_AGTK
static std::string s_applicationPath;

void FileUtilsLinux::setApplicationPath(const std::string& path)
{
    s_applicationPath = path;
    if (!s_applicationPath.empty() && s_applicationPath.back() != '/') {
        s_applicationPath += '/';
    }
}
#endif

FileUtils* FileUtils::getInstance()
{
    if (s_sharedFileUtils == nullptr)
    {
        s_sharedFileUtils = new FileUtilsLinux();
        if(!s_sharedFileUtils->init())
        {
          delete s_sharedFileUtils;
          s_sharedFileUtils = nullptr;
          CCLOG("ERROR: Could not init CCFileUtilsLinux");
        }
    }
    return s_sharedFileUtils;
}

FileUtilsLinux::FileUtilsLinux()
{}

bool FileUtilsLinux::init()
{
    DECLARE_GUARD;
    // get application path
    char fullpath[256] = {0};
    ssize_t length = readlink("/proc/self/exe", fullpath, sizeof(fullpath)-1);

    if (length <= 0) {
        return false;
    }

    fullpath[length] = '\0';
    std::string appPath = fullpath;
#ifdef USE_AGTK
    // Like player.exe on Windows, resources are read from the game folder itself.
    if (s_applicationPath.empty()) {
        setApplicationPath(appPath.substr(0, appPath.find_last_of("/")));
    }
    _defaultResRootPath = s_applicationPath;

    // Writable path: $XDG_DATA_HOME/pgmmv-player/player/, the counterpart of
    // %LOCALAPPDATA%/player/ on Windows. GameManager stores save data next to it,
    // in a folder named after the game folder.
    const char* xdg_data_path = getenv("XDG_DATA_HOME");
    std::string xdgDataPath;
    if (xdg_data_path == NULL || xdg_data_path[0] == '\0') {
        const char* home = getenv("HOME");
        xdgDataPath = home ? home : "";
        xdgDataPath += "/.local/share";
    } else {
        xdgDataPath = xdg_data_path;
    }
    _writablePath = xdgDataPath + "/pgmmv-player/player/";
    return FileUtils::init();
#else
    _defaultResRootPath = appPath.substr(0, appPath.find_last_of("/"));
    _defaultResRootPath += CC_RESOURCE_FOLDER_LINUX;
#endif

    // Set writable path to $XDG_CONFIG_HOME or ~/.config/<app name>/ if $XDG_CONFIG_HOME not exists.
    const char* xdg_config_path = getenv("XDG_CONFIG_HOME");
    std::string xdgConfigPath;
    if (xdg_config_path == NULL) {
        xdgConfigPath = getenv("HOME");
        xdgConfigPath += "/.config";
    } else {
        xdgConfigPath  = xdg_config_path;
    }
    _writablePath = xdgConfigPath;
    _writablePath += appPath.substr(appPath.find_last_of("/"));
    _writablePath += "/";

    return FileUtils::init();
}

string FileUtilsLinux::getWritablePath() const
{
    DECLARE_GUARD;
#ifdef USE_AGTK
    // The parent folders may not exist yet either.
    createDirectory(_writablePath);
#else
    struct stat st;
    stat(_writablePath.c_str(), &st);
    if (!S_ISDIR(st.st_mode)) {
        mkdir(_writablePath.c_str(), 0744);
    }
#endif

    return _writablePath;
}

#ifdef USE_AGTK
std::string FileUtilsLinux::getApplicationPath()
{
    return s_applicationPath;
}

std::vector<std::string> FileUtilsLinux::getDirContents(std::string dirname)
{
    // Regular files only, in the case-insensitive order Windows lists them in.
    std::vector<std::string> list;
    DIR* dir = opendir(dirname.c_str());
    if (dir == nullptr) {
        return list;
    }
    while (struct dirent* entry = readdir(dir)) {
        std::string path = dirname + "/" + entry->d_name;
        struct stat st;
        if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            list.push_back(entry->d_name);
        }
    }
    closedir(dir);
    std::sort(list.begin(), list.end(), [](const std::string& a, const std::string& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
    return list;
}

FILE* FileUtilsLinux::memFopen(const char* ptr, size_t sz, int p[2])
{
    (void)p;
    return fmemopen((void*)ptr, sz, "rb");
}

void FileUtilsLinux::memFclose(FILE* fp, int p[2])
{
    (void)p;
    fclose(fp);
}
#endif

bool FileUtilsLinux::isFileExistInternal(const std::string& strFilePath) const
{
    DECLARE_GUARD;
    if (strFilePath.empty())
    {
        return false;
    }

    std::string strPath = strFilePath;
    if (!isAbsolutePath(strPath))
    { // Not absolute path, add the default root path at the beginning.
        strPath.insert(0, _defaultResRootPath);
    }

    struct stat sts;
    return (stat(strPath.c_str(), &sts) == 0) && S_ISREG(sts.st_mode);
}

NS_CC_END

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_LINUX
