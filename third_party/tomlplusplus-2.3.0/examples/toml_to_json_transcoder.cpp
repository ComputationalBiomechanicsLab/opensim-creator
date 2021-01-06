// This file is a part of toml++ and is subject to the the terms of the MIT license.
// Copyright (c) 2019-2020 Mark Gillard <mark.gillard@outlook.com.au>
// See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license text.
// SPDX-License-Identifier: MIT
/*

	This example demonstrates how to use the toml::json_formatter to
	re-serialize TOML data as JSON.

*/
#include <iostream>
#include <fstream>
#include "utf8_console.h"

#define TOML_UNRELEASED_FEATURES 1
#include <toml++/toml.h>

using namespace std::string_view_literals;

int main(int argc, char** argv)
{
	std::ios_base::sync_with_stdio(false);
	init_utf8_console();

	// read from a file if a path argument is given
	if (argc > 1)
	{
		try
		{
			const auto table = toml::parse_file(argv[1]);
			std::cout << toml::json_formatter{ table } << "\n";
		}
		catch (const toml::parse_error& err)
		{
			std::cerr << err << "\n";
			return 1;
		}
	}

	// otherwise read directly from stdin
	else 
	{
		try
		{
			const auto table = toml::parse(std::cin, "stdin"sv);
			std::cout << toml::json_formatter{ table } << "\n";
		}
		catch (const toml::parse_error& err)
		{
			std::cerr << err << "\n";
			return 1;
		}
	}

	return 0;
}
