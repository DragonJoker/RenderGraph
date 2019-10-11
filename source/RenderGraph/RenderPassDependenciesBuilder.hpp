/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderPassDependencies.hpp"

#include <functional>

namespace crg
{
	namespace details
	{
		RenderPassDependenciesArray buildPassDependencies( std::vector< RenderPassPtr > const & passes );

		template< typename TypeT >
		void filter( std::vector< TypeT > const & inputs
			, std::function< bool( TypeT const & ) > filterFunc
			, std::function< void( TypeT const & ) > trueFunc
			, std::function< void( TypeT const & ) > falseFunc = []( TypeT const & ){} )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else
				{
					falseFunc( input );
				}
			}
		}
		
		template< typename TypeT >
		void filter( std::vector< TypeT > & inputs
			, std::function< bool( TypeT & ) > filterFunc
			, std::function< void( TypeT & ) > trueFunc
			, std::function< void( TypeT & ) > falseFunc = []( TypeT & ){} )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else
				{
					falseFunc( input );
				}
			}
		}
	}
}
