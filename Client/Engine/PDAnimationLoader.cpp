#include <Engine/PDAnimationLoader.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace
{
	constexpr float MinFrameSeconds = 0.001f;

	bool readSize(nlohmann::json const &value, float &outWidth, float &outHeight)
	{
		if (not value.is_array() or value.size() != 2)
		{
			return false;
		}

		if (not value[0].is_number() or not value[1].is_number())
		{
			return false;
		}

		outWidth = value[0].get<float>();
		outHeight = value[1].get<float>();

		return outWidth > 0.0f and outHeight > 0.0f;
	}
} // namespace

bool loadAnimationClip(std::string const &animPath, PDAnimationClip &outClip, std::string &outError)
{
	outClip = PDAnimationClip();

	std::ifstream stream(animPath, std::ios::binary);

	if (not stream)
	{
		outError = "cannot open " + animPath;

		return false;
	}

	nlohmann::json document;

	try
	{
		stream >> document;
	}
	catch (nlohmann::json::parse_error const &error)
	{
		outError = std::string("parse error in ") + animPath + ": " + error.what();

		return false;
	}

	if (not document.is_object())
	{
		outError = animPath + " is not a JSON object";

		return false;
	}

	float atlasWidth = 0.0f;
	float atlasHeight = 0.0f;

	if (not document.contains("atlasSize") or not readSize(document["atlasSize"], atlasWidth, atlasHeight))
	{
		outError = animPath + " has a missing or invalid atlasSize";

		return false;
	}

	if (not document.contains("atlas") or not document["atlas"].is_string())
	{
		outError = animPath + " has a missing or invalid atlas";

		return false;
	}

	std::filesystem::path const directory = std::filesystem::path(animPath).parent_path();
	outClip.atlasPath = (directory / document["atlas"].get<std::string>()).lexically_normal().string();

	if (not document.contains("frames") or not document["frames"].is_array() or document["frames"].empty())
	{
		outError = animPath + " has no frames";

		return false;
	}

	for (nlohmann::json const &frameValue : document["frames"])
	{
		if (not frameValue.is_object() or not frameValue.contains("rect"))
		{
			outError = animPath + " has a frame with no rect";

			return false;
		}

		nlohmann::json const &rect = frameValue["rect"];

		if (not rect.is_array() or rect.size() != 4)
		{
			outError = animPath + " has a frame with a malformed rect";

			return false;
		}

		float const x = rect[0].get<float>();
		float const y = rect[1].get<float>();
		float const width = rect[2].get<float>();
		float const height = rect[3].get<float>();

		if (width <= 0.0f or height <= 0.0f or x < 0.0f or y < 0.0f
			or x + width > atlasWidth or y + height > atlasHeight)
		{
			outError = animPath + " has a frame rect outside the atlas";

			return false;
		}

		float durationSeconds = MinFrameSeconds;

		if (frameValue.contains("durationMs") and frameValue["durationMs"].is_number())
		{
			durationSeconds = frameValue["durationMs"].get<float>() / 1000.0f;
		}

		PDAnimationFrame frame;
		frame.u0 = x / atlasWidth;
		frame.v0 = y / atlasHeight;
		frame.u1 = (x + width) / atlasWidth;
		frame.v1 = (y + height) / atlasHeight;
		frame.width = width;
		frame.height = height;
		frame.durationSeconds = durationSeconds > MinFrameSeconds ? durationSeconds : MinFrameSeconds;

		outClip.frames.push_back(frame);
	}

	outClip.loop = not document.contains("loop") or not document["loop"].is_boolean() or document["loop"].get<bool>();

	float pivotX = 0.0f;
	float pivotY = 0.0f;

	if (document.contains("pivot") and document["pivot"].is_array() and document["pivot"].size() == 2)
	{
		pivotX = document["pivot"][0].get<float>();
		pivotY = document["pivot"][1].get<float>();
	}

	if (pivotX == 0.0f and pivotY == 0.0f)
	{
		pivotX = outClip.frames.front().width * 0.5f;
		pivotY = outClip.frames.front().height * 0.5f;
	}

	outClip.pivotX = pivotX;
	outClip.pivotY = pivotY;
	outClip.valid = true;

	return true;
}
