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
		, uint32_t maxPassCount
		, rq::Config config )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } )
				, [this]( VkCommandBuffer cb, uint32_t i ){ doRecordInto( cb, i ); }
				, [this]( VkCommandBuffer cb, uint32_t i ){ doRecordDisabledInto( cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderQuad.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderQuad.isEnabled(); } ) }
			, maxPassCount
			, ( config.m_enabled ? true : false ) }
		, m_recordDisabledRenderPass{ config.m_recordDisabledRenderPass }
		, m_renderQuad{ pass
			, context
			, graph
			, config
			, maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, maxPassCount
			, std::move( config.m_renderSize ? *config.m_renderSize : defaultV< VkExtent2D > ) }
	{
	}

	RenderQuad::~RenderQuad()
	{
	}

	void RenderQuad::resetPipeline( VkPipelineShaderStageCreateInfoArray config )
	{
		resetCommandBuffer();
		m_renderQuad.resetPipeline( std::move( config ) );
		recordCurrent();
	}

	void RenderQuad::doInitialise()
	{
		m_renderPass.initialise( *this );
		m_renderQuad.initialise( *this
			, m_renderPass.getRenderSize()
			, m_renderPass.getRenderPass()
			, m_renderPass.createBlendState() );
	}

	void RenderQuad::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_renderPass.begin( commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
		m_renderQuad.record( commandBuffer, index );
		m_renderPass.end( commandBuffer );
		m_renderQuad.end( commandBuffer, index );
	}

	void RenderQuad::doRecordDisabledInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_recordDisabledRenderPass )
		{
			m_renderPass.begin( commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
			m_renderPass.end( commandBuffer );
		}

		m_renderQuad.recordDisabled( *this, commandBuffer, index );
	}

	VkPipelineStageFlags RenderQuad::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
}
