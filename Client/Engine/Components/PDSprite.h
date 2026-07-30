#pragma once

class PDTexture;

struct PDSprite
{
	PDTexture const *texture = nullptr;
	float width = 64.0f;
	float height = 64.0f;
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 1.0f;
	float v1 = 1.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
};
