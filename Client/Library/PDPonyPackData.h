#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct PDPonyAnimationRef
{
	std::string left;
	std::string right;
};

struct PDPonyBehaviorData
{
	std::string id;
	std::string name;
	std::string animation;
	std::string movement;
	std::string linkedBehavior;

	float chance = 0.0f;
	float durationMinSeconds = 0.0f;
	float durationMaxSeconds = 0.0f;
	float speedPxPerSec = 0.0f;
	int group = 0;
	bool special = false;
	bool skip = false;
	bool preventAnimationLoop = false;
};

struct PDPonyPackData
{
	std::string id;
	std::string packPath;
	std::string previewPath;

	std::unordered_map<std::string, PDPonyAnimationRef> animations;
	std::vector<PDPonyBehaviorData> behaviors;
	bool valid = false;
};
