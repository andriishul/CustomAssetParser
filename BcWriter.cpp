#include "BcWriter.hpp"
#include <fstream>
#include <vector>

#include <DirectXTex.h>

#include <stdexcept>
#include <iostream>

using namespace DirectX;

namespace asset::texture
{
	void convertCompressedTo(const wchar_t* in, const char* out, Format bcFormat)
    {
        ScratchImage src, mips, bc;

        DXGI_FORMAT fmt;
        bool srgb;
        uint32_t blockBytes = 0U;

        switch (bcFormat)
        {
            case Format::BC1_RGB_UNORM_BLOCK:
            case Format::BC1_RGBA_UNORM_BLOCK: fmt = DXGI_FORMAT_BC1_UNORM; srgb = false; blockBytes = 8;  break;
            case Format::BC1_RGB_SRGB_BLOCK:
            case Format::BC1_RGBA_SRGB_BLOCK: fmt = DXGI_FORMAT_BC1_UNORM_SRGB; srgb = true;  blockBytes = 8;  break;

            case Format::BC2_UNORM_BLOCK: fmt = DXGI_FORMAT_BC2_UNORM; srgb = false; blockBytes = 16; break;
            case Format::BC2_SRGB_BLOCK: fmt = DXGI_FORMAT_BC2_UNORM_SRGB; srgb = true;  blockBytes = 16; break;

            case Format::BC3_UNORM_BLOCK: fmt = DXGI_FORMAT_BC3_UNORM; srgb = false; blockBytes = 16; break;
            case Format::BC3_SRGB_BLOCK: fmt = DXGI_FORMAT_BC3_UNORM_SRGB; srgb = true;  blockBytes = 16; break;

            case Format::BC4_UNORM_BLOCK: fmt = DXGI_FORMAT_BC4_UNORM; srgb = false; blockBytes = 8;  break;
            case Format::BC4_SNORM_BLOCK: fmt = DXGI_FORMAT_BC4_SNORM; srgb = false; blockBytes = 8;  break;

            case Format::BC5_UNORM_BLOCK: fmt = DXGI_FORMAT_BC5_UNORM; srgb = false; blockBytes = 16; break;
            case Format::BC5_SNORM_BLOCK: fmt = DXGI_FORMAT_BC5_SNORM; srgb = false; blockBytes = 16; break;

            case Format::BC7_UNORM_BLOCK: fmt = DXGI_FORMAT_BC7_UNORM; srgb = false; blockBytes = 16; break;
            case Format::BC7_SRGB_BLOCK: fmt = DXGI_FORMAT_BC7_UNORM_SRGB; srgb = true;  blockBytes = 16; break;
            default: throw std::runtime_error("[convert] Texture format not supported.");
        }

        if (FAILED(LoadFromWICFile(in, srgb ? WIC_FLAGS_DEFAULT_SRGB : WIC_FLAGS_NONE, nullptr, src)))
            throw std::runtime_error("[convert] LoadFromWICFile failed");

        std::cout << "loaded " << src.GetMetadata().width << "x" << src.GetMetadata().height << "\n";

        if (FAILED(GenerateMipMaps(src.GetImages(), src.GetImageCount(), src.GetMetadata(), srgb ? TEX_FILTER_SRGB : TEX_FILTER_DEFAULT, 0, mips)))
            throw std::runtime_error("[convert] GenerateMipMaps failed");
        std::cout << "mips done\n";

        if (FAILED(Compress(mips.GetImages(), mips.GetImageCount(), mips.GetMetadata(), fmt, TEX_COMPRESS_PARALLEL | TEX_COMPRESS_BC7_QUICK, 1.0f, bc)))
            throw std::runtime_error("[convert] Compress failed");
        std::cout << "compressed\n";
        
        const TexMetadata& meta = bc.GetMetadata();
        std::ofstream fs(out, std::ios::binary);
        
        BcTextureHeader header
        {
            BC_MAGIC,
            static_cast<uint32_t>(meta.width), static_cast<uint32_t>(meta.height),
            static_cast<uint32_t>(bcFormat),
            blockBytes,
            static_cast<uint32_t>(meta.mipLevels) 
        };

        fs.write((const char*)&header, sizeof(header));

        for (size_t i = 0; i < meta.mipLevels; ++i)
        {
            const Image& img = bc.GetImages()[i];
            fs.write((const char*)img.pixels, img.slicePitch);      
        }
    }
}