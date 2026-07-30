#pragma once

#include <string>

struct PDSceneEntry
{
	std::string id;
	std::string displayName;
	std::string previewPath;
	std::string packPath;
	int quantity = 1;
};
