// png_write.cpp - the self-test's PNG writer (png_write.h): IHDR, one IDAT
// of uncompressed deflate blocks with filter 0 on every row, IEND.
#include "png_write.h"

#include <cstdio>
#include <vector>

namespace {

uint32_t crc32(const unsigned char *p, size_t n, uint32_t crc = 0)
{
    static uint32_t table[256];
    if (!table[1]) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            table[i] = c;
        }
    }
    crc = ~crc;
    for (size_t i = 0; i < n; i++) {
        crc = table[(crc ^ p[i]) & 255] ^ (crc >> 8);
    }
    return ~crc;
}

void put32(std::vector<unsigned char> &out, uint32_t v)
{
    out.push_back((unsigned char)(v >> 24));
    out.push_back((unsigned char)(v >> 16));
    out.push_back((unsigned char)(v >> 8));
    out.push_back((unsigned char)v);
}

void chunk(std::vector<unsigned char> &out, const char *kind, const std::vector<unsigned char> &body)
{
    put32(out, (uint32_t)body.size());
    std::vector<unsigned char> typed(kind, kind + 4);
    typed.insert(typed.end(), body.begin(), body.end());
    out.insert(out.end(), typed.begin(), typed.end());
    put32(out, crc32(typed.data(), typed.size()));
}

} // namespace

bool png_write(const char *path, const uint32_t *argb, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }
    std::vector<unsigned char> raw;
    raw.reserve((size_t)height * ((size_t)width * 4 + 1));
    for (int y = 0; y < height; y++) {
        raw.push_back(0);
        for (int x = 0; x < width; x++) {
            const uint32_t p = argb[(size_t)y * width + x];
            raw.push_back((unsigned char)(p >> 16));
            raw.push_back((unsigned char)(p >> 8));
            raw.push_back((unsigned char)p);
            raw.push_back((unsigned char)(p >> 24));
        }
    }
    std::vector<unsigned char> z = { 0x78, 0x01 };
    uint32_t a = 1, b = 0;
    for (unsigned char c : raw) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    for (size_t at = 0; at < raw.size() || raw.empty(); at += 65535) {
        const size_t n = raw.size() - at < 65535 ? raw.size() - at : 65535;
        z.push_back(at + n == raw.size() ? 1 : 0);
        z.push_back((unsigned char)n);
        z.push_back((unsigned char)(n >> 8));
        z.push_back((unsigned char)~n);
        z.push_back((unsigned char)(~n >> 8));
        z.insert(z.end(), raw.begin() + (ptrdiff_t)at, raw.begin() + (ptrdiff_t)(at + n));
        if (raw.empty()) {
            break;
        }
    }
    put32(z, b << 16 | a);
    std::vector<unsigned char> ihdr;
    put32(ihdr, (uint32_t)width);
    put32(ihdr, (uint32_t)height);
    ihdr.insert(ihdr.end(), { 8, 6, 0, 0, 0 });
    std::vector<unsigned char> out = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    FILE *f = std::fopen(path, "wb");
    if (!f) {
        return false;
    }
    const bool ok = std::fwrite(out.data(), 1, out.size(), f) == out.size();
    return std::fclose(f) == 0 && ok;
}
