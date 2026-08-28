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

struct PDPonyBehaviorGroup
{
	int id = 0;
	std::string name;
};

struct PDPonyInteractionData
{
	std::string id;
	std::string activation;

	std::vector<std::string> targets;
	std::vector<std::string> behaviors;

	float chance = 0.0f;
	float proximityPx = 0.0f;
	float reactivationDelaySeconds = 0.0f;
};

struct PDPonyPackData
{
	std::string id;
	std::string name;
	std::string packPath;
	std::string previewPath;

	std::unordered_map<std::string, PDPonyAnimationRef> animations;
	std::vector<PDPonyBehaviorData> behaviors;
	std::vector<PDPonyBehaviorGroup> groups;
	std::vector<PDPonyInteractionData> interactions;
	bool valid = false;
};
