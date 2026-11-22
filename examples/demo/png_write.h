// png_write.h - writes 0xAARRGGBB pixels as an RGBA PNG (zlib stored
// blocks, no compression library), for the example's self-test.
#pragma once

#include <cstdint>

bool png_write(const char *path, const uint32_t *argb, int width, int height);
