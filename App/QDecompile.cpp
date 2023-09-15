#include <QScript/QDecompile.h>

#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cout << "usage: " << argv[0] << " <input.qb> <output.q>" << std::endl;
		return 0;
	}
	try
	{
		std::string out;
		{
			// Read in file
			std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
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
		std::ofstream outFile(argv[2]);
		if (!outFile.is_open())
		{
			std::cerr << "Failed to open output file" << std::endl;
			return 1;
		}
		outFile.write(out.c_str(), out.size());
	}
	catch (const std::exception &e)
	{
		std::cerr << "QB decompilation failed: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
