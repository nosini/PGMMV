// Entry point of the Linux player (proj.win32/main.cpp on Windows).
//
// usage: player [--game-dir=DIR] [player arguments...]
//
// The Windows player.exe sits in the game folder and reads the game from
// there. The Linux player is installed separately, so the game folder is given
// with --game-dir. Without it, the current directory is used if it holds a
// game, and the folder of the executable otherwise. The remaining arguments
// are passed on as the Windows command line would be.

#include "AppDelegate.h"
#include "LinuxPlatform.h"
#include "cocos2d.h"
#include "platform/linux/CCFileUtils-linux.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

USING_NS_CC;

static bool isGameFolder(const std::string &dir)
{
	struct stat st;
	return stat((dir + "/data/project.json").c_str(), &st) == 0
		|| stat((dir + "/Resources/data/project.json").c_str(), &st) == 0;
}

static std::string currentDirectory()
{
	char buf[PATH_MAX];
	return getcwd(buf, sizeof(buf)) ? std::string(buf) : std::string();
}

int main(int argc, char **argv)
{
	std::string gameDir;
	std::vector<std::string> args;
	args.push_back(argv[0]);
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--game-dir=", 11) == 0) {
			gameDir = argv[i] + 11;
		}
		else if (strcmp(argv[i], "--game-dir") == 0 && i + 1 < argc) {
			gameDir = argv[++i];
		}
		else {
			args.push_back(argv[i]);
		}
	}

	if (gameDir.empty() && isGameFolder(currentDirectory())) {
		gameDir = currentDirectory();
	}
	if (!gameDir.empty()) {
		char resolved[PATH_MAX];
		if (!realpath(gameDir.c_str(), resolved)) {
			fprintf(stderr, "player: cannot open game folder %s\n", gameDir.c_str());
			return 2;
		}
		gameDir = resolved;
		// Windows starts the player with the game folder as working directory.
		if (chdir(gameDir.c_str()) != 0) {
			fprintf(stderr, "player: cannot enter game folder %s\n", gameDir.c_str());
			return 2;
		}
		FileUtilsLinux::setApplicationPath(gameDir);
	}
	LinuxPlatform::setArguments(args);

	AppDelegate app;
	return Application::getInstance()->run();
}
