#pragma once
#include "BcFormat.hpp"

namespace asset::texture
{
    enum class Format 
    {
        BC1_RGB_UNORM_BLOCK = 42,
        BC1_RGB_SRGB_BLOCK = 43,
        BC1_RGBA_UNORM_BLOCK = 44,
        BC1_RGBA_SRGB_BLOCK = 45,
        BC2_UNORM_BLOCK = 46,
        BC2_SRGB_BLOCK = 47,
        BC3_UNORM_BLOCK = 48,
        BC3_SRGB_BLOCK = 49,
        BC4_UNORM_BLOCK = 50,
        BC4_SNORM_BLOCK = 51,
        BC5_UNORM_BLOCK = 52,
        BC5_SNORM_BLOCK = 53,
        BC6H_UFLOAT_BLOCK = 54,
        BC6H_SFLOAT_BLOCK = 55,
        BC7_UNORM_BLOCK = 56,
        BC7_SRGB_BLOCK = 57
    };


    void convertCompressedTo(const wchar_t* in, const char* out, Format bcFormat);
}