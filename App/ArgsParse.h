#pragma once

#include <string>
#include <unordered_map>
#include <iostream>

namespace ArgsParse
{
	struct ArgumentDef
	{
		std::string desc;
		std::string def;
		std::string file_ext;
		std::unordered_map<std::string, std::string> options;
		bool required = false;
	};

	void PrintHelp(const std::unordered_map<std::string, ArgumentDef> &def)
	{
		for (const auto &arg : def)
		{
			std::cout << "  ";
			if (!arg.second.required)
				std::cout << "(optional) ";
			std::cout << "-" << arg.first;
			if (!arg.second.file_ext.empty())
				std::cout << " <*." << arg.second.file_ext << ">";
			std::cout << std::endl;
			std::cout << "    " << arg.second.desc << std::endl;
			if (!arg.second.def.empty())
				std::cout << "    Default: " << arg.second.def << std::endl;
			if (!arg.second.options.empty())
			{
				std::cout << "    Options:" << std::endl;
				for (const auto &opt : arg.second.options)
					std::cout << "      " << opt.first << "\n        " << opt.second << std::endl;
			}
		}
	}

	std::unordered_map<std::string, std::string> Parse(int argc, char *argv[], const std::unordered_map<std::string, ArgumentDef> &def)
	{
		// Check if help is requested
		if (argc <= 1)
		{
			std::cout << "Usage: " << argv[0] << std::endl;
			PrintHelp(def);
			return {};
		}

		// Get default arguments
		std::unordered_map<std::string, std::string> args;
		for (const auto &arg : def)
		{
			if (!arg.second.def.empty())
				args[arg.first] = arg.second.def;
		}

		// Parse arguments
		auto current_arg = def.cend();

		for (int i = 1; i < argc; i++)
		{
			if (current_arg == def.cend())
			{
				// Get argument name
				std::string arg = argv[i];
				if (arg.empty() || arg[0] != '-')
				{
					std::cout << "Expected argument, got: " << arg << std::endl;
					return {};
				}
				arg = arg.substr(1);

				// Find argument definition
				auto it = def.find(arg);
				if (it == def.cend())
				{
					std::cout << "Unknown argument: " << arg << std::endl;
					return {};
				}

				// Check if this is a long argument
				if (!it->second.file_ext.empty() || !it->second.options.empty())
					current_arg = it;
				else
					args[arg];
			}
			else
			{
				// Get argument value
				std::string val = argv[i];

				// Check if this is a valid option
				if (!current_arg->second.options.empty() && current_arg->second.options.find(val) == current_arg->second.options.end())
				{
					std::cout << "Invalid option for argument " << current_arg->first << ": " << val << std::endl;
					return {};
				}

				// Set argument value
				args[current_arg->first] = argv[i];
				current_arg = def.cend();
			}
		}

		// Check if we cut off a required argument value
		if (current_arg != def.cend())
		{
			std::cout << "Missing value for argument: " << current_arg->first << std::endl;
			return {};
		}

		// Check if any required arguments are missing
		for (const auto &arg : def)
		{
			if (arg.second.required && args.find(arg.first) == args.end())
			{
				std::cout << "Missing required argument: " << arg.first << std::endl;
				return {};
			}
		}

		return args;
	}
}
