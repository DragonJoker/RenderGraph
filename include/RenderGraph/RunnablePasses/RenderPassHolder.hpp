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
		RenderPassHolder( RenderPassHolder const & )noexcept = delete;
		RenderPassHolder & operator=( RenderPassHolder const & )noexcept = delete;
		RenderPassHolder( RenderPassHolder && )noexcept = delete;
		RenderPassHolder & operator=( RenderPassHolder && )noexcept = delete;
		CRG_API RenderPassHolder( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, uint32_t maxPassCount
			, Extent2D size = {} );
		CRG_API ~RenderPassHolder()noexcept;

		CRG_API bool initialise( RecordContext & context
			, crg::RunnablePass const & runnable
			, uint32_t passIndex );
		CRG_API VkRenderPassBeginInfo getBeginInfo( uint32_t index )const;
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
			return m_passes[index].renderPass;
		}

		Extent2D const & getRenderSize()const
		{
			return m_size;
		}

		Rect2D const & getRenderArea( uint32_t index )const
		{
			return m_passes[index].renderArea;
		}

		std::vector< VkClearValue > const & getClearValues( uint32_t index )const
		{
			return m_passes[index].clearValues;
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
		void doInitialiseRenderArea( uint32_t index );
		VkFramebuffer doCreateFramebuffer( uint32_t passIndex )const;

	private:
		struct PassData
		{
			VkRenderPass renderPass{};
			mutable VkFramebuffer frameBuffer{};
			Rect2D renderArea{};
			std::vector< Attachment const * > attachments;
			std::vector< VkClearValue > clearValues;
			std::vector< Entry > attaches;
			PipelineState previousState;
			PipelineState nextState;

			void cleanup( crg::GraphContext & context )noexcept;
		};

		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		Extent2D m_size;
		std::vector< PassData > m_passes;
		PassData const * m_currentPass{};
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
		uint32_t m_layers{};
		uint32_t m_index{};
		uint32_t m_count{ 1u };
	};
}
