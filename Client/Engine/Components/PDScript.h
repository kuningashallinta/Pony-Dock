#pragma once

#include <sol/sol.hpp>

#include <string>
#include <unordered_map>

struct PDScript
{
	struct Module
	{
		int errorCount = 0;
		bool spawned = false;
	};

	sol::table self;
	std::unordered_map<std::string, Module> modules;
};
