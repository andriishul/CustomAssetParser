#pragma once
#include "ModelFormat.hpp"

#include <string>
#include <memory>


namespace asset
{
	class ModelWriter 
	{
	public:
		/**
		 * @brief Constructs a ModelWriter to process a 3D model file.
		 * @param input Path to the source 3D model file (e.g., gltf - because mostly tested).
		 * @param output Name used to create the output folder and the resulting <name>.model file.
		 */
		ModelWriter(const std::string& input, const std::string& output);
		~ModelWriter();

		void operator()();
	private:
		struct ModelData;
		std::unique_ptr<ModelData> m_ModelData;
	};
}