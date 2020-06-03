/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderPassDependencies.hpp"

#include <functional>

namespace crg
{
	namespace builder
	{
		using RenderPassSet = std::set< RenderPass * >;

		RenderPassSet retrieveRoots( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies );
		RenderPassSet retrieveLeafs( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies );

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
