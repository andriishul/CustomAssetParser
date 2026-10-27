#include "ModelWriter.hpp"

#include "ModelReader.hpp"

#include <iostream>
#include <algorithm>

using namespace std;

int main()
{
	std::cout << "Highly recommend resizing textures after parsing to <= 2048x2048 so " 
		<< "CopyContext does not hit its limit. For normal maps, 1024x1024 or even 512x512 is "
		<< "usually sufficient.\n";

	std::string lookFor;
	try 
	{
		string input, output;

		std::cout << "Input path:";
		std::cin >> input;
		std::cout << "Output name:";
		std::cin >> output;

		std::replace(input.begin(), input.end(), '\\', '/');

		asset::ModelWriter writer(input, output);
		writer();

		lookFor = output + "/" + output + ".model";

	}
	catch (const std::exception& e)
	{
		std::cout << "[ModelWriter]:" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	asset::ModelReader reader;
	
	reader(lookFor);

	return EXIT_SUCCESS;
}
