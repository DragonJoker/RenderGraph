/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class ImageCopy
		: public RunnablePass
	{
	public:
		CRG_API ImageCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Extent3D const & copySize
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );
		CRG_API ImageCopy( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Extent3D const & copySize
			, ImageLayout finalOutputLayout
			, ru::Config ruConfig = {}
			, GetPassIndexCallback passIndex = GetPassIndexCallback( [](){ return 0u; } )
			, IsEnabledCallback isEnabled = IsEnabledCallback( [](){ return true; } ) );

	private:
		void doRecordInto( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;
		void doRecordMultiToMulti( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;
		void doRecordMultiToSingle( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;
		void doRecordSingleToMulti( RecordContext & context
			, VkCommandBuffer commandBuffer
			, uint32_t index )const;

	private:
		VkExtent3D m_copySize;
		ImageLayout m_finalOutputLayout;
	};
}
