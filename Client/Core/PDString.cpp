#include <Core/PDString.h>

#include <windows.h>

#include <algorithm>
#include <cctype>

std::string toLower(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character)
	{
		return static_cast<char>(std::tolower(character));
	});

	return text;
}

std::wstring toWide(std::string const &text)
{
	if (text.empty())
	{
		return std::wstring();
	}

	int const length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);

	if (length <= 0)
	{
		return std::wstring();
	}

	std::wstring wide(static_cast<std::size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), length);

	return wide;
}
