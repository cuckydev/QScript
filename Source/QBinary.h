#pragma once

#include <unordered_map>
#include <string>

#include "QToken.h"

namespace QScript
{
	// Binary processing functions
	char *SkipToken(char *p_start, char *p_end, char *p_token);

	int32_t GetSignedInteger(char *p_start, char *p_end, char *p_token);
	uint32_t GetUnsignedInteger(char *p_start, char *p_end, char *p_token);
	int16_t GetSignedShort(char *p_start, char *p_end, char *p_token);
	uint16_t GetUnsignedShort(char *p_start, char *p_end, char *p_token);
	float GetFloat(char *p_start, char *p_end, char *p_token);
	ptrdiff_t GetAddress_Relative(char *p_start, char *p_end, char *p_token);
	ptrdiff_t GetShortAddress_Relative(char *p_start, char *p_end, char *p_token);

	std::unordered_map<ptrdiff_t, std::string> GetLabels(char *p_start, char *p_end, char *p_token);
	std::unordered_map<uint32_t, std::string> GetChecksumStrings(char *p_start, char *p_end, char *p_token);
}
