/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#pragma warning( push )
#pragma warning( disable: 4365 )
#include <ostream>
#pragma warning( pop )

namespace crg::dot
{
	struct Config
	{
		bool withColours;
		bool withIds;
	};

	CRG_API void displayPasses( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config );
	CRG_API void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config );
}
