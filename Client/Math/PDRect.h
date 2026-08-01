#pragma once

struct PDRect
{
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;

	bool operator==(PDRect const &other) const = default;
};
