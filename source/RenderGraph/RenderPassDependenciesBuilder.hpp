/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		RenderPassDependenciesArray buildPassDependencies( RenderPassPtrArray const & passes );
	}
}
