// The libvlc functions the player calls, loaded from libvlc.so.5 when the
// first movie is created instead of being linked. The player then starts on
// systems without VLC; there, creating a movie fails and the game goes on
// without it, as when a movie file is missing.

#include <vlc/vlc.h>

#include <dlfcn.h>
#include <cstdio>

namespace {

void *libvlc()
{
	static void *handle = [] {
		void *library = dlopen("libvlc.so.5", RTLD_NOW | RTLD_LOCAL);
		if (!library) {
			fprintf(stderr, "player: movies are disabled: %s\n", dlerror());
		}
		return library;
	}();
	return handle;
}

template <typename Function>
Function resolve(const char *name)
{
	void *library = libvlc();
	return library ? reinterpret_cast<Function>(dlsym(library, name)) : nullptr;
}

}

// Calls the real function, or returns `fallback` if libvlc is unavailable.
#define LIBVLC_FORWARD(name, fallback, ...) \
	static const auto real = resolve<decltype(&name)>(#name); \
	return real ? real(__VA_ARGS__) : fallback

#define LIBVLC_FORWARD_VOID(name, ...) \
	static const auto real = resolve<decltype(&name)>(#name); \
	if (real) real(__VA_ARGS__)

extern "C" {

libvlc_instance_t *libvlc_new(int argc, const char *const *argv)
{
	LIBVLC_FORWARD(libvlc_new, nullptr, argc, argv);
}

void libvlc_release(libvlc_instance_t *instance)
{
	LIBVLC_FORWARD_VOID(libvlc_release, instance);
}

libvlc_media_t *libvlc_media_new_path(libvlc_instance_t *instance, const char *path)
{
	LIBVLC_FORWARD(libvlc_media_new_path, nullptr, instance, path);
}

void libvlc_media_add_option(libvlc_media_t *media, const char *options)
{
	LIBVLC_FORWARD_VOID(libvlc_media_add_option, media, options);
}

void libvlc_media_release(libvlc_media_t *media)
{
	LIBVLC_FORWARD_VOID(libvlc_media_release, media);
}

unsigned libvlc_media_tracks_get(libvlc_media_t *media, libvlc_media_track_t ***tracks)
{
	LIBVLC_FORWARD(libvlc_media_tracks_get, 0, media, tracks);
}

libvlc_media_player_t *libvlc_media_player_new(libvlc_instance_t *instance)
{
	LIBVLC_FORWARD(libvlc_media_player_new, nullptr, instance);
}

libvlc_media_player_t *libvlc_media_player_new_from_media(libvlc_media_t *media)
{
	LIBVLC_FORWARD(libvlc_media_player_new_from_media, nullptr, media);
}

void libvlc_media_player_release(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD_VOID(libvlc_media_player_release, player);
}

void libvlc_media_player_set_media(libvlc_media_player_t *player, libvlc_media_t *media)
{
	LIBVLC_FORWARD_VOID(libvlc_media_player_set_media, player, media);
}

int libvlc_media_player_play(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD(libvlc_media_player_play, -1, player);
}

void libvlc_media_player_set_pause(libvlc_media_player_t *player, int pause)
{
	LIBVLC_FORWARD_VOID(libvlc_media_player_set_pause, player, pause);
}

void libvlc_media_player_pause(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD_VOID(libvlc_media_player_pause, player);
}

void libvlc_media_player_stop(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD_VOID(libvlc_media_player_stop, player);
}

libvlc_state_t libvlc_media_player_get_state(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD(libvlc_media_player_get_state, libvlc_Error, player);
}

libvlc_event_manager_t *libvlc_media_player_event_manager(libvlc_media_player_t *player)
{
	LIBVLC_FORWARD(libvlc_media_player_event_manager, nullptr, player);
}

int libvlc_event_attach(libvlc_event_manager_t *manager, libvlc_event_type_t type,
	libvlc_callback_t callback, void *data)
{
	LIBVLC_FORWARD(libvlc_event_attach, -1, manager, type, callback, data);
}

void libvlc_video_set_callbacks(libvlc_media_player_t *player, libvlc_video_lock_cb lock,
	libvlc_video_unlock_cb unlock, libvlc_video_display_cb display, void *data)
{
	LIBVLC_FORWARD_VOID(libvlc_video_set_callbacks, player, lock, unlock, display, data);
}

void libvlc_video_set_format(libvlc_media_player_t *player, const char *chroma,
	unsigned width, unsigned height, unsigned pitch)
{
	LIBVLC_FORWARD_VOID(libvlc_video_set_format, player, chroma, width, height, pitch);
}

int libvlc_audio_set_volume(libvlc_media_player_t *player, int volume)
{
	LIBVLC_FORWARD(libvlc_audio_set_volume, -1, player, volume);
}

}
