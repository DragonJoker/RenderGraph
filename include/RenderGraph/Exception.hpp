/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#include <exception>

namespace crg
{
	class Exception
		: public std::exception
	{
	public:
		Exception( std::string text
			, std::string file
			, int line )
			: text{ file + ":" + std::to_string( line ) + " - " + text }
		{
		}

		char const * what()const noexcept override
		{
			return text.c_str();
		}

	private:
		std::string text;
	};

#define CRG_Exception( text )\
	throw crg::Exception{ text, __FILE__, __LINE__ }
}
