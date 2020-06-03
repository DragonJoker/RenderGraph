/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		ImageIdAliasMap optimiseImages( ImageIdDataOwnerCont const & images
			, RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root );
		ImageViewIdAliasMap optimiseImageViews( ImageViewIdDataOwnerCont const & imageViews
			, RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root );
	}
}
