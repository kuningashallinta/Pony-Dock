#pragma once

class PDTexture;

struct PDSprite
{
	PDTexture const *texture = nullptr;
	float width = 64.0f;
	float height = 64.0f;
};
