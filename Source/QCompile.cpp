#include <QScript/QCompile.h>

#define YY_NO_UNISTD_H
#include <Lexical/Lexical.h>

#include "QLexer.h"
#include "QUtil.h"

#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <stack>

namespace QScript
{
	std::vector<unsigned char> Compile(const std::string &source)
	{
		// Perform lexical analysis
		qscript_lex__scan_string(source.c_str());
		while (qscript_lex_lex()) {}
		qscript_lex_lex_destroy();

		// Process tokens
		std::vector<unsigned char> bytecode;

		auto add_token = [&bytecode](QScript::Token token)
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

		auto get_token_integer = [&bytecode](const TokenBase &token) -> signed long
			{
				switch (token.type)
				{
					case QScript::Token::Integer:
					case QScript::Token::HexInteger:
					{
						const auto &integer = (const QScript::TokenNumber &)token;
						return integer.value;
						break;
					}
					case QScript::Token::Float:
					{
						const auto &real = (const QScript::TokenReal &)token;
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
					case QScript::Token::Integer:
					case QScript::Token::HexInteger:
					{
						const auto &integer = (const QScript::TokenNumber &)token;
						return (float)integer.value;
						break;
					}
					case QScript::Token::Float:
					{
						const auto &real = (const QScript::TokenReal &)token;
						return real.value;
						break;
					}
					default:
						throw std::runtime_error("Expected integer or float");
				}
			};

		std::unordered_map<std::string, unsigned long> checksums;

		std::unordered_map<std::string, unsigned long> labels;
		std::vector<std::pair<unsigned long, std::string>> label_refs;

		struct RandomStack
		{
			size_t address = 0;
			size_t jump = 0, num_jumps = 0;
			std::vector<size_t> end_jumps;
		};
		std::stack<RandomStack> random_stack;

		auto &token_it = g_tokens.cbegin();
		auto &token_end = g_tokens.cend();

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

		while (token_can_pop())
		{
			const auto &token = token_pop();
			if (token == nullptr)
				break;

			switch (token->type)
			{
				case QScript::Token::Name:
				case QScript::Token::Arg:
				{
					const auto &str = (const QScript::TokenString &)*token;
					unsigned long crc = QScript::CRC(str.value.c_str());
					checksums[str.value] = crc;

					if (token->type == QScript::Token::Arg)
						add_token(QScript::Token::Arg);
					add_token(QScript::Token::Name);
					add_int(crc);
					break;
				}
				case QScript::Token::String:
				case QScript::Token::LocalString:
				{
					const auto &str = (const QScript::TokenString &)*token;
					add_token(token->type);
					add_string_sized(str.value);
					break;
				}
				case QScript::Token::Integer:
				case QScript::Token::HexInteger:
				{
					const auto &integer = (const QScript::TokenNumber &)*token;
					add_token(token->type);
					add_int(integer.value);
					break;
				}
				case QScript::Token::Float:
				{
					const auto &real = (const QScript::TokenReal &)*token;
					add_token(token->type);
					add_real(real.value);
					break;
				}
				case QScript::Token::Pair:
				{
					const auto &token_lp = token_pop();
					const auto &token_x = token_pop();
					const auto &token_comma = token_pop();
					const auto &token_y = token_pop();
					const auto &token_rp = token_pop();

					if (token_lp->type != QScript::Token::OpenParenth)
						throw std::runtime_error("Expected '('");
					if (token_comma->type != QScript::Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_rp->type != QScript::Token::CloseParenth)
						throw std::runtime_error("Expected ')'");

					add_token(QScript::Token::Pair);
					add_real(get_token_real(*token_x));
					add_real(get_token_real(*token_y));
					break;
				}
				case QScript::Token::Vector:
				{
					const auto &token_lp = token_pop();
					const auto &token_x = token_pop();
					const auto &token_comma_x = token_pop();
					const auto &token_y = token_pop();
					const auto &token_comma_y = token_pop();
					const auto &token_z = token_pop();
					const auto &token_rp = token_pop();

					if (token_lp->type != QScript::Token::OpenParenth)
						throw std::runtime_error("Expected '('");
					if (token_comma_x->type != QScript::Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_comma_y->type != QScript::Token::Comma)
						throw std::runtime_error("Expected ','");
					if (token_rp->type != QScript::Token::CloseParenth)
						throw std::runtime_error("Expected ')'");

					add_token(QScript::Token::Vector);
					add_real(get_token_real(*token_x));
					add_real(get_token_real(*token_y));
					add_real(get_token_real(*token_z));
					break;
				}
				case QScript::Token::EndOfLine:
				{
					// Don't add multiple end of lines in a row
					add_token(QScript::Token::EndOfLine);
					while (token_can_pop())
					{
						const auto &next_token = token_peek();
						if (next_token == nullptr || next_token->type != QScript::Token::EndOfLine)
							break;
						token_pop();
					}
					break;
				}
				case QScript::Token::KeywordRandom:
				case QScript::Token::KeywordRandom2:
				case QScript::Token::KeywordRandomNoRepeat:
				case QScript::Token::KeywordRandomPermute:
				{
					// Parse weight list
					const auto &token_lp = token_pop();
					if (token_lp->type != QScript::Token::OpenParenth)
						throw std::runtime_error("Expected '('");

					std::vector<signed long> weights;
					while (1)
					{
						// Grab number
						const auto &token_number = token_pop();
						if (token_number->type == QScript::Token::CloseParenth)
							break;
						weights.push_back(get_token_integer(*token_number));

						// Grab comma or close parenth
						const auto &token_next = token_pop();
						if (token_next->type == QScript::Token::CloseParenth)
							break;
						if (token_next->type != QScript::Token::Comma)
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
				case QScript::Token::KeywordRandomCase:
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
						add_token(QScript::Token::Jump);
						add_int(0);
					}

					// Push address to case
					size_t addr = random.address + 1 + 4 + 2 * random.num_jumps + 4 * random.jump;
					set_address(addr, bytecode.size());
					random.jump++;
					break;
				}
				case QScript::Token::KeywordRandomEnd:
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

		for (const auto &checksum : checksums)
		{
			add_token(QScript::Token::ChecksumName);
			add_int(checksum.second);
			add_string(checksum.first.c_str());
		}

		add_token(QScript::Token::EndOfFile);

		g_tokens.clear();
		return bytecode;
	}
}
