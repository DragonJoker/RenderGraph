/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderQuad.hpp"

namespace crg
{
	RenderQuad::RenderQuad( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config ruConfig
		, rq::Config rqConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderQuad.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderQuad.isEnabled(); } ) }
			, { ruConfig.maxPassCount
				, true /*resettable*/
				, ruConfig.actions } }
		, m_renderQuad{ pass
			, context
			, graph
			, rqConfig
			, ruConfig.maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, std::move( rqConfig.m_renderSize ? *rqConfig.m_renderSize : defaultV< VkExtent2D > ) }
	{
	}

	RenderQuad::~RenderQuad()
	{
	}

	void RenderQuad::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		resetCommandBuffer();
		m_renderQuad.resetPipeline( std::move( config ) );
		reRecordCurrent();
	}

	void RenderQuad::doInitialise()
	{
	}

	void RenderQuad::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_renderPass.initialise( context, *this ) )
		{
			m_renderQuad.initialise( *this
				, m_renderPass.getRenderSize()
				, m_renderPass.getRenderPass()
				, m_renderPass.createBlendState() );
		}

		m_renderPass.begin( context, commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
		m_renderQuad.record( context, commandBuffer, index );
		m_renderPass.end( context, commandBuffer );
		m_renderQuad.end( context, commandBuffer, index );
	}

	VkPipelineStageFlags RenderQuad::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
}
