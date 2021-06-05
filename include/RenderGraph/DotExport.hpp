/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#include <ostream>

namespace crg::dot
{
	CRG_API void displayPasses( std::ostream & stream
		, RunnableGraph const & value );
	CRG_API void displayTransitions( std::ostream & stream
		, RunnableGraph const & value );
}
