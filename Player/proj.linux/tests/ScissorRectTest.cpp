#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#define CC_TARGET_PLATFORM 5
#define CC_PLATFORM_LINUX 5
#define CC_PLATFORM_NX 99
#define USE_AGTK

using GLint = int;
using GLsizei = int;
using GLfloat = float;
enum { GL_VIEWPORT, GL_SCISSOR_BOX };
struct Vec { float x, y; };
struct Size { float width, height; };
struct Rect {
    Vec origin;
    Size size;
    Rect(float x = 0, float y = 0, float w = 0, float h = 0)
        : origin{x, y}, size{w, h} {}
};

static std::array<int, 4> viewport;
static std::array<int, 4> scissor;
static void glGetIntegerv(int, int *values)
{
    for (int i = 0; i < 4; ++i) values[i] = viewport[i];
}
static void glGetFloatv(int, float *values)
{
    for (int i = 0; i < 4; ++i) values[i] = scissor[i];
}
static void glScissor(int x, int y, int w, int h) { scissor = {{x, y, w, h}}; }

struct IMGUIGLViewImpl {
    float _frameZoomFactor = 1, _scaleX = 1, _scaleY = 1;
    int _retinaFactor = 1;
    Rect _viewPortRect;
    Size _designResolutionSize{640, 480};
    float framebufferScale = 1;
    float getFramebufferScale() const { return framebufferScale; }
    void setScissorInPoints(float, float, float, float);
    Rect getScissorRect() const;
};

#include "ScissorMethods.inc"

static void check(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

static void roundTrip(IMGUIGLViewImpl &view, bool offscreen)
{
    viewport = offscreen ? std::array<int, 4>{{0, 0, 640, 480}}
                         : std::array<int, 4>{{0, 0, 1920, 1080}};
    view.setScissorInPoints(32, 48, 120, 120);
    const auto original = scissor;
    const Rect saved = view.getScissorRect();
    check(std::fabs(saved.origin.x - 32) < 0.001f
        && std::fabs(saved.origin.y - 48) < 0.001f
        && std::fabs(saved.size.width - 120) < 0.001f
        && std::fabs(saved.size.height - 120) < 0.001f,
        "scissor readback changed the rectangle in points");

    // An inner layout changes the scissor before restoring its parent's box.
    view.setScissorInPoints(64, 64, 24, 24);
    view.setScissorInPoints(saved.origin.x, saved.origin.y, saved.size.width, saved.size.height);
    check(scissor == original, "nested layout did not restore the original GL scissor");
}

int main()
{
    IMGUIGLViewImpl view;
    for (float scale : {1.0f, 1.25f, 1.5f, 2.0f}) {
        view.framebufferScale = scale;
        for (bool offscreen : {false, true}) {
            roundTrip(view, offscreen);
            // Include design scaling, letterboxing, window zoom and retina.
            view._scaleX = 2;
            view._scaleY = 3;
            view._frameZoomFactor = 2;
            view._retinaFactor = 2;
            view._viewPortRect = Rect(8, 16);
            roundTrip(view, offscreen);
            view = IMGUIGLViewImpl();
            view.framebufferScale = scale;
        }
    }
}
