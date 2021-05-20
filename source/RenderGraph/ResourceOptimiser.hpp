/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		void mergeViews( FramePassPtrArray const & passes );
		ImageIdAliasMap optimiseImages( ImageIdDataOwnerCont const & images
			, FramePassDependenciesMap const & dependencies
			, RootNode const & root );
		ImageViewIdAliasMap optimiseImageViews( ImageViewIdDataOwnerCont const & imageViews
			, FramePassDependenciesMap const & dependencies
			, RootNode const & root );
	}
}
