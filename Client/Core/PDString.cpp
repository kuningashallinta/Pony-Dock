#include <Core/PDString.h>

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
