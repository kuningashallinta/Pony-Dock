#pragma once

#include <string>

class PDTexture;

struct PDLabel
{
	PDTexture const *texture = nullptr;
	std::string text;
};
