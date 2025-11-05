/*
 * GlyphGL Utility Header - Memory Management and Debugging Macros
 *
 * This header provides configurable memory management functions and debugging
 * utilities for the GlyphGL library. All memory operations can be overridden
 * by defining custom GLYPH_MALLOC, GLYPH_FREE, and GLYPH_REALLOC macros before
 * including this header, allowing integration with custom allocators.
 *
 * The debugging system provides conditional logging that can be enabled by
 * defining GLYPHGL_DEBUG before including GlyphGL headers.
 */

#ifndef GLYPH_UTIL_H
#define GLYPH_UTIL_H

#include <stdlib.h>

/*
 * Memory allocation macro - defaults to standard malloc
 *
 * Can be overridden by defining GLYPH_MALLOC before including this header.
 * This allows GlyphGL to work with custom memory allocators or memory pools.
 */
#ifndef GLYPH_MALLOC
#define GLYPH_MALLOC malloc
#endif

/*
 * Memory deallocation macro - defaults to standard free
 *
 * Can be overridden by defining GLYPH_FREE before including this header.
 * Must be paired with the corresponding GLYPH_MALLOC implementation.
 */
#ifndef GLYPH_FREE
#define GLYPH_FREE free
#endif

/*
 * Memory reallocation macro - defaults to standard realloc
 *
 * Can be overridden by defining GLYPH_REALLOC before including this header.
 * Used for dynamic buffer growth in vertex batching and other operations.
 */
#ifndef GLYPH_REALLOC
#define GLYPH_REALLOC realloc
#endif

/*
 * Debug logging macro - conditionally compiled based on GLYPHGL_DEBUG
 *
 * When GLYPHGL_DEBUG is defined, this expands to printf() calls for logging.
 * When not defined, it compiles to nothing, eliminating runtime overhead.
 *
 * Usage: GLYPH_LOG("Error: %s\n", error_message);
 */
#ifdef GLYPHGL_DEBUG
#define GLYPH_LOG(...) printf(__VA_ARGS__)
#else
#define GLYPH_LOG(...)
#endif

#endif