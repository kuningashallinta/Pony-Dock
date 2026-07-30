#pragma once

#include <Library/PDPonyPackData.h>

#include <string>

bool loadPonyPack(std::string const &packPath, PDPonyPackData &outData, std::string &outError);
