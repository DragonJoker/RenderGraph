/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

#include <functional>
#include <optional>

namespace crg
{
	using RunnablePassCreator = std::function< RunnablePassPtr( RenderPass const &
		, GraphContext const &
		, RunnableGraph const & ) >;

	struct RenderPass
	{
		RenderPass( std::string const & name
			, AttachmentArray const & sampled
			, AttachmentArray const & colourInOuts
			, RunnablePassCreator runnableCreator );
		RenderPass( std::string const & name
			, AttachmentArray const & sampled
			, AttachmentArray const & colourInOuts
			, std::optional< Attachment > const & depthStencilInOut
			, RunnablePassCreator runnableCreator );

		RunnablePassPtr createRunnable( GraphContext const & context
			, RunnableGraph const & graph )const;

		std::string name;
		AttachmentArray sampled;
		AttachmentArray colourInOuts;
		std::optional< Attachment > depthStencilInOut;
		RunnablePassCreator runnableCreator;
	};
}
