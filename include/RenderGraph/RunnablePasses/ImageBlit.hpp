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
			, Offset3D const & blitSrcOffset
			, Extent3D const & blitSrcSize
			, Offset3D const & blitDstOffset
			, Extent3D const & blitDstSize
			, FilterMode filter
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext const & context
			, VkCommandBuffer commandBuffer
			, uint32_t index );

	private:
		VkOffset3D m_srcOffset;
		VkExtent3D m_srcSize;
		VkOffset3D m_dstOffset;
		VkExtent3D m_dstSize;
		FilterMode m_filter;
	};
}
