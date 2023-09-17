#include "QBinary.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace QScript
{
	char *SkipToken(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[SkipToken] Unexpected start of file");
		if (p_token >= p_end)
			throw std::runtime_error("[SkipToken] Unexpected end of file");

		switch ((Token)*p_token)
		{
			case Token::EndOfFile:
			{
				return nullptr;
			}
			case Token::EndOfLine:
			case Token::Equals:
			case Token::Dot:
			case Token::Comma:
			case Token::Minus:
			case Token::Add:
			case Token::Divide:
			case Token::Multiply:
			case Token::OpenParenth:
			case Token::CloseParenth:
			case Token::SameAs:
			case Token::LessThan:
			case Token::LessThanEqual:
			case Token::GreaterThan:
			case Token::GreaterThanEqual:
			case Token::StartStruct:
			case Token::StartArray:
			case Token::EndStruct:
			case Token::EndArray:
			case Token::KeywordBegin:
			case Token::KeywordRepeat:
			case Token::KeywordBreak:
			case Token::KeywordScript:
			case Token::KeywordEndScript:
			case Token::KeywordIf:
			case Token::KeywordElse:
			case Token::KeywordElseIf:
			case Token::KeywordEndIf:
			case Token::KeywordReturn:
			case Token::KeywordAllArgs:
			case Token::Arg:
			case Token::Or:
			case Token::And:
			case Token::Xor:
			case Token::ShiftLeft:
			case Token::ShiftRight:
			case Token::KeywordRandomRange:
			case Token::KeywordRandomRange2:
			case Token::KeywordNot:
			case Token::KeywordAnd:
			case Token::KeywordOr:
			case Token::KeywordSwitch:
			case Token::KeywordEndSwitch:
			case Token::KeywordCase:
			case Token::KeywordDefault:
			case Token::Colon:
			{
				p_token++;
				break;
			}
			case Token::Name:
			case Token::Integer:
			case Token::HexInteger:
			case Token::Float:
			case Token::EndOfLineNumber:
			case Token::Jump:
			// case Token::RuntimeMemberFunction:
			// case Token::RuntimeCFunction:
			{
				p_token += 5;
				break;
			}
			case Token::Vector:
			{
				p_token += 13;
				break;
			}
			case Token::Pair:
			{
				p_token += 9;
				break;
			}
			case Token::String:
			case Token::LocalString:
			{
				p_token++;

				uint32_t num_bytes = GetUnsignedInteger(p_start, p_end, p_token);
				p_token += 4;
				p_token += num_bytes;
				break;
			}
			case Token::ChecksumName:
			{
				// Skip over the token and checksum.
				p_token += 5;

				// Skip over the string.
				while (*p_token != '\0')
					p_token++;
				p_token++;
				break;
			}
			case Token::KeywordRandom:
			case Token::KeywordRandom2:
			case Token::KeywordRandomNoRepeat:
			case Token::KeywordRandomPermute:
			{
				p_token++;

				uint32_t num_jumps = GetUnsignedInteger(p_start, p_end, p_token);
				p_token += 4;

				// Skip over all the weight & jump offsets.
				p_token += 2 * num_jumps + 4 * num_jumps;
				break;
			}
			case Token::FastIf:
			case Token::FastElse:
			case Token::ShortJump:
			{
				p_token += 3;
				break;
			}
			default:
			{
				throw std::runtime_error("[SkipToken] Unrecognized script token " + std::to_string((int)(unsigned char)*p_token) + " at " + std::to_string((int)(p_token - p_start)));
				break;
			}
		}
		return p_token;
	}

	int32_t GetSignedInteger(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetSignedInteger] Unexpected start of file");
		if (p_token + 4 > p_end)
			throw std::runtime_error("[GetSignedInteger] Unexpected end of file");
		return
			((int32_t)(unsigned char)p_token[0] << 0) |
			((int32_t)(unsigned char)p_token[1] << 8) |
			((int32_t)(unsigned char)p_token[2] << 16) |
			((int32_t)(unsigned char)p_token[3] << 24);
	}
	uint32_t GetUnsignedInteger(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetUnsignedInteger] Unexpected start of file");
		if (p_token + 4 > p_end)
			throw std::runtime_error("[GetUnsignedInteger] Unexpected end of file");
		return
			((uint32_t)(unsigned char)p_token[0] << 0) |
			((uint32_t)(unsigned char)p_token[1] << 8) |
			((uint32_t)(unsigned char)p_token[2] << 16) |
			((uint32_t)(unsigned char)p_token[3] << 24);
	}
	int16_t GetSignedShort(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetSignedShort] Unexpected start of file");
		if (p_token + 2 > p_end)
			throw std::runtime_error("[GetSignedShort] Unexpected end of file");
		return
			((int16_t)(unsigned char)p_token[0] << 0) |
			((int16_t)(unsigned char)p_token[1] << 8);
	}
	uint16_t GetUnsignedShort(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetUnsignedShort] Unexpected start of file");
		if (p_token + 2 > p_end)
			throw std::runtime_error("[GetUnsignedShort] Unexpected end of file");
		return
			((uint16_t)(unsigned char)p_token[0] << 0) |
			((uint16_t)(unsigned char)p_token[1] << 8);
	}
	float GetFloat(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetFloat] Unexpected start of file");
		if (p_token + 4 > p_end)
			throw std::runtime_error("[GetFloat] Unexpected end of file");
		return *(float*)(p_token); // TODO: how?
	}
	ptrdiff_t GetAddress_Relative(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetAddress_Relative] Unexpected start of file");
		if (p_token + 4 > p_end)
			throw std::runtime_error("[GetAddress_Relative] Unexpected end of file");
		int32_t address = GetSignedInteger(p_start, p_end, p_token);
		return (p_token + 4 + address) - p_start;
	}
	ptrdiff_t GetShortAddress_Relative(char *p_start, char *p_end, char *p_token)
	{
		if (p_token < p_start)
			throw std::runtime_error("[GetShortAddress_Relative] Unexpected start of file");
		if (p_token + 2 > p_end)
			throw std::runtime_error("[GetShortAddress_Relative] Unexpected end of file");
		int16_t address = GetSignedShort(p_start, p_end, p_token);
		return (p_token + address) - p_start;
	}

	std::unordered_map<ptrdiff_t, std::string> GetLabels(char *p_start, char *p_end, char *p_token)
	{
		std::unordered_map<ptrdiff_t, std::string> labels;
		while (p_token != nullptr)
		{
			char *p_base = SkipToken(p_start, p_end, p_token);

			// Check if this is a token with an address
			switch ((Token)*p_token)
			{
				case Token::FastIf:
				case Token::FastElse:
				case Token::ShortJump:
				{
					ptrdiff_t address = GetShortAddress_Relative(p_start, p_end, p_token + 1);
					if (labels.find(address) == labels.end())
						labels[address] = "LABEL_" + std::to_string(address) + ":";
					break;
				}
				case Token::KeywordRandom:
				case Token::KeywordRandom2:
				case Token::KeywordRandomNoRepeat:
				case Token::KeywordRandomPermute:
				{
					uint32_t num_jumps = GetUnsignedInteger(p_start, p_end, p_token + 1);
					if (num_jumps == 0)
					{
						ptrdiff_t address = (p_token + 5) - p_start;
						labels[address] = "RANDOMEND";
						break;
					}
					for (uint32_t i = 0; i < num_jumps; i++)
					{
						uint16_t weight = GetUnsignedShort(p_start, p_end, p_token + 5 + i * 2);
						ptrdiff_t address = GetAddress_Relative(p_start, p_end, p_token + 5 + 2 * num_jumps + 4 * i);
						if (num_jumps == 1)
							labels[address] = "RANDOMCASE RANDOMEND";
						else
							labels[address] = "RANDOMCASE";

						if (i > 0)
						{
							// Get last jump address
							char *p = p_start + address - 5;
							if (p < p_start || p >= p_end)
								throw std::runtime_error("[GetLabels] Unexpected end of file");
							if ((Token)*p == Token::Jump)
							{
								ptrdiff_t jump_address = GetAddress_Relative(p_start, p_end, p + 1);
								labels[jump_address] = "RANDOMEND";
							}
						}
					}
					break;
				}
				default:
					break;
			}

			// Skip over the token
			p_token = p_base;
		}
		return labels;
	}

	std::unordered_map<uint32_t, std::string> GetChecksumStrings(char *p_start, char *p_end, char *p_token)
	{
		std::unordered_map<uint32_t, std::string> checksum_strings;
		while (p_token != nullptr)
		{
			// Check if this is a checksum name token
			if ((Token)*p_token == Token::ChecksumName)
			{
				// Get checksum and name
				unsigned long checksum = GetUnsignedInteger(p_start, p_end, p_token + 1);
				std::string name;
				char *name_p = p_token + 5;
				while (*name_p)
				{
					name += *name_p;
					++name_p;
				}

				// Add to map
				// std::cout << "Found checksum " << checksum << " for name " << name << std::endl;
				checksum_strings[checksum] = name;
			}

			// Skip over the token
			p_token = SkipToken(p_start, p_end, p_token);
		}
		return checksum_strings;
	}
}
