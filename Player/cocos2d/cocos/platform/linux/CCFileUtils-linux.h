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
#ifndef __CC_FILEUTILS_LINUX_H__
#define __CC_FILEUTILS_LINUX_H__

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_LINUX

#include "platform/CCFileUtils.h"
#include "platform/CCPlatformMacros.h"
#include "base/ccTypes.h"
#include <string>
#include <vector>

NS_CC_BEGIN

/**
 * @addtogroup platform
 * @{
 */

//! @brief  Helper class to handle file operations
class CC_DLL FileUtilsLinux : public FileUtils
{
    friend class FileUtils;
protected:
    FileUtilsLinux();
private:
    std::string _writablePath;
public:
    /* override functions */
    bool init() override;
    virtual std::string getWritablePath() const override;
#ifdef USE_AGTK
    /**
     * Sets the game folder: resources are looked up there and getApplicationPath() returns it,
     * like the folder of player.exe on Windows. Call before FileUtils::getInstance().
     * Defaults to the folder of the executable.
     */
    static void setApplicationPath(const std::string& path);
    virtual std::string getApplicationPath() override;
    virtual std::vector<std::string> getDirContents(std::string dirname) override;
    virtual FILE* memFopen(const char* ptr, size_t sz, int p[2]) override;
    virtual void memFclose(FILE* fp, int p[2]) override;

    /**
     * Returns path unchanged if it exists. Otherwise matches each component ignoring
     * case and accepts backslashes as separators, like file lookups on Windows, which game
     * data relies on. Components that cannot be found are kept as given, so the result
     * also works for files that are about to be created.
     */
    static std::string resolvePathCase(const std::string& path);
    virtual std::string getSuitableFOpen(const std::string& filenameUtf8) const override;
    virtual long getFileSize(const std::string &filepath) const override;
    virtual time_t getLastWriteTime(const std::string &filename) override;
#endif
private:
    virtual bool isFileExistInternal(const std::string& strFilePath) const override;
#ifdef USE_AGTK
    virtual bool isDirectoryExistInternal(const std::string& dirPath) const override;
#endif
};

// end of platform group
/// @}

NS_CC_END

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_LINUX

#endif    // __CC_FILEUTILS_LINUX_H__
