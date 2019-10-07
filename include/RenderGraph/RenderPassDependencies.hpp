/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderGraphPrerequisites.hpp"

namespace crg
{
	struct RenderPassDependencies
	{
		RenderPass const * srcPass;
		RenderPass const * dstPass;
		AttachmentArray dependencies;
	};

	inline bool operator==( RenderPassDependencies const & lhs
		, RenderPassDependencies const & rhs )
	{
		return lhs.dstPass == rhs.dstPass
			&& lhs.srcPass == rhs.srcPass
			&& lhs.dependencies == rhs.dependencies;
	}

	inline bool operator==( RenderPassDependenciesArray const & lhs
		, RenderPassDependenciesArray const & rhs )
	{
		auto result = lhs.size() == rhs.size();

		for ( size_t i = 0u; result && i < lhs.size(); ++i )
		{
			result = lhs[i] == rhs[i];
		}

		return result;
	}
}
