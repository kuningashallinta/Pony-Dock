#pragma once

#include <Engine/PDAnimationClip.h>

#include <string>

bool loadAnimationClip(std::string const &animPath, PDAnimationClip &outClip, std::string &outError);
