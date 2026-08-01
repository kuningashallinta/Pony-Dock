#pragma once

#include <string>
#include <vector>

struct PDMonitor
{
	std::string device;
	std::string label;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool primary = false;
};

std::vector<PDMonitor> enumerateMonitors();
