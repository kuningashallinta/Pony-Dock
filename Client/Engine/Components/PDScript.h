#pragma once

#include <sol/sol.hpp>

#include <string>

struct PDScript
{
	std::string path;
	sol::table self;
	int errorCount = 0;
	bool spawned = false;
};
