// libstdc++ before GCC 14 declares the float variants of the <cmath>
// functions (floorf, sinf, ...) only in the global namespace. The player
// sources call them through std, which MSVC and newer libstdc++ allow.
#pragma once

#include <cmath>

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 14
namespace std {
using ::acosf;
using ::asinf;
using ::atan2f;
using ::atanf;
using ::ceilf;
using ::cosf;
using ::coshf;
using ::expf;
using ::fabsf;
using ::floorf;
using ::fmodf;
using ::logf;
using ::log10f;
using ::powf;
using ::sinf;
using ::sinhf;
using ::sqrtf;
using ::tanf;
using ::tanhf;
}
#endif
