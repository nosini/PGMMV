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
#include <mutex>
#include <unordered_map>
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
    dirname = resolvePathCase(dirname);
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

namespace {

// Directory listings used by resolvePathCase, keyed by directory path and reread when the
// directory's modification time changes. Textures load on several threads, hence the mutex.
struct DirListing
{
    bool valid = false;
    struct timespec mtime = {};
    std::vector<std::string> names; // sorted case-insensitively, so ties resolve the same way
};
std::mutex s_dirListingMutex;
std::unordered_map<std::string, DirListing> s_dirListings;

// Returns the entry of dir that equals name ignoring ASCII case, or "" if there is none.
std::string findNameIgnoringCase(const std::string& dir, const std::string& name)
{
    struct stat st;
    if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        return "";
    }
    std::lock_guard<std::mutex> lock(s_dirListingMutex);
    DirListing& listing = s_dirListings[dir];
    if (!listing.valid || listing.mtime.tv_sec != st.st_mtim.tv_sec || listing.mtime.tv_nsec != st.st_mtim.tv_nsec) {
        listing.names.clear();
        if (DIR* d = opendir(dir.c_str())) {
            while (struct dirent* entry = readdir(d)) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    listing.names.push_back(entry->d_name);
                }
            }
            closedir(d);
        }
        std::sort(listing.names.begin(), listing.names.end());
        listing.valid = true;
        listing.mtime = st.st_mtim;
    }
    for (const auto& entry : listing.names) {
        if (strcasecmp(entry.c_str(), name.c_str()) == 0) {
            return entry;
        }
    }
    return "";
}

} // namespace

std::string FileUtilsLinux::resolvePathCase(const std::string& path)
{
    struct stat st;
    if (path.empty() || stat(path.c_str(), &st) == 0) {
        return path;
    }
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    // result always ends with '/' (or is empty) before the next component is added.
    std::string result;
    size_t pos = 0;
    if (normalized[0] == '/') {
        result = "/";
        pos = 1;
    }
    bool resolving = true;
    while (pos <= normalized.size()) {
        size_t end = normalized.find('/', pos);
        if (end == std::string::npos) {
            end = normalized.size();
        }
        std::string part = normalized.substr(pos, end - pos);
        if (!part.empty()) {
            std::string candidate = result + part;
            if (resolving && part != "." && part != ".." && lstat(candidate.c_str(), &st) != 0) {
                std::string match = findNameIgnoringCase(result.empty() ? "." : result, part);
                if (match.empty()) {
                    resolving = false; // keep the rest as given
                } else {
                    candidate = result + match;
                }
            }
            result = candidate;
        }
        if (end < normalized.size()) {
            result += '/';
        }
        pos = end + 1;
    }
    return result;
}

std::string FileUtilsLinux::getSuitableFOpen(const std::string& filenameUtf8) const
{
    return resolvePathCase(filenameUtf8);
}

long FileUtilsLinux::getFileSize(const std::string &filepath) const
{
    std::string fullpath = isAbsolutePath(filepath) ? filepath : fullPathForFilename(filepath);
    if (fullpath.empty()) {
        return 0;
    }
    return FileUtils::getFileSize(resolvePathCase(fullpath));
}

time_t FileUtilsLinux::getLastWriteTime(const std::string &filename)
{
    return FileUtils::getLastWriteTime(resolvePathCase(fullPathForFilename(filename)));
}

bool FileUtilsLinux::isDirectoryExistInternal(const std::string& dirPath) const
{
    struct stat st;
    return stat(resolvePathCase(dirPath).c_str(), &st) == 0 && S_ISDIR(st.st_mode);
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
#ifdef USE_AGTK
    strPath = resolvePathCase(strPath);
#endif
    return (stat(strPath.c_str(), &sts) == 0) && S_ISREG(sts.st_mode);
}

NS_CC_END

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_LINUX
