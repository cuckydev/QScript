#include <QScript/QDecompile.h>

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <limits>
#include <sstream>

#include "QBinary.h"
#include "QUtil.h"

namespace QScript
{
	// Decompile function
	std::string Decompile(void *start, void *end)
	{
		// Run through file
		std::stringstream out_stream;

		char *p_start = (char*)start;
		char *p_end = (char *)end;

		char *p_token = (char*)start;

		std::stringstream line;
		int tab_depth = 0;
		int pre_tab_depth = 0;
		int post_tab_depth = 0;

		bool is_arg = false;

		std::unordered_map<ptrdiff_t, std::string> labels = GetLabels(p_start, p_end, p_token);
		std::unordered_map<uint32_t, std::string> checksum_strings = GetChecksumStrings(p_start, p_end, p_token);

		while (p_token != nullptr)
		{
			// Print address
			// line << "(" << std::hex << (p_token - p_start) << std::dec << ") ";

			// Get token
			Token token = (Token)*p_token;
			char *p_base = SkipToken(p_start, p_end, p_token);

			// If there's a label here, print
			{
				ptrdiff_t address = p_token - p_start;
				auto it = labels.find(address);
				if (it != labels.end())
				{
					line << it->second << " ";
					if (it->second.find("RANDOMEND") != std::string::npos)
						pre_tab_depth--;
				}
			}
			
			// Write appropriate string for AST token
			switch (token)
			{
				case Token::EndOfFile:
				case Token::EndOfLine:
					// Process tabs
					if (pre_tab_depth < -post_tab_depth)
						tab_depth += pre_tab_depth + post_tab_depth;
					for (int i = 0; i < tab_depth; ++i)
						out_stream << "\t";
					if (pre_tab_depth > -post_tab_depth)
						tab_depth += post_tab_depth + pre_tab_depth;
					post_tab_depth = 0;
					pre_tab_depth = 0;
					if (tab_depth < 0)
						tab_depth = 0;

					// Write line
					out_stream << line.str() << std::endl;
					line.str("");
					break;
				case Token::StartStruct:
					line << "{ ";
					post_tab_depth++;
					break;
				case Token::EndStruct:
					line << "} ";
					pre_tab_depth--;
					break;
				case Token::StartArray:
					line << "[ ";
					post_tab_depth++;
					break;
				case Token::EndArray:
					line << "] ";
					pre_tab_depth--;
					break;
				case Token::Equals:
					line << "= ";
					break;
				case Token::Dot:
					line << ". ";
					break;
				case Token::Comma:
					line << ", ";
					break;
				case Token::Minus:
					line << "- ";
					break;
				case Token::Add:
					line << "+ ";
					break;
				case Token::Divide:
					line << "/ ";
					break;
				case Token::Multiply:
					line << "* ";
					break;
				case Token::OpenParenth:
					line << "( ";
					break;
				case Token::CloseParenth:
					line << ") ";
					break;
				case Token::DebugInfo:
					std::cout << "DEBUGINFO" << std::endl;
					break;
				case Token::SameAs:
					line << "== ";
					break;
				case Token::LessThan:
					line << "< ";
					break;
				case Token::LessThanEqual:
					line << "<= ";
					break;
				case Token::GreaterThan:
					line << "> ";
					break;
				case Token::GreaterThanEqual:
					line << ">= ";
					break;
				case Token::Name:
				{
					uint32_t checksum = GetUnsignedInteger(p_start, p_end, p_token + 1);
					
					if (is_arg)
						line << "<";
					else
						line << "(";

					auto find = checksum_strings.find(checksum);
					if (find != checksum_strings.end())
					{
						// Check if string contains any non identifier characters
						if (find->second.empty() || (find->second.front() >= '0' && find->second.front() <= '9') || find->second.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string::npos)
							line << "%\"" << find->second << "\"";
						else 
							line << find->second;
					}
					else
					{
						line << "0x" << std::hex << checksum << std::dec;
						std::cout << "WARNING: Could not find name for checksum 0x" << std::hex << checksum << std::dec << std::endl;
					}

					if (is_arg)
						line << ">";
					else
						line << ")";

					line << " ";
					is_arg = false;
					break;
				}
				case Token::Integer:
				{
					line << GetSignedInteger(p_start, p_end, p_token + 1) << " ";
					break;
				}
				case Token::HexInteger:
				{
					line << std::hex << "0x" << GetUnsignedInteger(p_start, p_end, p_token + 1) << std::dec << " ";
				}
				case Token::Enum:
				{
					std::cout << "ENUM" << std::endl;
					break;
				}
				case Token::Float:
				{
					line.precision(std::numeric_limits<float>::max_digits10 + 2);
					line << std::fixed << GetFloat(p_start, p_end, p_token + 1) << " ";
					break;
				}
				case Token::String:
				{
					uint32_t length = GetUnsignedInteger(p_start, p_end, p_token + 1);
					if (length)
						length--;
					line << "\"" << EscapeString(std::string(p_token + 5, length)) << "\" ";
					break;
				}
				case Token::LocalString:
				{
					uint32_t length = GetUnsignedInteger(p_start, p_end, p_token + 1);
					if (length)
						length--;
					line << "#\"" << EscapeString(std::string(p_token + 5, length)) << "\" ";
					break;
					break;
				}
				case Token::Array:
				{
					std::cout << "ARRAY" << std::endl;
					break;
				}
				case Token::Vector:
				{
					float x = GetFloat(p_start, p_end, p_token + 1);
					float y = GetFloat(p_start, p_end, p_token + 5);
					float z = GetFloat(p_start, p_end, p_token + 9);
					line << "VECTOR(" << x << ", " << y << ", " << z << ") ";
					break;
				}
				case Token::Pair:
				{
					float x = GetFloat(p_start, p_end, p_token + 1);
					float y = GetFloat(p_start, p_end, p_token + 5);
					line << "PAIR(" << x << ", " << y << ") ";
					break;
				}
				case Token::KeywordBegin:
				{
					line << "BEGIN ";
					post_tab_depth++;
					break;
				}
				case Token::KeywordRepeat:
				{
					line << "REPEAT ";
					tab_depth--;
					break;
				}
				case Token::KeywordBreak:
				{
					line << "BREAK ";
					break;
				}
				case Token::KeywordScript:
				{
					line << "SCRIPT ";
					post_tab_depth++;
					break;
				}
				case Token::KeywordEndScript:
				{
					line << "ENDSCRIPT" << std::endl;
					pre_tab_depth--;
					break;
				}
				case Token::KeywordIf:
				{
					line << "IF ";
					post_tab_depth++;
					break;
				}
				case Token::KeywordElse:
				{
					line << "ELSE ";
					tab_depth--;
					post_tab_depth++;
					break;
				}
				case Token::KeywordElseIf:
				{
					line << "ELSEIF ";
					pre_tab_depth--;
					post_tab_depth++;
					break;
				}
				case Token::KeywordEndIf:
				{
					line << "ENDIF ";
					pre_tab_depth--;
					break;
				}
				case Token::KeywordReturn:
				{
					line << "RETURN ";
					break;
				}
				case Token::Undefined:
				{
					std::cout << "UNDEFINED" << std::endl;
					break;
				}
				case Token::KeywordAllArgs:
				{
					line << "<...> ";
					break;
				}
				case Token::Arg:
				{
					is_arg = true;
					break;
				}
				case Token::Jump:
				{
					/*
					ptrdiff_t address = GetAddress_Relative(p_start, p_end, p_token + 1);

					std::string label = labels[address];
					if (label == "RANDOMEND")
						break;

					if (label.back() == ':')
						label.pop_back();

					line << "JUMP (" << label << ") ";
					*/
					break;
				}
				case Token::KeywordRandom:
				case Token::KeywordRandom2:
				case Token::KeywordRandomNoRepeat:
				case Token::KeywordRandomPermute:
				{
					uint32_t num_jumps = GetUnsignedInteger(p_start, p_end, p_token + 1);
					switch (token)
					{
						case Token::KeywordRandom:
							line << "RANDOM(";
							break;
						case Token::KeywordRandom2:
							line << "RANDOM2(";
							break;
						case Token::KeywordRandomNoRepeat:
							line << "RANDOM_NO_REPEAT(";
							break;
						case Token::KeywordRandomPermute:
							line << "RANDOM_PERMUTE(";
							break;
					}

					// Print jump weights
					for (uint32_t i = 0; i < num_jumps; ++i)
					{
						uint16_t weight = GetUnsignedShort(p_start, p_end, p_token + 5 + i * 2);
						line << (int)weight;
						if ((i + 1) < num_jumps)
							line << ", ";
					}
					line << ") ";

					post_tab_depth++;
					break;
				}
				case Token::KeywordRandomRange:
				{
					line << "RANDOM_RANGE ";
					break;
				}
				case Token::Or:
				{
					line << "| ";
					break;
				}
				case Token::And:
				{
					line << "& ";
					break;
				}
				case Token::Xor:
				{
					line << "^ ";
					break;
				}
				case Token::KeywordNot:
				{
					line << "NOT ";
					break;
				}
				case Token::KeywordAnd:
				{
					line << "AND ";
					break;
				}
				case Token::KeywordOr:
				{
					line << "OR ";
					break;
				}
				case Token::KeywordSwitch:
				{
					line << "SWITCH ";
					post_tab_depth += 2;
					break;
				}
				case Token::KeywordEndSwitch:
				{
					line << "ENDSWITCH ";
					pre_tab_depth -= 2;
					break;
				}
				case Token::KeywordCase:
				{
					line << "CASE ";
					tab_depth--;
					post_tab_depth++;
					break;
				}
				case Token::KeywordDefault:
				{
					line << "DEFAULT ";
					tab_depth--;
					post_tab_depth++;
					break;
				}
				case Token::Colon:
				{
					line << ": ";
					break;
				}
				case Token::FastIf:
				{
					line << "IF ";
					post_tab_depth++;
					/*
					ptrdiff_t address = GetShortAddress_Relative(p_start, p_end, p_token + 1);

					std::string label = labels[address];
					if (label.back() == ':')
						label.pop_back();

					line << "(" << label << ") ";
					*/
					break;
				}
				case Token::FastElse:
				{
					line << "ELSE ";
					tab_depth--;
					post_tab_depth++;
					/*
					ptrdiff_t address = GetShortAddress_Relative(p_start, p_end, p_token + 1);

					std::string label = labels[address];
					if (label.back() == ':')
						label.pop_back();

					line << "(" << label << ") ";
					*/
					break;
				}
				case Token::ShortJump:
				{
					/*
					line << "SHORTJUMP ";
					ptrdiff_t address = GetShortAddress_Relative(p_start, p_end, p_token + 1);

					std::string label = labels[address];
					if (label.back() == ':')
						label.pop_back();

					line << "(" << label << ") ";
					*/
					break;
				}
				default:
					break;
			}

			// Go to next token
			p_token = p_base;
		}

		return out_stream.str();
	}
}
