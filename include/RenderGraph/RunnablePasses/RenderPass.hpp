/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/PipelinePass.hpp"

namespace crg
{
	CRG_API VkDescriptorPoolSizeArray getBindingsSizes( VkDescriptorSetLayoutBindingArray const & bindings
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
		CRG_API RenderPass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph );
		CRG_API ~RenderPass();

		VkRenderPass getRenderPass()const
		{
			return m_renderPass;
		}

	protected:
		CRG_API void doInitialise()override final;
		CRG_API void doRecordInto( VkCommandBuffer commandBuffer )const override;
		CRG_API VkPipelineStageFlags doGetSemaphoreWaitFlags()const override final;
		CRG_API virtual void doSubInitialise() = 0;
		CRG_API virtual void doSubRecordInto( VkCommandBuffer commandBuffer )const = 0;
		CRG_API virtual VkSubpassContents doGetSubpassContents( uint32_t subpassIndex )const
		{
			return VK_SUBPASS_CONTENTS_INLINE;
		}

		CRG_API void doCreateRenderPass();
		CRG_API void doCreateFramebuffer();
		CRG_API VkPipelineColorBlendStateCreateInfo doCreateBlendState();

	protected:
		VkRenderPass m_renderPass{ VK_NULL_HANDLE };
		VkFramebuffer m_frameBuffer{ VK_NULL_HANDLE };
		VkRect2D m_renderArea{};
		std::vector< VkClearValue > m_clearValues;
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
	};
}
