/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#include <ostream>

namespace crg::dot
{
	void displayPasses( std::ostream & stream
		, FrameGraph const & value );
	void displayTransitions( std::ostream & stream
		, FrameGraph const & value );
}
