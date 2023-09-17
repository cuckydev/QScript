%option never-interactive
%option nounistd
%option nounput noinput noyywrap
%option prefix="qscript_lex_"
%option yylineno

%{

#include <QLexer.h>

%}

/* Regex definitions */
newline [\n\r]
whitespace [ \t]

digit_bin [01]
digit_dec [0-9]
digit_hex [0-9a-fA-F]

letter [a-zA-Z]

ident_letter {letter}|_

number_bin [-+]?0b{digit_bin}+
number_dec [-+]?{digit_dec}+
number_hex [-+]?0x{digit_hex}+
number_real [+-]?({digit_dec}+([.]{digit_dec}*)?|[.]{digit_dec}+)

string \"([^\\\"]|\\.)*\"
local_string \#{string}
raw_checksum_string %{string}

checksum_string \({raw_checksum_string}\)
arg_checksum_string <{raw_checksum_string}>

raw_identifier {ident_letter}({ident_letter}|{digit_dec})*
identifier \({raw_identifier}\)
arg_identifier <{raw_identifier}>

raw_checksum 0x{digit_hex}+
checksum \({raw_checksum}\)
arg_checksum <{raw_checksum}>

label {raw_identifier}:

/* Tokens */
%%

 /* Ignore C-style comments */
"/*"([^*]|[*][^/])*[*]?"/"

 /* Ignore C++-style comments */
"//"(.)*

 /* Numbers */
{number_dec}  { QScript::g_tokens.emplace_back(new QScript::TokenNumber(QScript::Token::Integer, yytext, 10, nullptr)); }
{number_bin}  { QScript::g_tokens.emplace_back(new QScript::TokenNumber(QScript::Token::Integer, yytext, 2, "0b")); }
{number_hex}  { QScript::g_tokens.emplace_back(new QScript::TokenNumber(QScript::Token::Integer, yytext, 16, "0x")); }
{number_real} { QScript::g_tokens.emplace_back(new QScript::TokenReal(QScript::Token::Float, yytext)); }

 /* Strings */
{arg_checksum_string} { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::Arg, yytext, 3, -2)); }
{checksum_string} { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::Name, yytext, 3, -2)); }

{local_string} { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::LocalString, yytext, 2, -1)); }
{string}       { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::String, yytext, 1, -1)); }

 /* Identifiers */
{label} { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::Label, yytext, 0, -1)); }

{checksum}     { QScript::g_tokens.emplace_back(new QScript::TokenNumber(QScript::Token::NameChecksum, yytext + 1, 16, "0x")); }
{arg_checksum} { QScript::g_tokens.emplace_back(new QScript::TokenNumber(QScript::Token::ArgChecksum, yytext + 1, 16, "0x")); }

{arg_identifier} { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::Arg, yytext, 1, -1)); }
{identifier}     { QScript::g_tokens.emplace_back(new QScript::TokenString(QScript::Token::Name, yytext, 1, -1)); }

 /* Tokens */
"{" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::StartStruct)); }
"}" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::EndStruct)); }
"[" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::StartArray)); }
"]" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::EndArray)); }
"=" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Equals)); }
"." { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Dot)); }
"," { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Comma)); }
"-" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Minus)); }
"+" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Add)); }
"/" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Divide)); }
"*" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Multiply)); }
"(" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::OpenParenth)); }
")" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::CloseParenth)); }
":" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Colon)); }

 /* Comparisons */
"==" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::SameAs)); }
"<"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::LessThan)); }
"<=" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::LessThanEqual)); }
">"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::GreaterThan)); }
">=" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::GreaterThanEqual)); }

 /* Logical Operators */
"|" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Or)); }
"&" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::And)); }
"^" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Xor)); }

"<<" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::ShiftLeft)); }
">>" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::ShiftRight)); }

 /* Keywords */
"BEGIN"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordBegin)); }
"REPEAT" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRepeat)); }
"BREAK"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordBreak)); }

"SCRIPT"    { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordScript)); }
"ENDSCRIPT" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordEndScript)); }

"IF"     { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordIf)); }
"ELSE"   { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordElse)); }
"ELSEIF" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordElseIf)); }
"ENDIF"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordEndIf)); }

"RETURN" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordReturn)); }

"<...>" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordAllArgs)); }

"JUMP" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Jump)); }

"RANDOM_RANGE"     { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandomRange)); }

"RANDOMEND" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandomEnd)); }
"RANDOMCASE" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandomCase)); }

"RANDOM_NO_REPEAT" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandomNoRepeat)); }
"RANDOM_PERMUTE"   { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandomPermute)); }
"RANDOM2"          { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandom2)); }
"RANDOM"           { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordRandom)); }

"NOT" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordNot)); }
"AND" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordAnd)); }
"OR"  { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordOr)); }

"SWITCH"    { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordSwitch)); }
"ENDSWITCH" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordEndSwitch)); }
"CASE"      { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordCase)); }
"DEFAULT"   { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::KeywordDefault)); }

 /* Types */
"PAIR"   { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Pair)); }
"VECTOR" { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::Vector)); }

 /* Newline */
{newline} { QScript::g_tokens.emplace_back(new QScript::TokenBase(QScript::Token::EndOfLine)); }

 /* Skip whitespace */
{whitespace}

 /* Unknown */
. { printf("Unrecognized character [%c] at line %lu\n", yytext[0], yylineno); return 0; }
%%
