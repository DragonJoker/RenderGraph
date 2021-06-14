/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePass.hpp"

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
			, RunnableGraph & graph
			, VkExtent2D const & size = {}
			, uint32_t maxPassCount = 1u );
		CRG_API ~RenderPass();

		VkRenderPass getRenderPass()const
		{
			return m_renderPass;
		}

	protected:
		CRG_API void doInitialise( uint32_t index )override final;
		CRG_API void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index )override;
		CRG_API VkPipelineStageFlags doGetSemaphoreWaitFlags()const override final;
		CRG_API virtual void doSubInitialise( uint32_t index ) = 0;
		CRG_API virtual void doSubRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index ) = 0;
		CRG_API virtual VkSubpassContents doGetSubpassContents( uint32_t subpassIndex )const
		{
			return VK_SUBPASS_CONTENTS_INLINE;
		}

		CRG_API void doCreateRenderPass();
		CRG_API void doCreateFramebuffer();
		CRG_API VkPipelineColorBlendStateCreateInfo doCreateBlendState();

	protected:
		VkExtent2D m_size;
		VkRenderPass m_renderPass{ VK_NULL_HANDLE };
		std::vector< VkFramebuffer > m_frameBuffers;
		VkRect2D m_renderArea{};
		std::vector< VkClearValue > m_clearValues;
		VkPipelineColorBlendAttachmentStateArray m_blendAttachs;
	};
}
