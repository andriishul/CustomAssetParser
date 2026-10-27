#include "ModelWriter.hpp"

#include <stdexcept>

#include <cstring>

#include <unordered_map>
#include <unordered_set>

#include <fstream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace asset
{
    struct ModelWriter::ModelData
    {
        Assimp::Importer importer;

        std::unordered_map<std::string, uint32_t> boneNameToIndex;

        std::vector<StaticVertex> staticVertices;
        std::vector<SkinnedVertex> skinnedVertices;
        std::vector<uint32_t> indices;

        std::vector<MeshEntry> meshes;
        std::vector<MaterialEntry> materials;
        std::vector<BoneEntry> bones;

        std::vector<AnimationChannel> channels;
        std::vector<AnimationHeader> animationHeaders;

        std::vector<uint8_t> keyframeBlob;

        std::string name;
        std::string path;
    };

    template <uint32_t N>
    static void copyString(char(&dst)[N], const aiString& src)
    {
        uint32_t length = std::min(N - 1, src.length);

        strncpy(dst, src.C_Str(), length);

        dst[length] = 0;
    }

    template <uint32_t N>
    static void copyTexturePath(char(&dst)[N], const aiString& src)
    {
        copyString(dst, src);
        for (auto& ch : dst) if (ch == '\\') ch = '/';
    }
    
    
    static void copyTextureFile(const std::string& directoryFrom, const std::string& directoryTo, const std::string& file)
    {
        if (!file[0]) return;
        std::string src = directoryFrom + '/' + file;
        std::string dst = directoryTo + '/' + file;
        if (!std::filesystem::exists(src) || std::filesystem::exists(dst)) return;

        auto lastSlash = dst.find_last_of("/");

        if(lastSlash != std::string::npos)
            std::filesystem::create_directories(dst.substr(0, lastSlash));
        
        std::filesystem::copy_file(src, dst);
    }

    static aiVector3D sampleVec3(const aiVectorKey* keys, uint32_t n, double time, const aiVector3D& fallback)
    {
        if (n == 0) return fallback;
        if (time <= keys[0].mTime) return keys[0].mValue;
        if (time >= keys[n - 1].mTime) return keys[n - 1].mValue;

        for (uint32_t i = 0; i + 1 < n; ++i)
        {
            if (time <= keys[i + 1].mTime)
            {
                double span = keys[i + 1].mTime - keys[i].mTime;
                float a = (span > 0.0) ? float((time - keys[i].mTime) / span) : 0.0f;
                return keys[i].mValue * (1.0f - a) + keys[i + 1].mValue * a;
            }
        }
        return keys[n - 1].mValue;
    }

    static aiQuaternion sampleQuat(const aiQuatKey* keys, uint32_t n, double time, const aiQuaternion& quat)
    {
        if (n == 0) return quat;
        if (time <= keys[0].mTime) return keys[0].mValue;
        if (time >= keys[n - 1].mTime) return keys[n - 1].mValue;

        for (uint32_t i = 0; i + 1 < n; ++i)
        {
            if (time <= keys[i + 1].mTime)
            {
                double span = keys[i + 1].mTime - keys[i].mTime;
                float a = (span > 0.0) ? float((time - keys[i].mTime) / span) : 0.0f;
                aiQuaternion out;
                aiQuaternion::Interpolate(out, keys[i].mValue, keys[i + 1].mValue, a);
                return out;
            }
        }
        return keys[n - 1].mValue;
    }

    static void buildBones(const aiNode* node, int32_t parentBoneIdx, 
        std::vector<BoneEntry>& bones, std::unordered_map<std::string, uint32_t>& map)
    {
        int32_t thisBoneIdx = parentBoneIdx;
      
        BoneEntry entry{};
        copyString(entry.name, node->mName);

        aiMatrix4x4 m = node->mTransformation;
        m.Transpose();
        memcpy(entry.bindLocalTransform, &m, sizeof(float) * 16);

        entry.parentIndex = parentBoneIdx;
        thisBoneIdx = static_cast<int32_t>(bones.size());

        bones.push_back(entry);
        map[node->mName.C_Str()] = thisBoneIdx;
        
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            buildBones(node->mChildren[i], thisBoneIdx, bones, map);
    }


    static void processModel(const aiScene* scene,
        std::unordered_map<std::string, uint32_t>& boneNameToIndex,
        std::vector<MeshEntry>& meshes,
        std::vector<SkinnedVertex>& skinned,
        std::vector<StaticVertex>& statc,
        std::vector<uint32_t>& indices,
        std::vector<BoneEntry>& bones)
    {
        uint32_t indexCursor = 0;
        
        buildBones(scene->mRootNode, -1, bones, boneNameToIndex);

        for (size_t i = 0; i < bones.size(); ++i)
        {
            aiMatrix4x4 world;
            memcpy(&world, bones[i].bindLocalTransform, sizeof(float) * 16);
        }
       

        for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
        {
            const aiMesh* mesh = scene->mMeshes[m];
            const bool isSkinned = mesh->mNumBones > 0;

            if (isSkinned) 
            {
                for (uint32_t b = 0; b < mesh->mNumBones; ++b)
                {
                    const auto* bone = mesh->mBones[b];

                    auto it = boneNameToIndex.find(bone->mName.C_Str());
                    if (it == boneNameToIndex.end()) continue;

                    aiMatrix4x4 inv = bone->mOffsetMatrix;
                    inv.Transpose();
                    memcpy(bones[it->second].inverseBindMatrix, &inv, sizeof(float) * 16);
                }
            }


            MeshEntry entry{};
            entry.vertexIndex = isSkinned
                ? static_cast<uint32_t>(skinned.size())
                : static_cast<uint32_t>(statc.size());
            entry.vertexCount = mesh->mNumVertices;
            entry.firstIndex = indexCursor;
            entry.indexCount = 0; 
            entry.materialIndex = mesh->mMaterialIndex;
            entry.hasSkin = isSkinned ? 1 : 0;

            if (isSkinned)
            {
                for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
                {
                    SkinnedVertex vertex{};

                    vertex.position[0] = mesh->mVertices[v].x;
                    vertex.position[1] = mesh->mVertices[v].y;
                    vertex.position[2] = mesh->mVertices[v].z;

                    if (mesh->mNormals)
                    {
                        vertex.normal[0] = mesh->mNormals[v].x;
                        vertex.normal[1] = mesh->mNormals[v].y;
                        vertex.normal[2] = mesh->mNormals[v].z;
                    }

                    if (mesh->mTextureCoords[0])
                    {
                        vertex.uv[0] = mesh->mTextureCoords[0][v].x;
                        vertex.uv[1] = mesh->mTextureCoords[0][v].y;
                    }

                    if (mesh->mTangents)
                    {
                        vertex.tangent[0] = mesh->mTangents[v].x;
                        vertex.tangent[1] = mesh->mTangents[v].y;
                        vertex.tangent[2] = mesh->mTangents[v].z;

                        if (mesh->mBitangents && mesh->mNormals)
                        {
                            aiVector3D n = mesh->mNormals[v];
                            aiVector3D t = mesh->mTangents[v];
                            aiVector3D b = mesh->mBitangents[v];
                            aiVector3D cross = n ^ t;
                            float dot = cross * b;
                            vertex.tangent[3] = (dot < 0.0f) ? -1.0f : 1.0f;
                        }
                    }
                    for (uint32_t s = 0; s < 4; ++s)
                    {
                        vertex.boneIndices[s] = 0;
                        vertex.boneWeights[s] = 0.0f;
                    }

                    skinned.push_back(vertex);
                }
            }
            else
            {
                for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
                {
                    StaticVertex vertex{};

                    vertex.position[0] = mesh->mVertices[v].x;
                    vertex.position[1] = mesh->mVertices[v].y;
                    vertex.position[2] = mesh->mVertices[v].z;

                    if (mesh->mNormals)
                    {
                        vertex.normal[0] = mesh->mNormals[v].x;
                        vertex.normal[1] = mesh->mNormals[v].y;
                        vertex.normal[2] = mesh->mNormals[v].z;
                    }

                    if (mesh->mTextureCoords[0])
                    {
                        vertex.uv[0] = mesh->mTextureCoords[0][v].x;
                        vertex.uv[1] = mesh->mTextureCoords[0][v].y;
                    }

                    if (mesh->mTangents)
                    {
                        vertex.tangent[0] = mesh->mTangents[v].x;
                        vertex.tangent[1] = mesh->mTangents[v].y;
                        vertex.tangent[2] = mesh->mTangents[v].z;

                        if (mesh->mBitangents && mesh->mNormals)
                        {
                            aiVector3D n = mesh->mNormals[v];
                            aiVector3D t = mesh->mTangents[v];
                            aiVector3D b = mesh->mBitangents[v];
                            aiVector3D cross = n ^ t;
                            float dot = cross * b;
                            vertex.tangent[3] = (dot < 0.0f) ? -1.0f : 1.0f;
                        }
                    }

                    statc.push_back(vertex);
                }
            }

            for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
            {
                const aiFace& face = mesh->mFaces[f];
                for (uint32_t k = 0; k < face.mNumIndices; ++k)
                    indices.push_back(face.mIndices[k]);
                entry.indexCount += face.mNumIndices;
            }

            const uint32_t vertexBase = static_cast<uint32_t>(entry.vertexIndex);

            if (isSkinned)
            {
                for (uint32_t b = 0; b < mesh->mNumBones; ++b)
                {
                    const auto bone = mesh->mBones[b];

                    auto it = boneNameToIndex.find(bone->mName.C_Str());
                    if (it == boneNameToIndex.end()) continue; 
                    const uint32_t boneIndex = it->second;
                    for (uint32_t w = 0; w < bone->mNumWeights; ++w)
                    {
                        const aiVertexWeight& vw = bone->mWeights[w];
                        SkinnedVertex& v = skinned[vertexBase + vw.mVertexId];

                        for (uint32_t slot = 0; slot < 4; ++slot)
                        {
                            if (v.boneWeights[slot] == 0.0f)
                            {
                                v.boneIndices[slot] = boneIndex;
                                v.boneWeights[slot] = vw.mWeight;
                                break;
                            }
                        }
                    }
                }

                if (!bones.empty())
                {
                    for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
                    {
                        SkinnedVertex& vert = skinned[vertexBase + v];
                        float sum = vert.boneWeights[0] + vert.boneWeights[1] +
                            vert.boneWeights[2] + vert.boneWeights[3];

                        if (sum == 0.0f)
                        {
                            vert.boneIndices[0] = 0;
                            vert.boneWeights[0] = 1.0f;
                        }
                    }
                }
            }

            meshes.push_back(entry);
            indexCursor += entry.indexCount;
        }
    }

    static void processMaterials(const aiScene* scene, std::vector<MaterialEntry>& materials)
    {
        materials.resize(scene->mNumMaterials);

        for (uint32_t i = 0; i < static_cast<uint32_t>(materials.size()); ++i)
        {
            auto& mat = scene->mMaterials[i];
            MaterialEntry entry{};

            aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
            mat->Get(AI_MATKEY_BASE_COLOR, baseColor);
            std::memcpy(entry.baseColor, &baseColor, sizeof baseColor);

            float roughness = 0.0f;
            mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            entry.roughness = roughness;

            float metallic = 0.0f;
            mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
            entry.metallic = metallic;

            aiString path;
            if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS)
                copyTexturePath(entry.albedoPath, path);
            else entry.albedoPath[0] = 0;

            if (mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
                copyTexturePath(entry.normalPath, path);
            else entry.normalPath[0] = 0;

            if (mat->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS)
                copyTexturePath(entry.mrPath, path);
            else entry.mrPath[0] = 0;

            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS)
                copyTexturePath(entry.emissivePath, path);
            else entry.emissivePath[0] = 0;

            materials[i] = entry;
        }
    }

    static void processAnimations(
        const aiScene* scene, std::vector<BoneEntry>& bones,
        const std::unordered_map<std::string, uint32_t>& boneNameToIndex,
        std::vector<uint8_t>& keyframeBlob,
        std::vector<AnimationChannel>& channels, 
        std::vector<AnimationHeader>& headers)
    {
        headers.resize(scene->mNumAnimations);

        for (uint32_t a = 0; a < scene->mNumAnimations; ++a)
        {
            const auto* anim = scene->mAnimations[a];

            AnimationHeader header{};
            copyString(header.name, anim->mName);

            float ticksPerSecond = anim->mTicksPerSecond > 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

            header.duration = static_cast<float>(anim->mDuration) / ticksPerSecond;
            header.channelIndex = static_cast<uint32_t>(channels.size());
            header.channelCount = 0;


            for (uint32_t c = 0; c < anim->mNumChannels; ++c)
            {
                const auto* chan = anim->mChannels[c];

                auto it = boneNameToIndex.find(chan->mNodeName.C_Str());
                if (it == boneNameToIndex.end()) continue;


                std::vector<AnimationKey> keys;
                std::vector<double> times;
                times.reserve(chan->mNumPositionKeys + chan->mNumRotationKeys + chan->mNumScalingKeys);
                for (uint32_t k = 0; k < chan->mNumPositionKeys; ++k) times.push_back(chan->mPositionKeys[k].mTime);
                for (uint32_t k = 0; k < chan->mNumRotationKeys; ++k) times.push_back(chan->mRotationKeys[k].mTime);
                for (uint32_t k = 0; k < chan->mNumScalingKeys; ++k) times.push_back(chan->mScalingKeys[k].mTime);

                // Sort frame times and remove all almost identical
                std::sort(times.begin(), times.end());
                times.erase(std::unique(times.begin(), times.end(),
                    [](double x, double y) { return std::abs(x - y) < ai_epsilon; }), times.end());

                uint32_t n = static_cast<uint32_t>(times.size());
                if (n == 0) continue;

                for (uint32_t k = 0; k < n; ++k)
                {
                    AnimationKey key{};

                    double t = times[k];

                    aiVector3D bindPos, bindScale;
                    aiQuaternion bindRot;
                    {
                        aiMatrix4x4 m;
                        memcpy(&m, bones[it->second].bindLocalTransform, sizeof(float) * 16);
                        m.Transpose(); 
                        m.Decompose(bindScale, bindRot, bindPos);
                    }

                    auto p = sampleVec3(chan->mPositionKeys, chan->mNumPositionKeys, t, bindPos);
                    auto r = sampleQuat(chan->mRotationKeys, chan->mNumRotationKeys, t, bindRot);
                    auto s = sampleVec3(chan->mScalingKeys, chan->mNumScalingKeys, t, bindScale);

                    
                    key.time = static_cast<float>(t) / ticksPerSecond;
                    key.position[0] = p.x;
                    key.position[1] = p.y;
                    key.position[2] = p.z;
                    key.rotation[0] = r.x;
                    key.rotation[1] = r.y;
                    key.rotation[2] = r.z;
                    key.rotation[3] = r.w;
                    key.scale[0] = s.x;
                    key.scale[1] = s.y;
                    key.scale[2] = s.z;

                    keys.push_back(key);
                }
                const uint64_t base = keyframeBlob.size();

                AnimationChannel ac{};
                ac.boneIndex = it->second;
                ac.keyCount = n;
                ac.keyIndex = static_cast<uint32_t>(base / sizeof(AnimationKey));

                keyframeBlob.resize(base + n * sizeof(AnimationKey));
                memcpy(keyframeBlob.data() + base, keys.data(), n * sizeof(AnimationKey));

               
                channels.push_back(ac);
                header.channelCount++;
            }

            headers[a] = header;
        }
    }

    ModelWriter::~ModelWriter() = default;

    ModelWriter::ModelWriter(const std::string& input, const std::string& output) : m_ModelData(new ModelData)
    {
        if (output.empty())
            throw std::runtime_error("[ModelWriter] Output name is empty.");

        if (output.find('/') != std::string::npos || output.find('\\') != std::string::npos)
            throw std::runtime_error("[ModelWriter] Output name must not contain a path separator.");

        m_ModelData->path = std::filesystem::path(input).parent_path().generic_string();
        m_ModelData->name = output;
        std::replace(m_ModelData->path.begin(), m_ModelData->path.end(), '\\', '/');

        const aiScene* scene = m_ModelData->importer.ReadFile(input,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_GenUVCoords |
            aiProcess_FlipUVs |
            aiProcess_LimitBoneWeights);

        if (!scene || !scene->mRootNode)
            throw std::runtime_error(m_ModelData->importer.GetErrorString());

        processModel(scene,m_ModelData->boneNameToIndex, m_ModelData->meshes,
            m_ModelData->skinnedVertices,m_ModelData->staticVertices,
            m_ModelData->indices, m_ModelData->bones);

        processMaterials(scene, m_ModelData->materials);
        processAnimations(scene, m_ModelData->bones, m_ModelData->boneNameToIndex, m_ModelData->keyframeBlob, m_ModelData->channels, m_ModelData->animationHeaders);
    }

    void ModelWriter::operator()()
    {
        const std::string modelFmt = ".model";

        uint64_t next_offset = 0;

        // 1
        ModelHeader header{};
        header.magic = MODEL_MAGIC;
        
        if (m_ModelData->meshes.empty()) throw std::runtime_error("Current file had 0 meshes!");
        
        // 2
        header.meshCount = static_cast<uint32_t>(m_ModelData->meshes.size());
        header.meshTableOffset = sizeof(ModelHeader);
      
        next_offset = header.meshTableOffset + header.meshCount * sizeof(MeshEntry);

        if (!m_ModelData->channels.empty())
        {
            // 3
            header.channelTableSize = static_cast<uint64_t>(m_ModelData->channels.size()) * sizeof(AnimationChannel);
            header.channelTableOffset = next_offset;

            next_offset += header.channelTableSize;
        }

        if (!m_ModelData->animationHeaders.empty())
        {
            // 4
            header.animationCount = static_cast<uint32_t>(m_ModelData->animationHeaders.size());
            header.animationTableOffset = next_offset;

            next_offset += header.animationCount * sizeof(AnimationHeader);
        }
        
        if (!m_ModelData->staticVertices.empty()) 
        {
            //5
            header.staticVertexBlobSize = static_cast<uint64_t>(m_ModelData->staticVertices.size()) * sizeof(StaticVertex);
            header.staticVertexBlobOffset = next_offset;

            next_offset += header.staticVertexBlobSize;
        }
        if (!m_ModelData->skinnedVertices.empty())
        {
            //6
            header.skinnedVertexBlobSize = static_cast<uint64_t>(m_ModelData->skinnedVertices.size()) * sizeof(SkinnedVertex);
            header.skinnedVertexBlobOffset = next_offset;

            next_offset += header.skinnedVertexBlobSize;
        }

        if (!m_ModelData->materials.empty()) 
        {
            // 7
            header.materialCount = static_cast<uint32_t>(m_ModelData->materials.size());
            header.materialTableOffset = next_offset;

            next_offset += header.materialCount * sizeof(MaterialEntry);
        }

        if (!m_ModelData->bones.empty())
        {
            // 8
            header.boneCount = static_cast<uint32_t>(m_ModelData->bones.size());
            header.boneTableOffset = next_offset;

            next_offset += header.boneCount * sizeof(BoneEntry);
        }

        if (!m_ModelData->indices.empty())
        {
            // 9
            header.indexBlobSize = static_cast<uint64_t>(m_ModelData->indices.size()) * sizeof(uint32_t);
            header.indexBlobOffset = next_offset;

            next_offset += header.indexBlobSize;
        }
        
        if (!m_ModelData->keyframeBlob.empty()) 
        {
            header.keyframeBlobSize = static_cast<uint64_t>(m_ModelData->keyframeBlob.size());
            header.keyframeBlobOffset = next_offset;

            next_offset += header.keyframeBlobSize;
        }

        std::string folderName = m_ModelData->name;
        
        std::string modelFileName = folderName + "/" + m_ModelData->name + modelFmt;

        std::filesystem::create_directories(folderName);

        for (auto& mat : m_ModelData->materials) 
        {
            copyTextureFile(m_ModelData->path, folderName, mat.albedoPath);
            copyTextureFile(m_ModelData->path, folderName, mat.normalPath);
            copyTextureFile(m_ModelData->path, folderName, mat.mrPath);
            copyTextureFile(m_ModelData->path, folderName, mat.emissivePath);
        }

        std::ofstream out(modelFileName, std::ios::binary | std::ios::out);

        if (!out) throw std::runtime_error("Failed to open " + modelFileName);

        out.write(reinterpret_cast<const char*>(&header), sizeof header);
        out.write(reinterpret_cast<const char*>(m_ModelData->meshes.data()), header.meshCount * sizeof(MeshEntry));

        if (!m_ModelData->channels.empty())
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->channels.data()), header.channelTableSize);
        }
        
        if (!m_ModelData->animationHeaders.empty()) 
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->animationHeaders.data()), header.animationCount * sizeof(AnimationHeader));
        }
       
        if (!m_ModelData->staticVertices.empty())
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->staticVertices.data()), header.staticVertexBlobSize);
        }
        if (!m_ModelData->skinnedVertices.empty())
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->skinnedVertices.data()), header.skinnedVertexBlobSize);
        }

        if (!m_ModelData->materials.empty()) 
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->materials.data()), header.materialCount * sizeof(MaterialEntry));
        }
        if (!m_ModelData->bones.empty()) 
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->bones.data()), header.boneCount * sizeof(BoneEntry));
        }

        if (!m_ModelData->indices.empty()) 
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->indices.data()), header.indexBlobSize);
        }

        if (!m_ModelData->keyframeBlob.empty()) 
        {
            out.write(reinterpret_cast<const char*>(m_ModelData->keyframeBlob.data()), header.keyframeBlobSize);
        }

        out.flush();

        if (!out.good()) throw std::runtime_error("Failed to write: " + modelFileName);

        out.close();
    }
}