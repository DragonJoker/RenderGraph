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
			, VkImageLayout outputLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doInitialise( uint32_t index );
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		LayoutState m_outputLayout;
	};
}
