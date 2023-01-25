/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

namespace crg
{
	class RenderPassHolder
	{
	public:
		struct Entry
		{
			crg::ImageViewId view;
			LayoutState input;
			LayoutState output;
		};

	public:
		CRG_API RenderPassHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, uint32_t maxPassCount
			, VkExtent2D const & size = {} );
		CRG_API ~RenderPassHolder();

		CRG_API bool initialise( RecordContext & context
			, crg::RunnablePass const & runnable
			, uint32_t passIndex );
		CRG_API VkRenderPassBeginInfo getBeginInfo( uint32_t index );
		CRG_API void begin( RecordContext & context
			, VkCommandBuffer commandBuffer
			, VkSubpassContents subpassContents
			, uint32_t index );
		CRG_API void end( RecordContext & context
			, VkCommandBuffer commandBuffer );
		CRG_API VkPipelineColorBlendStateCreateInfo createBlendState();
		CRG_API VkFramebuffer getFramebuffer( uint32_t index )const;

		VkRenderPass getRenderPass( uint32_t index )const
		{
			return m_renderPasses[index];
		}

		VkExtent2D const & getRenderSize()const
		{
			return m_size;
		}

		VkRect2D const & getRenderArea()const
		{
			return m_renderArea;
		}

		std::vector< VkClearValue > const & getClearValues()const
		{
			return m_clearValues;
		}

		VkPipelineColorBlendAttachmentStateArray const & getBlendAttachs()const
		{
			return m_blendAttachs;
		}

	private:
		void doCreateRenderPass( RecordContext & context
			, crg::RunnablePass const & runnable
			, PipelineState const & previousState
			, PipelineState const & nextState
			, uint32_t passIndex );
		void doInitialiseRenderArea();
		VkFramebuffer doCreateFramebuffer( uint32_t passIndex )const;
		void doCleanup();

	private:
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		VkExtent2D m_size;
		std::vector< VkRenderPass > m_renderPasses{};
		mutable std::vector< VkFramebuffer > m_frameBuffers;
		VkRect2D m_renderArea{};
		std::vector< Attachment const * > m_attachments;
		uint32_t m_layers{};
		std::vector< VkClearValue > m_clearValues;
		std::vector< Entry > m_attaches;
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
	};
}
