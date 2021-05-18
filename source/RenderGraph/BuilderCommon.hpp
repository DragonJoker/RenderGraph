/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePassDependencies.hpp"

#include <functional>

namespace crg
{
	namespace builder
	{
		using FramePassSet = std::set< FramePass * >;

		FramePassSet retrieveRoots( FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies );
		FramePassSet retrieveLeafs( FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies );

		template< typename TypeT >
		void filter( std::vector< TypeT > const & inputs
			, std::function< bool( TypeT const & ) > filterFunc
			, std::function< void( TypeT const & ) > trueFunc
			, std::function< void( TypeT const & ) > falseFunc = nullptr )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else if ( falseFunc )
				{
					falseFunc( input );
				}
			}
		}

		template< typename TypeT >
		void filter( std::vector< TypeT > & inputs
			, std::function< bool( TypeT & ) > filterFunc
			, std::function< void( TypeT & ) > trueFunc
			, std::function< void( TypeT & ) > falseFunc = nullptr )
		{
			for ( auto & input : inputs )
			{
				if ( filterFunc( input ) )
				{
					trueFunc( input );
				}
				else if ( falseFunc )
				{
					falseFunc( input );
				}
			}
		}
	}
}
