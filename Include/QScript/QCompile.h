#pragma once

#include <string>
#include <vector>

namespace QScript
{
	// Compile function
	std::vector<unsigned char> Compile(const std::string &source);
}
