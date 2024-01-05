/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#include <exception>
#include <source_location>

namespace crg
{
	class Exception
		: public std::exception
	{
	public:
		Exception( std::string const & text
			, std::string const & file
			, uint32_t line )
			: text{ file + ":" + std::to_string( line ) + " - " + text }
		{
		}

		Exception( std::string const & text
			, std::source_location const & loc )
			: Exception{ text, loc.file_name(), loc.line() }
		{
		}

		char const * what()const noexcept override
		{
			return text.c_str();
		}

	private:
		std::string text;
	};
}
