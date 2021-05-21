/*
See LICENSE file in root folder.
*/
#pragma once

#include "PipelinePass.hpp"

namespace crg
{
	VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
		, uint32_t maxSets );

	template< typename VkType, typename LibType >
	inline std::vector< VkType > makeVkArray( std::vector< LibType > const & input )
	{
		std::vector< VkType > result;
		result.reserve( input.size() );

		for ( auto const & element : input )
		{
			result.emplace_back( element );
		}

		return result;
	}

	class RenderPass
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

	public:
		RenderPass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph );
		~RenderPass();

		VkRenderPass getRenderPass()const
		{
			return m_renderPass;
		}

	protected:
		void doInitialise()override final;
		void doRecordInto( VkCommandBuffer commandBuffer )const override final;
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const override final;
		virtual void doSubInitialise() = 0;
		virtual void doSubRecordInto( VkCommandBuffer commandBuffer )const = 0;
		virtual VkSubpassContents doGetSubpassContents( uint32_t subpassIndex )const
		{
			return VK_SUBPASS_CONTENTS_INLINE;
		}

		void doCreateRenderPass();
		void doCreateFramebuffer();
		VkPipelineColorBlendStateCreateInfo doCreateBlendState();

	protected:
		VkRenderPass m_renderPass{ VK_NULL_HANDLE };
		VkFramebuffer m_frameBuffer{ VK_NULL_HANDLE };
		VkRect2D m_renderArea{};
		std::vector< VkClearValue > m_clearValues;
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
	};
}
