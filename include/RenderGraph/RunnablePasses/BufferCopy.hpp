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
			, DeviceSize copyOffset
			, DeviceSize copyRange
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;

	private:
		DeviceSize m_copyOffset;
		DeviceSize m_copyRange;
	};
}
