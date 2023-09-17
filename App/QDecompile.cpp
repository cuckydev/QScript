#include <QScript/QDecompile.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "ArgsParse.h"

int main(int argc, char *argv[])
{
	// Parse arguments
	static const std::unordered_map<std::string, ArgsParse::ArgumentDef> args_def = {
		{ "input", { "Input script", "", "q", {}, true}},
		{ "output", { "Output binary", "", "qb", {}, true}},
	};
	std::unordered_map<std::string, std::string> args = ArgsParse::Parse(argc, argv, args_def);
	if (args.empty())
		return 0;

	try
	{
		std::string out;
		{
			// Read in file
			std::ifstream file(args["input"], std::ios::binary | std::ios::ate);
			if (!file.is_open())
			{
				std::cerr << "Failed to open input file" << std::endl;
				return 1;
			}

			size_t size = file.tellg();
			std::vector<char> data(size);

			file.seekg(0, std::ios::beg);
			file.read(data.data(), size);

			// Decompile
			out = QScript::Decompile(data.data(), data.data() + data.size());
		}

		// Write out file
		std::ofstream outFile(args["output"]);
		if (!outFile.is_open())
		{
			std::cerr << "Failed to open output file" << std::endl;
			return 1;
		}
		outFile.write(out.c_str(), out.size());
	}
	catch (const std::exception &e)
	{
		std::cerr << "QScript decompilation failed: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
