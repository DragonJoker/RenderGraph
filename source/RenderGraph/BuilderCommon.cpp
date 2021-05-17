/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

#include "RenderGraph/RenderPassDependencies.hpp"
#include "RenderGraph/RenderPass.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		RenderPassSet retrieveRoots( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies )
		{
			RenderPassSet result;
			filter< RenderPassPtr >( passes
				, [&dependencies]( RenderPassPtr const & pass )
				{
					// We want the passes that are not listed as destination to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( RenderPassDependencies const & lookup )
						{
							return lookup.srcPass
								&& lookup.dstPass == pass.get();
						} );
				}
				, [&result]( RenderPassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
		}

		RenderPassSet retrieveLeafs( RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies )
		{
			RenderPassSet result;
			filter< RenderPassPtr >( passes
				, [&dependencies]( RenderPassPtr const & pass )
				{
					// We want the passes that are not listed as source to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( RenderPassDependencies const & lookup )
						{
							return lookup.dstPass
								&& lookup.srcPass == pass.get();
						} );
				}
				, [&result]( RenderPassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
		}
	}
}
