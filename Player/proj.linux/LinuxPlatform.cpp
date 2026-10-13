#include "LinuxPlatform.h"

#include <cstdlib>
#include <cstring>

static std::vector<std::string> s_arguments;

void LinuxPlatform::setArguments(const std::vector<std::string> &args)
{
	s_arguments = args;
}

const std::vector<std::string> &LinuxPlatform::getArguments()
{
	return s_arguments;
}

std::string LinuxPlatform::getUiLocale()
{
	// Same precedence as gettext: LANGUAGE (first entry), LC_ALL, LC_MESSAGES, LANG.
	std::string lang;
	const char *language = getenv("LANGUAGE");
	if (language && *language) {
		lang = language;
		lang = lang.substr(0, lang.find(':'));
	}
	if (lang.empty()) {
		for (const char *name : { "LC_ALL", "LC_MESSAGES", "LANG" }) {
			const char *value = getenv(name);
			if (value && *value) {
				lang = value;
				break;
			}
		}
	}
	if (lang.compare(0, 2, "ja") == 0) {
		return "ja_JP";
	}
	// Windows only switches for Simplified Chinese (China).
	if (lang.compare(0, 5, "zh_CN") == 0) {
		return "zh_CN";
	}
	return "en_US";
}
