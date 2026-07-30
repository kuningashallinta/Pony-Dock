#pragma once

#include <Engine/PDAnimationClip.h>
#include <Engine/PDTextureCache.h>

#include <string>
#include <unordered_map>

class PDDiagnostics;

class PDAnimationCache
{
public:
	void initialize(PDTextureCache &textures, PDDiagnostics &diagnostics);

	PDAnimationClip const *clip(std::string const &animPath);

private:
	PDTextureCache *m_textures = nullptr;
	PDDiagnostics *m_diagnostics = nullptr;
	std::unordered_map<std::string, PDAnimationClip> m_clips;
};
