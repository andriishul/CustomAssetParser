#pragma once
#include <cstdint>

namespace asset
{
    constexpr uint32_t MODEL_MAGIC = 0x4C444F4D;
    
    constexpr uint32_t MAX_STRING_LENGTH = 64;

    /*
        1. AssetHeader
        2. MeshEntry
        3. AnimationChannel
        4. AnimationHeader

        5. StaticVertex
        6. SkinnedVertex
        7. MaterialEntry
        8. BoneEntry
        9. Frames BLOB
    */
    struct ModelHeader
    {
        uint32_t magic = MODEL_MAGIC;

        uint32_t meshCount = 0;
        uint32_t animationCount = 0;
        uint32_t materialCount = 0;
        uint32_t boneCount = 0;

        uint64_t meshTableOffset = 0;
        uint64_t animationTableOffset = 0;

        uint64_t materialTableOffset;
        uint64_t boneTableOffset = 0;

        uint64_t staticVertexBlobOffset = 0;
        uint64_t staticVertexBlobSize = 0;
        uint64_t skinnedVertexBlobOffset = 0;
        uint64_t skinnedVertexBlobSize = 0;

        uint64_t indexBlobOffset = 0;
        uint64_t indexBlobSize = 0;

        uint64_t channelTableOffset = 0;
        uint64_t channelTableSize = 0;

        uint64_t keyframeBlobOffset = 0;
        uint64_t keyframeBlobSize = 0;
    };


    struct StaticVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
        float tangent[4];
    };

    struct SkinnedVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
        float tangent[4];
        uint32_t boneIndices[4];
        float boneWeights[4];
    };

    struct MaterialEntry
    {
        char albedoPath[MAX_STRING_LENGTH];
        char normalPath[MAX_STRING_LENGTH];
        char mrPath[MAX_STRING_LENGTH];
        char emissivePath[MAX_STRING_LENGTH];

        float baseColor[4];
        float metallic = 0;
        float roughness = 0;
    };

    struct BoneEntry
    {
        char name[MAX_STRING_LENGTH];
        float bindLocalTransform[16];
        float inverseBindMatrix[16];
        int32_t parentIndex = -1;
    };

    struct MeshEntry
    {
        uint32_t vertexIndex = 0;    
        uint32_t vertexCount = 0;
        
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        
        uint32_t materialIndex = 0;
        uint32_t hasSkin = 0;
    };

    struct AnimationHeader
    {
        char name[MAX_STRING_LENGTH];
        float duration = 0;
        uint32_t channelIndex = 0;
        uint32_t channelCount = 0;
    };

    struct AnimationKey
    {
        float time;
        float position[3];
        float rotation[4];
        float scale[3];
    };

    struct AnimationChannel
    {
        uint32_t boneIndex = 0;
        uint32_t keyCount = 0;
        uint32_t keyIndex = 0;
    };
}