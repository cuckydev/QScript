#include <QScript/QCompile.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include "ArgsParse.h"

int main(int argc, char *argv[])
{
	// Parse arguments
	static const std::unordered_map<std::string, ArgsParse::ArgumentDef> args_def = {
		{ "input", { "Input script", "", "q", {}, true}},
		{ "output", { "Output binary", "", "qb", {}, true}},
		{ "target", { "Script target", "", "", { { "thug1", "Tony Hawk's Underground" }, {"thug2", "Tony Hawk's Underground 2"} }, true}},
	};
	std::unordered_map<std::string, std::string> args = ArgsParse::Parse(argc, argv, args_def);
	if (args.empty())
		return 0;

	try
	{
		std::vector<unsigned char> out;
		{
			// Read in file
			std::ifstream file(args["input"]);
			if (!file.is_open())
			{
				std::cerr << "Failed to open input file" << std::endl;
				return 1;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();

			// Select target
			QScript::Target target;
			if (args["target"] == "thug1")
				target = QScript::Target::THUG1;
			else if (args["target"] == "thug2")
				target = QScript::Target::THUG2;
			else
			{
				std::cerr << "Invalid target" << std::endl;
				return 1;
			}

			// Compile
			out = QScript::Compile(buffer.str(), target);
		}

		// Write out file
		std::ofstream outFile(args["output"], std::ios::binary);
		if (!outFile.is_open())
		{
			std::cerr << "Failed to open output file" << std::endl;
			return 1;
		}
		outFile.write((const char*)out.data(), out.size());
	}
	catch (const std::exception &e)
	{
		std::cerr << "QScript compilation failed: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
