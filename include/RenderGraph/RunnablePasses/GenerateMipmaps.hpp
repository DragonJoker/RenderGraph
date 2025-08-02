/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class GenerateMipmaps
		: public RunnablePass
	{
	public:
		CRG_API GenerateMipmaps( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, ImageLayout outputLayout = ImageLayout::eUndefined
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );
		void doProcessImageView( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageViewId viewId );

	private:
		LayoutState m_outputLayout;
	};
}
