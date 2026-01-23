/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderMesh.hpp"

namespace crg
{
	RenderMesh::RenderMesh( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ru::Config const & ruConfig
		, rm::Config rmConfig )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eColorAttachmentOutput ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, GetPassIndexCallback( [this](){ return m_renderMesh.getPassIndex(); } )
				, IsEnabledCallback( [this](){ return m_renderMesh.isEnabled(); } ) }
			, { ruConfig.maxPassCount
				, true /*resettable*/
				, ruConfig.prePassActions
				, ruConfig.postPassActions
				, ruConfig.implicitImageActions
				, ruConfig.implicitBufferActions } }
		, m_renderMesh{ pass
			, context
			, graph
			, std::move( rmConfig )
			, ruConfig.maxPassCount }
		, m_renderPass{ pass
			, context
			, graph
			, ruConfig.maxPassCount
			, m_renderMesh.getRenderSize() }
	{
	}

	void RenderMesh::resetPipelineLayout( std::vector< VkDescriptorSetLayout > const & layouts
		, std::vector< VkPushConstantRange > const & ranges
		, VkPipelineShaderStageCreateInfoArray const & config
		, uint32_t index )
	{
		bool hadCommandBuffer = resetCommandBuffer( index );
		m_renderMesh.resetPipelineLayout( layouts, ranges, config, index );
		if ( hadCommandBuffer )
			reRecordCurrent();
	}

	void RenderMesh::resetPipeline( VkPipelineShaderStageCreateInfoArray config
		, uint32_t index )
	{
		resetCommandBuffer( index );
		m_renderMesh.resetPipeline( std::move( config ), index );
		reRecordCurrent();
	}

	void RenderMesh::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_renderPass.initialise( context, *this, index ) )
		{
			m_renderMesh.cleanup();
			m_renderMesh.initialise( m_renderPass.getRenderSize()
				, m_renderPass.getRenderPass( index )
				, m_renderPass.createBlendState()
				, index );
		}

		m_renderPass.begin( context, commandBuffer, VK_SUBPASS_CONTENTS_INLINE, index );
		m_renderMesh.record( context, commandBuffer, index );
		m_renderPass.end( context, commandBuffer );
		m_renderMesh.end( context, commandBuffer, index );
	}
}
