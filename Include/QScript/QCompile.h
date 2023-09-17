#pragma once

#include <string>
#include <vector>

namespace QScript
{
	// Targets
	enum class Target
	{
		THUG1,
		THUG2,
	};

	// Compile function
	std::vector<unsigned char> Compile(const std::string &source, Target target);
}
