#include <QScript/QCompile.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cout << "usage: " << argv[0] << " <input.q> <output.qb>" << std::endl;
		return 0;
	}
	try
	{
		std::vector<unsigned char> out;
		{
			// Read in file
			std::ifstream file(argv[1]);
			if (!file.is_open())
			{
				std::cerr << "Failed to open input file" << std::endl;
				return 1;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();

			// Decompile
			out = QScript::Compile(buffer.str());
		}

		// Write out file
		std::ofstream outFile(argv[2], std::ios::binary);
		if (!outFile.is_open())
		{
			std::cerr << "Failed to open output file" << std::endl;
			return 1;
		}
		outFile.write((const char*)out.data(), out.size());
	}
	catch (const std::exception &e)
	{
		std::cerr << "QB compilation failed: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
