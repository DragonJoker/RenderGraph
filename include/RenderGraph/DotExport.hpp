/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#pragma warning( push )
#pragma warning( disable: 4365 )
#include <map>
#include <ostream>
#include <string>
#include <sstream>
#pragma warning( pop )

namespace crg::dot
{
	struct Config
	{
		bool withColours{};
		bool withIds{};
		bool withGroups{};
		bool splitGroups{};
	};
	using DisplayResult = std::map< std::string, std::stringstream >;

	CRG_API DisplayResult displayPasses( RunnableGraph const & value
		, Config const & config );
	CRG_API DisplayResult displayTransitions( RunnableGraph const & value
		, Config const & config );
	CRG_API void displayPasses( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config );
	CRG_API void displayTransitions( std::ostream & stream
		, RunnableGraph const & value
		, Config const & config );
}
