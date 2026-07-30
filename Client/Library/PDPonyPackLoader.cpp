#include <Library/PDPonyPackLoader.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

static std::string stringOr(nlohmann::json const &object, char const *key, std::string const &fallback)
{
	auto const found = object.find(key);

	return found != object.end() and found->is_string() ? found->get<std::string>() : fallback;
}

static float numberOr(nlohmann::json const &object, char const *key, float fallback)
{
	auto const found = object.find(key);

	return found != object.end() and found->is_number() ? found->get<float>() : fallback;
}

static int intOr(nlohmann::json const &object, char const *key, int fallback)
{
	auto const found = object.find(key);

	return found != object.end() and found->is_number_integer() ? found->get<int>() : fallback;
}

static bool boolOr(nlohmann::json const &object, char const *key, bool fallback)
{
	auto const found = object.find(key);

	return found != object.end() and found->is_boolean() ? found->get<bool>() : fallback;
}

static std::string absolutePath(std::filesystem::path const &packPath, std::string const &relative)
{
	if (relative.empty())
	{
		return std::string();
	}

	return (packPath / relative).lexically_normal().string();
}

bool loadPonyPack(std::string const &packPath, PDPonyPackData &outData, std::string &outError)
{
	outData = PDPonyPackData();

	std::filesystem::path const root(packPath);
	std::filesystem::path const jsonPath = root / "pony.json";

	std::ifstream stream(jsonPath, std::ios::binary);

	if (not stream)
	{
		outError = "cannot open " + jsonPath.string();

		return false;
	}

	nlohmann::json document;

	try
	{
		stream >> document;
	}
	catch (nlohmann::json::parse_error const &error)
	{
		outError = std::string("parse error in ") + jsonPath.string() + ": " + error.what();

		return false;
	}

	if (not document.is_object())
	{
		outError = jsonPath.string() + " is not a JSON object";

		return false;
	}

	outData.packPath = root.lexically_normal().string();
	outData.id = stringOr(document, "id", root.filename().string());
	outData.previewPath = absolutePath(root, stringOr(document, "preview", std::string()));

	auto const animations = document.find("animations");

	if (animations != document.end() and animations->is_object())
	{
		for (auto const &entry : animations->items())
		{
			if (not entry.value().is_object())
			{
				continue;
			}

			PDPonyAnimationRef reference;
			reference.left = absolutePath(root, stringOr(entry.value(), "left", std::string()));
			reference.right = absolutePath(root, stringOr(entry.value(), "right", std::string()));

			if (reference.left.empty() and reference.right.empty())
			{
				continue;
			}

			if (reference.left.empty())
			{
				reference.left = reference.right;
			}

			if (reference.right.empty())
			{
				reference.right = reference.left;
			}

			outData.animations.emplace(entry.key(), std::move(reference));
		}
	}

	if (outData.animations.empty())
	{
		outError = jsonPath.string() + " has no usable animations";

		return false;
	}

	auto const behaviors = document.find("behaviors");

	if (behaviors != document.end() and behaviors->is_array())
	{
		for (nlohmann::json const &entry : *behaviors)
		{
			if (not entry.is_object())
			{
				continue;
			}

			PDPonyBehaviorData behavior;
			behavior.id = stringOr(entry, "id", std::string());
			behavior.name = stringOr(entry, "name", behavior.id);
			behavior.animation = stringOr(entry, "animation", std::string());
			behavior.movement = stringOr(entry, "movement", "None");
			behavior.linkedBehavior = stringOr(entry, "linkedBehavior", std::string());
			behavior.chance = numberOr(entry, "chance", 0.0f);
			behavior.speedPxPerSec = numberOr(entry, "speedPxPerSec", 0.0f);
			behavior.group = intOr(entry, "group", 0);
			behavior.special = boolOr(entry, "special", false);
			behavior.skip = boolOr(entry, "skip", false);
			behavior.preventAnimationLoop = boolOr(entry, "preventAnimationLoop", false);

			auto const duration = entry.find("durationMs");

			if (duration != entry.end() and duration->is_object())
			{
				behavior.durationMinSeconds = numberOr(*duration, "min", 0.0f) / 1000.0f;
				behavior.durationMaxSeconds = numberOr(*duration, "max", 0.0f) / 1000.0f;
			}

			if (behavior.durationMaxSeconds < behavior.durationMinSeconds)
			{
				behavior.durationMaxSeconds = behavior.durationMinSeconds;
			}

			if (behavior.animation.empty() or outData.animations.find(behavior.animation) == outData.animations.end())
			{
				continue;
			}

			outData.behaviors.push_back(std::move(behavior));
		}
	}

	if (outData.behaviors.empty())
	{
		outError = jsonPath.string() + " has no usable behaviors";

		return false;
	}

	outData.valid = true;

	return true;
}
