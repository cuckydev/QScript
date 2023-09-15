cmake_minimum_required(VERSION 3.26)

# Project
project(QScript LANGUAGES C CXX)

# Find flex
find_program(FLEX_EXECUTABLE flex)

if(FLEX_EXECUTABLE)
	# Notify user that flex was found
	message(STATUS "Found flex: ${FLEX_EXECUTABLE}")

	# Generate lexer
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Include/Lexical)
	add_custom_command(
		OUTPUT Include/Lexical/Lexical.cpp Include/Lexical/Lexical.h
		COMMAND ${FLEX_EXECUTABLE} --outfile=Include/Lexical/Lexical.cpp --header-file=Include/Lexical/Lexical.h ${CMAKE_CURRENT_SOURCE_DIR}/Source/Lexical.l
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Lexical.l
		COMMENT "Generating QScript lexer"
	)

	add_custom_target(
		QScript.Lexer
		DEPENDS Include/Lexical/Lexical.cpp Include/Lexical/Lexical.h
	)

	# Compile QCompile library
	add_library(QScript.QCompile STATIC
		"Source/QCompile.cpp"
		"Include/QScript/QCompile.h"

		"Source/QLexer.cpp"
		"Source/QLexer.h"
		"Source/QToken.h"
		"Source/QUtil.h"

		"${CMAKE_BINARY_DIR}/Include/Lexical/Lexical.cpp"
		"${CMAKE_BINARY_DIR}/Include/Lexical/Lexical.h"
	)
	target_include_directories(QScript.QCompile PRIVATE "Source")
	target_include_directories(QScript.QCompile PUBLIC "Include")

	add_dependencies(QScript.QCompile QScript.Lexer)
	target_include_directories(QScript.QCompile PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/Include)
	
	# Compile QCompile app
	add_executable(QScript.QCompile.App
		"App/QCompile.cpp"
	)

	target_link_libraries(QScript.QCompile.App PRIVATE QScript.QCompile)

	# Install QCompile
	install(TARGETS QScript.QCompile DESTINATION lib)
	install(TARGETS QScript.QCompile.App DESTINATION bin)
endif()

# Compile QDecompile library
add_library(QScript.QDecompile STATIC
	"Source/QDecompile.cpp"
	"Include/QScript/QDecompile.h"

	"Source/QBinary.cpp"
	"Source/QBinary.h"
	"Source/QUtil.h"
)
target_include_directories(QScript.QDecompile PRIVATE "Source")
target_include_directories(QScript.QDecompile PUBLIC "Include")

# Compile QDecompile app
add_executable(QScript.QDecompile.App
	"App/QDecompile.cpp"
)

target_link_libraries(QScript.QDecompile.App PRIVATE QScript.QDecompile)

# Install QDecompile
install(TARGETS QScript.QDecompile DESTINATION lib)
install(TARGETS QScript.QDecompile.App DESTINATION bin)
