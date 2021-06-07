/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#include <ostream>

namespace crg::dot
{
	static std::string_view constexpr imgColour{ "#8b008b" };
	static std::string_view constexpr bufColour{ "#458b00" };
	static std::string_view constexpr passColour{ "#00007f" };

	CRG_API void displayPasses( std::ostream & stream
		, RunnableGraph const & value
		, bool withColours );
	CRG_API void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, bool withColours );
}
