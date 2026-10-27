#pragma once
#include "ModelFormat.hpp"

#include <fstream>
#include <string>
#include <vector>



namespace asset
{
	struct Model 
	{
		std::vector<MeshEntry> meshes;

		std::vector<StaticVertex> staticVertices;
		std::vector<SkinnedVertex> skinnedVertices;

		std::vector<uint32_t> indices;

		std::vector<MaterialEntry> materials;
		std::vector<BoneEntry> bones;
		
		std::vector<AnimationChannel> channels;
		std::vector<AnimationHeader> animations;
		std::vector<AnimationKey> frames;
	};

	class ModelReader 
	{
	public:
		ModelReader() = default;

		inline Model operator()(const std::string& file) 
		{
			Model model{};

			std::ifstream fs(file, std::ios::ate | std::ios::binary);
			if (!fs.is_open()) throw std::runtime_error("[ModelReader] Failed to open: " + file);
			auto end = fs.tellg();
			fs.seekg(0, std::ios::beg);
			std::vector<char> bytes(end, 0);
			fs.read(bytes.data(), end);
			fs.close();

			ModelHeader header{};
			memcpy(&header, bytes.data(), sizeof header);

			if (header.magic != MODEL_MAGIC) throw std::runtime_error("[ModelReader] Model format not recognized.");

			model.meshes.resize(header.meshCount);
			memcpy(model.meshes.data(), bytes.data() + header.meshTableOffset, header.meshCount * sizeof(MeshEntry));
			
			model.animations.resize(header.animationCount);
			memcpy(model.animations.data(), bytes.data() + header.animationTableOffset, header.animationCount * sizeof(AnimationHeader));

			model.channels.resize(header.channelTableSize / sizeof(AnimationChannel));
			memcpy(model.channels.data(), bytes.data() + header.channelTableOffset, header.channelTableSize);

			model.staticVertices.resize(header.staticVertexBlobSize / sizeof(StaticVertex));
			memcpy(model.staticVertices.data(), bytes.data() + header.staticVertexBlobOffset, header.staticVertexBlobSize);

			model.skinnedVertices.resize(header.skinnedVertexBlobSize / sizeof(SkinnedVertex));
			memcpy(model.skinnedVertices.data(), bytes.data() + header.skinnedVertexBlobOffset, header.skinnedVertexBlobSize);

			model.materials.resize(header.materialCount);
			memcpy(model.materials.data(), bytes.data() + header.materialTableOffset, header.materialCount * sizeof(MaterialEntry));

			model.bones.resize(header.boneCount);
			memcpy(model.bones.data(), bytes.data() + header.boneTableOffset, header.boneCount * sizeof(BoneEntry));

			model.frames.resize(header.keyframeBlobSize / sizeof(AnimationKey));
			memcpy(model.frames.data(), bytes.data() + header.keyframeBlobOffset, header.keyframeBlobSize);

			model.indices.resize(header.indexBlobSize / sizeof(uint32_t));
			memcpy(model.indices.data(), bytes.data() + header.indexBlobOffset, header.indexBlobSize);
			
			if (!model.indices.size()) throw std::runtime_error("[ModelReader] No indices in: " + file);

			return model;
		}
	};
}