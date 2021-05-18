/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

#include "RenderGraph/FramePassDependencies.hpp"
#include "RenderGraph/FramePass.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		FramePassSet retrieveRoots( FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies )
		{
			FramePassSet result;
			filter< FramePassPtr >( passes
				, [&dependencies]( FramePassPtr const & pass )
				{
					// We want the passes that are not listed as destination to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( FramePassDependencies const & lookup )
						{
							return lookup.srcPass
								&& lookup.dstPass == pass.get();
						} );
				}
				, [&result]( FramePassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
		}

		FramePassSet retrieveLeafs( FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies )
		{
			FramePassSet result;
			filter< FramePassPtr >( passes
				, [&dependencies]( FramePassPtr const & pass )
				{
					// We want the passes that are not listed as source to other passes.
					return dependencies.end() == std::find_if( dependencies.begin()
						, dependencies.end()
						, [&pass]( FramePassDependencies const & lookup )
						{
							return lookup.dstPass
								&& lookup.srcPass == pass.get();
						} );
				}
				, [&result]( FramePassPtr const & lookup )
				{
					result.insert( lookup.get() );
				} );
			return result;
		}
	}
}
