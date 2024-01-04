/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class ImageBlit
		: public RunnablePass
	{
	public:
		CRG_API ImageBlit( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, VkOffset3D blitSrcOffset
			, VkExtent3D blitSrcSize
			, VkOffset3D blitDstOffset
			, VkExtent3D blitDstSize
			, VkFilter filter
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		VkOffset3D m_srcOffset;
		VkExtent3D m_srcSize;
		VkOffset3D m_dstOffset;
		VkExtent3D m_dstSize;
		VkFilter m_filter;
	};
}
