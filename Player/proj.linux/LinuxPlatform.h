// Linux replacements for what the Windows player gets from Win32 APIs.
#ifndef __LINUX_PLATFORM_H__
#define __LINUX_PLATFORM_H__

#include <string>
#include <vector>

class LinuxPlatform
{
public:
	// Called by main() with the arguments meant for the player (argv[0] included),
	// in place of GetCommandLine() on Windows.
	static void setArguments(const std::vector<std::string> &args);
	static const std::vector<std::string> &getArguments();

	// UI language as the Windows player picks it from GetUserDefaultUILanguage():
	// "ja_JP", "zh_CN" or "en_US".
	static std::string getUiLocale();
};

#endif // __LINUX_PLATFORM_H__
