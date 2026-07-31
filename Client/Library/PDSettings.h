#pragma once

#include <string>
#include <vector>

enum class PDSettingKind
{
	Checkbox,
	Slider,
	Text,
	Dropdown,
	Button
};

enum class PDSettingValueType
{
	None,
	Boolean,
	Number,
	Text
};

struct PDSettingValue
{
	PDSettingValueType type = PDSettingValueType::None;
	bool boolean = false;
	float number = 0.0f;
	std::string text;
};

struct PDSettingDeclaration
{
	std::string id;
	std::string label;
	PDSettingKind kind = PDSettingKind::Checkbox;
	PDSettingValue defaultValue;
	float minimum = 0.0f;
	float maximum = 1.0f;
	std::vector<std::string> options;
};
