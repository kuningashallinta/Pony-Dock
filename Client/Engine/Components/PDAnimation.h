#pragma once

#include <string>

struct PDAnimationClip;

struct PDAnimation
{
	PDAnimationClip const *clip = nullptr;
	std::string name;
	int frame = 0;
	float elapsedSeconds = 0.0f;
	bool loop = true;
	bool finished = false;
	bool facingRight = true;
};
