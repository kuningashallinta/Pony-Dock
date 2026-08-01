#pragma once

#include <imgui.h>

struct PDRowCard
{
	ImVec2 position;
	ImVec2 rectMax;
	float width = 0.0f;
	bool clicked = false;
	bool hovered = false;
};

void beginToolbar();
void endToolbar();
void toolbarSummary(const char *text);

bool beginModal(const char *title, ImVec2 size);
void endModal();

PDRowCard beginRowCard(float height, bool selected);
void endRowCard(PDRowCard const &card, float height, float gap);
