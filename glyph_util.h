#ifndef GLYPH_UTIL_H
#define GLYPH_UTIL_H

#include <stdlib.h>
#ifndef GLYPH_MALLOC
#define GLYPH_MALLOC malloc
#endif

#ifndef GLYPH_FREE
#define GLYPH_FREE free
#endif

#ifndef GLYPH_REALLOC
#define GLYPH_REALLOC realloc
#endif

#ifdef GLYPHGL_DEBUG
#define GLYPH_LOG(...) printf(__VA_ARGS__)
#else
#define GLYPH_LOG(...)
#endif

#endif