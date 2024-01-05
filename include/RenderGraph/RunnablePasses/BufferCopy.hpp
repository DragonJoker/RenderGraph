/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class BufferCopy
		: public RunnablePass
	{
	public:
		CRG_API BufferCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkDeviceSize copyOffset
			, VkDeviceSize copyRange
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;

	private:
		VkDeviceSize m_copyOffset;
		VkDeviceSize m_copyRange;
	};
}
