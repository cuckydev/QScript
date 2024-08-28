#include <QScript/QCompile.h>

#define YY_NO_UNISTD_H
#include <Lexical/Lexical.h>

#include "QLexer.h"
#include "QUtil.h"

#include <iostream>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <stack>

namespace QScript
{
	// Target properties
	struct TargetProps
	{
		bool fast_if_else_case;
	};

	static const TargetProps s_target_props[] = {
		// THUG1
		{
			false,
		},
		// THUG2
		{
			true,
		},
	};

	// Compile function
	std::vector<unsigned char> Compile(const std::string &source, Target target)
	{
		// Get target properties
		const auto &target_props = s_target_props[(int)target];

		// Perform lexical analysis
		qscript_lex__scan_string(source.c_str());
		while (qscript_lex_lex()) {}
		qscript_lex_lex_destroy();

		// Process tokens
		std::vector<unsigned char> bytecode;

		auto add_token = [&bytecode](Token token)
			{
				bytecode.push_back((unsigned char)token);
			};

		auto add_short = [&bytecode](signed short value)
			{
				bytecode.push_back((unsigned char)((value >> 0) & 0xFF));
				bytecode.push_back((unsigned char)((value >> 8) & 0xFF));
			};

		auto add_int = [&bytecode](signed long value)
			{
				bytecode.push_back((unsigned char)((value >> 0) & 0xFF));
				bytecode.push_back((unsigned char)((value >> 8) & 0xFF));
				bytecode.push_back((unsigned char)((value >> 16) & 0xFF));
				bytecode.push_back((unsigned char)((value >> 24) & 0xFF));
			};

		auto add_real = [&bytecode](float value)
			{
				unsigned long raw = *(unsigned long*)&value;
				bytecode.push_back((unsigned char)((raw >> 0) & 0xFF));
				bytecode.push_back((unsigned char)((raw >> 8) & 0xFF));
				bytecode.push_back((unsigned char)((raw >> 16) & 0xFF));
				bytecode.push_back((unsigned char)((raw >> 24) & 0xFF));
			};

		auto add_string = [&bytecode](const std::string &str)
			{
				for (const auto &c : str)
					bytecode.push_back((unsigned char)c);
				bytecode.push_back(0);
			};

		auto add_string_sized = [&bytecode, &add_int, &add_string](const std::string &str)
			{
				add_int(str.size() + 1);
				add_string(str);
			};

		auto set_address = [&bytecode](size_t to, size_t address)
			{
				ptrdiff_t relative = (ptrdiff_t)address - (ptrdiff_t)(to + 4);
				bytecode.at(to + 0) = (unsigned char)((relative >> 0) & 0xFF);
				bytecode.at(to + 1) = (unsigned char)((relative >> 8) & 0xFF);
				bytecode.at(to + 2) = (unsigned char)((relative >> 16) & 0xFF);
				bytecode.at(to + 3) = (unsigned char)((relative >> 24) & 0xFF);
			};

		auto set_short_address = [&bytecode](size_t to, size_t address)
			{
				ptrdiff_t relative = (ptrdiff_t)address - (ptrdiff_t)(to);
				bytecode.at(to + 0) = (unsigned char)((relative >> 0) & 0xFF);
				bytecode.at(to + 1) = (unsigned char)((relative >> 8) & 0xFF);
			};

		auto get_token_integer = [&bytecode](const TokenBase &token) -> signed long
			{
				switch (token.type)
				{
					case Token::Integer:
					case Token::HexInteger:
					{
						const auto &integer = (const TokenNumber &)token;
						return integer.value;
						break;
					}
					case Token::Float:
					{
						const auto &real = (const TokenReal &)token;
						return (signed long)std::floor(real.value);
						break;
					}
					default:
						throw std::runtime_error("Expected integer or float");
				}
			};

		auto get_token_real = [&bytecode](const TokenBase &token) -> float
			{
				switch (token.type)
				{
					case Token::Integer:
					case Token::HexInteger:
					{
						const auto &integer = (const TokenNumber &)token;
						return (float)integer.value;
						break;
					}
					case Token::Float:
					{
						const auto &real = (const TokenReal &)token;
						return real.value;
						break;
					}
					default:
						throw std::runtime_error("Expected integer or float");
				}
			};

		std::unordered_map<unsigned long, std::string> checksums;

		std::unordered_map<std::string, unsigned long> labels;
		std::vector<std::pair<unsigned long, std::string>> label_refs;

		struct RandomStack // Keeps track of RandomCase jumps
		{
			size_t address = 0;
			size_t jump = 0, num_jumps = 0;
			std::vector<size_t> end_jumps;
		};
		std::stack<RandomStack> random_stack;

		struct ShortStack // Keeps track of FastIf, FastElse
		{
			size_t address = 0;
		};
		std::stack<ShortStack> short_stack;

		struct SwitchStack // Keeps track of Case jumps
		{
			std::vector<size_t> cases;
		};
		std::stack<SwitchStack> switch_stack;

		auto token_it = g_tokens.cbegin();
		auto token_end = g_tokens.cend();

		auto token_can_pop = [&token_it, &token_end]() -> bool
			{
			if (token_it == token_end)
				return false;
			return true;
		};

		auto token_pop = [&token_it, &token_end]() -> const std::unique_ptr<TokenBase>&
		{
			if (token_it == token_end)
				throw std::runtime_error("Unexpected end of script");
			const auto &token = *token_it;
			token_it++;
			return token;
		};

		auto token_peek = [&token_it, &token_end]() -> const std::unique_ptr<TokenBase>&
		{
			if (token_it == token_end)
				throw std::runtime_error("Unexpected end of script");
			return *token_it;
		};

		// Process tokens
		while (token_can_pop())
		{
			const auto &token = token_pop();
			if (token == nullptr)
				break;

			switch (token->type)
			{
				case Token::KeywordSwitch:
				{
					if (target_props.fast_if_else_case)
					{
						// Push switch stack
						SwitchStack stack;
						switch_stack.push(stack);
					}

					// Push Switch
					add_token(Token::KeywordSwitch);
					break;
				}
				case Token::KeywordEndSwitch:
				{
					if (target_props.fast_if_else_case)
					{
						// Get top of stack
						if (switch_stack.empty())
							throw std::runtime_error("Unexpected 'ENDSWITCH' (no 'SWITCH')");

						auto &switch_top = switch_stack.top();
						
						// Resolve jumps
						for (size_t i = 0; i < switch_top.cases.size(); i++)
						{
							size_t case_addr = switch_top.cases[i];

							// Set jump to next case
							if (i != switch_top.cases.size() - 1)
								set_short_address(case_addr + 2, switch_top.cases[i + 1]);
							else
								set_short_address(case_addr + 2, bytecode.size());

							// Set end jump address
							if (i != 0)
								set_short_address(case_addr - 2, bytecode.size() + 1);
						}

						switch_stack.pop();
					}

					// Push EndSwitch
					add_token(Token::KeywordEndSwitch);
					break;
				}
				case Token::KeywordCase:
				case Token::KeywordDefault:
				{
					if (target_props.fast_if_else_case)
					{
						// Get top of stack
						if (switch_stack.empty())
							throw std::runtime_error("Unexpected 'CASE' or 'DEFAULT' (no 'SWITCH')");

						auto &switch_top = switch_stack.top();

						// If this isn't the first case, add a jump to the end
						if (!switch_top.cases.empty())
						{
							add_token(Token::ShortJump);
							add_short(0);
						}

						// Push case address
						switch_top.cases.push_back(bytecode.size());

						// Push Case and short jump
						add_token(token->type);
						add_token(Token::ShortJump);
						add_short(0);
					}
					else
					{
						// Push Case
						add_token(token->type);
					}
					break;
				}
				case Token::KeywordIf:
				{
					if (target_props.fast_if_else_case)
					{
						// Push FastIf
						short_stack.emplace(ShortStack{ bytecode.size() });
						add_token(Token::FastIf);
						add_short(0);
					}
					else
					{
						// Push If
						add_token(Token::KeywordIf);
					}
					break;
				}
				case Token::KeywordElse:
				{
					if (target_props.fast_if_else_case)
					{
						// Set FastIf jump address
						if (short_stack.empty())
							throw std::runtime_error("Unexpected 'ELSE' (no 'IF')");

						auto &if_stack = short_stack.top();
						if (bytecode[if_stack.address] != (unsigned char)Token::FastIf)
							throw std::runtime_error("Unexpected 'ELSE' (no 'IF')");

						set_short_address(if_stack.address + 1, bytecode.size() + 3);
						short_stack.pop();

						// Push FastElse
						short_stack.emplace(ShortStack{ bytecode.size() });
						add_token(Token::FastElse);
						add_short(0);
					}
					else
					{
						// Push Else
						add_token(Token::KeywordElse);
					}
					break;
				}
				case Token::KeywordEndIf:
				{
					if (target_props.fast_if_else_case)
					{
						// Set FastIf/FastElse jump address
						if (short_stack.empty())
							throw std::runtime_error("Unexpected 'ENDIF' (no 'IF' or 'ELSE')");

						auto &if_stack = short_stack.top();
						if (bytecode[if_stack.address] != (unsigned char)Token::FastIf && bytecode[if_stack.address] != (unsigned char)Token::FastElse)
							throw std::runtime_error("Unexpected 'ELSE' (no 'IF' or 'ELSE')");

						set_short_address(if_stack.address + 1, bytecode.size() + 1);
						short_stack.pop();
					}

					// Push EndIf
					add_token(Token::KeywordEndIf);
					break;
				}
				case Token::Name:
				case Token::Arg:
				{
					// Get checksum of string
					const auto &str = (const TokenString&)*token;
					unsigned long crc = CRC(str.value.c_str());

					// Remember checksum name
					auto find = checksums.find(crc);
					if (find != checksums.end())
					{
						// Check if there's a collision
						if (SimpleString(find->second) != SimpleString(str.value))
							throw std::runtime_error("Checksum collision (" + find->second + " == " + str.value + ")");
					}
					else
					{
						// Set checksum
						checksums[crc] = str.value;
					}

					if (token->type == Token::Arg)
						add_token(Token::Arg);
					add_token(Token::Name);
					add_int(crc);
					break;
				}
				case Token::NameChecksum:
				case Token::ArgChecksum:
				{
					// Get checksum of string
					const auto &integer = (const TokenNumber &)*token;

					if (token->type == Token::ArgChecksum)
						add_token(Token::Arg);
					add_token(Token::Name);
					add_int(integer.value);
					break;
				}
				case Token::String:
				case Token::LocalString:
				{
					const auto &str = (const TokenString &)*token;
					add_token(token->type);
					add_string_sized(str.value);
					break;
				}
				case Token::Integer:
				case Token::HexInteger:
				{
					const auto &integer = (const TokenNumber &)*token;
					add_token(token->type);
					add_int(integer.value);
					break;
				}
				case Token::Float:
				{
					const auto &real = (const TokenReal &)*token;
					add_token(token->type);
					add_real(real.value);
					break;
				}
				case Token::Pair:
				{
					const auto &token_lp = token_pop();
					const auto &token_x = token_pop();
					const auto &token_comma = token_pop();
					const auto &token_y = token_pop();
					const auto &token_rp = token_pop();

					if (token_lp->type != Token::OpenParenth)
						throw std::runtime_error("Expected '('");
					if (token_comma->type != Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_rp->type != Token::CloseParenth)
						throw std::runtime_error("Expected ')'");

					add_token(Token::Pair);
					add_real(get_token_real(*token_x));
					add_real(get_token_real(*token_y));
					break;
				}
				case Token::Vector:
				{
					const auto &token_lp = token_pop();
					const auto &token_x = token_pop();
					const auto &token_comma_x = token_pop();
					const auto &token_y = token_pop();
					const auto &token_comma_y = token_pop();
					const auto &token_z = token_pop();
					const auto &token_rp = token_pop();

					if (token_lp->type != Token::OpenParenth)
						throw std::runtime_error("Expected '('");
					if (token_comma_x->type != Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_comma_y->type != Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_rp->type != Token::CloseParenth)
						throw std::runtime_error("Expected ')'");

					add_token(Token::Vector);
					add_real(get_token_real(*token_x));
					add_real(get_token_real(*token_y));
					add_real(get_token_real(*token_z));
					break;
				}
				case Token::EndOfLine:
				{
					// Don't add multiple end of lines in a row
					add_token(Token::EndOfLine);
					while (token_can_pop())
					{
						const auto &next_token = token_peek();
						if (next_token == nullptr || next_token->type != Token::EndOfLine)
							break;
						token_pop();
					}
					break;
				}
				case Token::KeywordRandom:
				case Token::KeywordRandom2:
				case Token::KeywordRandomNoRepeat:
				case Token::KeywordRandomPermute:
				{
					// Parse weight list
					const auto &token_lp = token_pop();
					if (token_lp->type != Token::OpenParenth)
						throw std::runtime_error("Expected '('");

					std::vector<signed long> weights;
					while (1)
					{
						// Grab number
						const auto &token_number = token_pop();
						if (token_number->type == Token::CloseParenth)
							break;
						weights.push_back(get_token_integer(*token_number));

						// Grab comma or close parenth
						const auto &token_next = token_pop();
						if (token_next->type == Token::CloseParenth)
							break;
						if (token_next->type != Token::Comma)
							throw std::runtime_error("Expected ',' or ')'");
					}

					// Create random stack
					RandomStack random;
					random.address = bytecode.size();
					random.jump = 0;
					random.num_jumps = weights.size();
					random_stack.push(random);

					// Push bytecode
					add_token(token->type);
					add_int(weights.size());
					for (const auto &weight : weights)
						add_short(weight);
					for (const auto &weight : weights)
						add_int(0);
					break;
				}
				case Token::KeywordRandomCase:
				{
					// Get top of stack
					if (random_stack.empty())
						throw std::runtime_error("Unexpected 'RANDOMCASE' (no random)");

					auto &random = random_stack.top();
					if (random.jump >= random.num_jumps)
						throw std::runtime_error("Unexpected 'RANDOMCASE' (more cases than weights)");

					// If this isn't the first jump, add a jump to the end
					if (random.jump != 0)
					{
						random.end_jumps.push_back(bytecode.size());
						add_token(Token::Jump);
						add_int(0);
					}

					// Push address to case
					size_t addr = random.address + 1 + 4 + 2 * random.num_jumps + 4 * random.jump;
					set_address(addr, bytecode.size());
					random.jump++;
					break;
				}
				case Token::KeywordRandomEnd:
				{
					// Get top of stack
					if (random_stack.empty())
						throw std::runtime_error("Unexpected 'RANDOMEND' (no random)");

					auto &random = random_stack.top();
					if (random.jump != random.num_jumps)
						throw std::runtime_error("Unexpected 'RANDOMEND' (random was incomplete)");

					// Set end jump addresses
					for (const auto &end_jump : random.end_jumps)
						set_address(end_jump + 1, bytecode.size());

					// Pop stack
					random_stack.pop();
					break;
				}
				default:
				{
					add_token(token->type);
					break;
				}
			}
		}
		g_tokens.clear();

		// Check if stacks are empty
		if (!switch_stack.empty())
			throw std::runtime_error("Unexpected end of script (missing 'ENDSWITCH')");
		if (!random_stack.empty())
			throw std::runtime_error("Unexpected end of script (missing 'RANDOMEND')");
		if (!short_stack.empty())
			throw std::runtime_error("Unexpected end of script (missing 'ENDIF')");

		// Write out checksums
		for (const auto &checksum : checksums)
		{
			add_token(Token::ChecksumName);
			add_int(checksum.first);
			add_string(checksum.second.c_str());
		}

		// Terminate bytecode
		add_token(Token::EndOfFile);
		return bytecode;
	}
}
