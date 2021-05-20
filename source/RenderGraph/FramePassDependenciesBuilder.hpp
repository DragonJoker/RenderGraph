/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		FramePassDependenciesArray buildPassAttachDependencies( FramePassPtrArray const & passes );
		void filterPassDependencies( FramePassDependenciesArray & dependencies );
	}
}
