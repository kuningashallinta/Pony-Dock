#pragma once

#include <string>
#include <vector>

class PDTexture;

struct PDAnimationFrame
{
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 1.0f;
	float v1 = 1.0f;
	float width = 0.0f;
	float height = 0.0f;
	float durationSeconds = 0.0f;
};

struct PDAnimationClip
{
	std::vector<PDAnimationFrame> frames;

	std::string atlasPath;
	PDTexture const *atlas = nullptr;

	float pivotX = 0.0f;
	float pivotY = 0.0f;

	bool loop = true;
	bool valid = false;
};
