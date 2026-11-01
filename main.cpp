#include "ModelWriter.hpp"

#include "ModelReader.hpp"

#include <iostream>
#include <algorithm>
#include <objbase.h>

using namespace std;

int main()
{
	std::cout << "Highly recommend resizing textures after parsing to <= 2048x2048 so " 
		<< "CopyContext does not hit its limit. For normal maps, 1024x1024 or even 512x512 is "
		<< "usually sufficient.\n";
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	std::string lookFor;
	try 
	{
		string input, output;

	/*	std::cout << "Input path:";
		std::cin >> input;
		std::cout << "Output name:";
		std::cin >> output;

		std::replace(input.begin(), input.end(), '\\', '/');*/
	
		std::string Ins[] =
		{
			"C:/Users/vulka/Documents/3D_Models/readonly/macabre_gargoyle_statue/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/electric_floor_lamp/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/graveyard_angel_statue/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/japanese_house/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/pelter_-_artillery_mech/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/robot/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/robot_police_unit_animated/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/steampunk_robot/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/stone_floor/Untitled.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/stone_wall_scan/scene.gltf",
			"C:/Users/vulka/Documents/3D_Models/readonly/wooden_floor/scene.gltf"
		};

		const std::string Outs[] =
		{
			"gargoyle_statue",
			"lamp",
			"graveyard_angel_statue",
			"japanese_house",
			"artillery_mech",
			"robot",
			"robot_police",
			"steampunk_robot",
			"stone_floor",
			"stone_wall",
			"wooden_floor"
		};


		for (uint32_t i = 0; i < 11; ++i) 
		{
			asset::ModelWriter writer(Ins[i], Outs[i]);
			writer();

			lookFor = Outs[i] + "/" + Outs[i] + ".model";
			asset::ModelReader reader;
			reader(lookFor);
		}

		

	}
	catch (const std::exception& e)
	{
		std::cout << "[ModelWriter]:" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	CoUninitialize();
	return EXIT_SUCCESS;
}
