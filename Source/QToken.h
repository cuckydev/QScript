#pragma once

namespace QScript
{
	// Token enum
	enum class Token
	{
		// INTERNAL TOKENS
		KeywordRandomEnd = -1,
		KeywordRandomCase = -2,

		Label = -3,

		NameChecksum = -4, // Name, except we don't know the name
		ArgChecksum = -5, // Arg, except we don't know the name

		// Qb tokens
		EndOfFile = 0,
		EndOfLine = 1,
		EndOfLineNumber = 2,
		StartStruct = 3,
		EndStruct = 4,
		StartArray = 5,
		EndArray = 6,
		Equals = 7,
		Dot = 8,
		Comma = 9,
		Minus = 10,
		Add = 11,
		Divide = 12,
		Multiply = 13,
		OpenParenth = 14,
		CloseParenth = 15,

		// This is ignored by the interpreter.
		// Allows inclusion of source level debugging info, eg line number.
		DebugInfo = 16,

		// Comparisons
		SameAs = 17,
		LessThan = 18,
		LessThanEqual = 19,
		GreaterThan = 20,
		GreaterThanEqual = 21,

		// Types
		Name = 22,
		Integer = 23,
		HexInteger = 24, // The parser doesn't seem to fully support this, and it doesn't appear in any qb files
		Enum = 25,
		Float = 26,
		String = 27,
		LocalString = 28,
		Array = 29,
		Vector = 30,
		Pair = 31,

		// Key words
		KeywordBegin = 32,
		KeywordRepeat = 33,
		KeywordBreak = 34,
		KeywordScript = 35,
		KeywordEndScript = 36,
		KeywordIf = 37,
		KeywordElse = 38,
		KeywordElseIf = 39, // Doesn't appear in any qb files
		KeywordEndIf = 40,
		KeywordReturn = 41,

		Undefined = 42,

		// For debugging
		ChecksumName = 43,

		// Token for the <...> symbol
		KeywordAllArgs = 44,
		// Token that preceds a name when the name is enclosed in < > in the source.
		Arg = 45,

		// A relative jump. Used to speed up if-else-endif and break statements, and
		// used to jump to the end of lists of items in the random operator.
		Jump = 46,
		// Precedes a list of items that are to be randomly chosen from.
		KeywordRandom = 47,

		// Precedes two integers enclosed in parentheses.
		KeywordRandomRange = 48,

		// Only used internally by qcomp, never appears in a .qb
		At = 49,

		// Logical operators
		Or = 50,
		And = 51,
		Xor = 52,

		// Shift operators
		ShiftLeft = 53,
		ShiftRight = 54,

		// These versions use the Rnd2 function, for use in certain things so as not to mess up
		// the determinism of the regular Rnd function in replays.
		KeywordRandom2 = 55,
		KeywordRandomRange2 = 56,

		KeywordNot = 57,
		KeywordAnd = 58,
		KeywordOr = 59,
		KeywordSwitch = 60,
		KeywordEndSwitch = 61,
		KeywordCase = 62,
		KeywordDefault = 63,

		KeywordRandomNoRepeat = 64,
		KeywordRandomPermute = 65,

		Colon = 66,

		FastIf = 71,
		FastElse = 72,
		ShortJump = 73,
	};
}
