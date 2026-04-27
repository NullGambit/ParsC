#pragma once

#include "fmt.hpp"

std::string fmt::do_format(std::string_view fmt, const std::string& buffer)
{
	std::string output;
	u32 offset {};

	for (auto i = 0; i < fmt.size(); i++)
	{
		if (fmt[i] == '{')
		{
			if (fmt[++i] == '{')
				goto concat;

			for (char c = buffer[offset++]; c; c = buffer[offset++])
			{
				output += c;
			}

			while (i < fmt.size() && fmt[i] != '}')
			{
				i++;
			}

			continue;
		}

		concat:
			output += fmt[i];
	}

	return output;
}
