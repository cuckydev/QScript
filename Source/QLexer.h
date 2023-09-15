#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <cstring>

#include "QToken.h"

namespace QScript
{
	// Token structure
	struct TokenBase
	{
		Token type;

		TokenBase(Token _type) : type(_type) {}

		protected:
		static int ctoi(char c)
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		}
	};

	struct TokenString : public TokenBase
	{
		std::string value;

		TokenString(Token _type, const char *_str, int substart, int subend) : TokenBase(_type)
		{
			// Get string start and end
			const char *qstart = _str;
			const char *qend = _str + strlen(_str) - 1;

			if (subend > 0)
				qend = qstart + subend;
			else if (subend < 0)
				qend += subend;
			if (substart > 0)
				qstart += substart;
			if (qstart > qend) return;
			if (qend < qstart) return;

			// Parse string
			// Handle escape sequences
			const char *qp = qstart;
			auto qpeek = [&qp, qend]() -> char
				{
					if (qp > qend) return '\0';
					return *qp;
				};
			auto qpop = [&qp, qend]() -> char
				{
					if (qp > qend) return '\0';
					return *qp++;
				};

			while (1)
			{
				// Pop character
				char c = qpop();
				if (c == '\0') break;

				// Handle escape sequences
				if (c == '\\')
				{
					char e0 = qpop();
					switch (e0)
					{
						case 'a':
							value += '\a';
							break;
						case 'b':
							value += '\b';
							break;
						case 'f':
							value += '\f';
							break;
						case 'n':
							value += '\n';
							break;
						case 'r':
							value += '\r';
							break;
						case 't':
							value += '\t';
							break;
						case 'v':
							value += '\v';
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						{
							// Grab first digit
							e0 = e0 - '0';

							// Grab second digit
							int e1;
							if (e1 = ctoi(qpeek()), (e1 >= 0 && e1 < 8))
							{
								// 2+ digit octal
								qpop();

								// Grab third digit
								int e2;
								if (e2 = ctoi(qpeek()), (e2 >= 0 && e2 < 8))
								{
									// 3 digit octal
									qpop();

									// Write character
									value += (e0 << 6) | (e1 << 3) | e2;
								}
								else
								{
									// 2 digit octal
									value += (e0 << 3) | e1;
								}
							}
							else
							{
								// 1 digit octal
								value += e0;
							}
							break;
						}
						case 'x':
						{
							// Read digits
							char value = 0;
							while (1)
							{
								// Grab digit
								int e = ctoi(qpeek());
								if (e < 0 || e > 15) break;

								// Push digit to value
								value = (value << 4) | e;
							}

							// Write character
							value += value;
							break;
						}
						default:
						{
							// Write character
							value += e0;
							break;
						}
					}
				}
				else
				{
					// Write character
					value += c;
				}
			}
		}
	};

	struct TokenNumber : public TokenBase
	{
		signed long value = 0;

		TokenNumber(Token _type, const char *_str, int base, const char *pre) : TokenBase(_type)
		{
			// Get string start and end
			const char *qstart = _str;
			const char *qend = _str + strlen(_str) - 1;

			const char *qp = qstart;
			auto qpop = [&qp, qend]() -> char
				{
					if (qp > qend) return '\0';
					return *qp++;
				};

			// Pop first character
			bool minus = false;

			char c = qpop();
			if (c == '\0') return;

			if (c == '-')
			{
				minus = true;
				c = qpop();
			}
			else if (c == '+')
			{
				minus = false;
				c = qpop();
			}

			// Check for prefix
			if (pre != nullptr)
			{
				// Check prefix
				while (1)
				{
					// Get prefix character
					char p = *pre++;
					if (p == '\0') break;

					// Check prefix character
					if (c != p) return;

					// Pop next character
					c = qpop();
					if (c == '\0') return;
				}
			}

			// Parse string
			while (1)
			{
				// Get digit
				int digit = ctoi(c);
				if (digit < 0 || digit >= base) break; // TODO: throw exception

				// Push digit to value
				value = (value * base) + digit;

				// Pop next character
				c = qpop();
				if (c == '\0') break;
			}

			// Handle sign
			if (minus)
				value = -value;
		}
	};

	struct TokenReal : public TokenBase
	{
		double value = 0.0;

		TokenReal(Token _type, const char *_str) : TokenBase(_type)
		{
			// Get string start and end
			const char *qstart = _str;
			const char *qend = _str + strlen(_str) - 1;

			const char *qp = qstart;
			auto qpop = [&qp, qend]() -> char
				{
					if (qp > qend) return '\0';
					return *qp++;
				};

			// Pop first character
			bool minus = false;

			char c = qpop();
			if (c == '\0') return; // TODO: throw exception

			if (c == '-')
			{
				minus = true;
				c = qpop();
			}
			else if (c == '+')
			{
				minus = false;
				c = qpop();
			}

			// Parse string
			double decimal = 0.0;
			while (1)
			{
				// Check for decimal
				if (decimal != 0.0)
				{
					// Get digit
					int digit = ctoi(c);
					if (digit < 0 || digit >= 10) break; // TODO: throw exception

					// Push digit to value
					value += digit / decimal;
					decimal *= 10.0;
				}
				else
				{
					if (c == '.')
					{
						// Start decimal parse
						decimal = 10.0;
					}
					else
					{
						// Get digit
						int digit = ctoi(c);
						if (digit < 0 || digit >= 10) break; // TODO: throw exception

						// Push digit to value
						value = (value * 10.0) + digit;
					}
				}

				// Pop next character
				c = qpop();
				if (c == '\0') break;
			}

			// Handle sign
			if (minus)
				value = -value;
		}
	};

	// QScript globals
	extern std::list<std::unique_ptr<TokenBase>> g_tokens;
}
