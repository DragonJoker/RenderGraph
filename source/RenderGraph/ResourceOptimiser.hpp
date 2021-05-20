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
			, FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root );
		ImageViewIdAliasMap optimiseImageViews( ImageViewIdDataOwnerCont const & imageViews
			, FramePassPtrArray const & passes
			, FramePassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root );
	}
}
