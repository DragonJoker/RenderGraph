/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/RenderPassHolder.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	//*********************************************************************************************

	namespace rpHolder
	{
		static VkAttachmentReference addAttach( RecordContext & context
			, Attachment const & attach
			, ImageViewId view
			, VkAttachmentDescriptionArray & attaches
			, std::vector< RenderPassHolder::Entry > & viewAttaches
			, std::vector< VkClearValue > & clearValues
			, LayoutState initialLayout
			, LayoutState finalLayout
			, bool separateDepthStencilLayouts )
		{
			VkAttachmentReference result{ uint32_t( attaches.size() )
				, convert( attach.getImageLayout( separateDepthStencilLayouts ) ) };
			attaches.push_back( { 0u
				, view.data->info.format
				, view.data->image.data->info.samples
				, ( initialLayout.layout == ImageLayout::eUndefined
					? VK_ATTACHMENT_LOAD_OP_CLEAR
					: attach.getLoadOp() )
				, attach.getStoreOp()
				, attach.getStencilLoadOp()
				, attach.getStencilStoreOp()
				, convert( initialLayout.layout )
				, convert( finalLayout.layout ) } );
			viewAttaches.push_back( { view, initialLayout, finalLayout } );
			clearValues.push_back( attach.getClearValue() );

			if ( view.data->source.empty() )
			{
				context.setLayoutState( view, finalLayout );
			}
			else
			{
				for ( auto & source : view.data->source )
				{
					context.setLayoutState( source, finalLayout );
				}
			}

			return result;
		}

		static VkAttachmentReference addAttach( RecordContext & context
			, Attachment const & attach
			, ImageViewId view
			, VkAttachmentDescriptionArray & attaches
			, std::vector< RenderPassHolder::Entry > & viewAttaches
			, std::vector< VkClearValue > & clearValues
			, VkPipelineColorBlendAttachmentStateArray & blendAttachs
			, LayoutState initialLayout
			, LayoutState finalLayout
			, bool separateDepthStencilLayouts )
		{
			blendAttachs.push_back( attach.getBlendState() );
			return addAttach( context
				, attach
				, view
				, attaches
				, viewAttaches
				, clearValues
				, initialLayout
				, finalLayout
				, separateDepthStencilLayouts );
		}

		static bool operator==( PipelineState const & lhs, PipelineState const & rhs )
		{
			return lhs.access == rhs.access
				&& lhs.pipelineStage == rhs.pipelineStage;
		}

		static bool operator!=( LayoutState const & lhs, LayoutState const & rhs )
		{
			return lhs.layout != rhs.layout
				|| lhs.state.access != rhs.state.access
				|| ( lhs.state.access != AccessFlags::eNone && lhs.state.pipelineStage != rhs.state.pipelineStage );
		}

		static bool checkAttaches( RecordContext const & context
			, std::vector< RenderPassHolder::Entry > const & attaches
			, uint32_t passIndex )
		{
			auto it = std::find_if( attaches.begin()
				, attaches.end()
				, [&context, passIndex]( RenderPassHolder::Entry const & lookup )
				{
					auto layout = context.getLayoutState( resolveView( lookup.view, passIndex ) );
					return layout != lookup.input
						&& layout.layout != ImageLayout::eUndefined;
				} );
			return it == attaches.end();
		}
	}

	//*********************************************************************************************

	void RenderPassHolder::PassData::cleanup( crg::GraphContext & context )noexcept
	{
		attaches.clear();
		clearValues.clear();

		if ( frameBuffer )
		{
			crgUnregisterObject( context, frameBuffer );
			context.vkDestroyFramebuffer( context.device
				, frameBuffer
				, context.allocator );
			frameBuffer = {};
		}

		if ( renderPass )
		{
			crgUnregisterObject( context, renderPass );
			context.vkDestroyRenderPass( context.device
				, renderPass
				, context.allocator );
			renderPass = {};
		}
	}

	//*********************************************************************************************

	RenderPassHolder::RenderPassHolder( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, uint32_t maxPassCount
		, VkExtent2D size )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_size{ std::move( size ) }
	{
		m_passes.resize( maxPassCount );
	}

	RenderPassHolder::~RenderPassHolder()noexcept
	{
		for ( auto & data : m_passes )
		{
			data.cleanup( m_context );
		}
	}

	bool RenderPassHolder::initialise( RecordContext & context
		, crg::RunnablePass const & runnable
		, uint32_t passIndex )
	{
		using rpHolder::operator==;

		auto & data = m_passes[passIndex];
		auto previousState = context.getPrevPipelineState();
		auto nextState = context.getNextPipelineState();

		if ( data.renderPass
			&& rpHolder::checkAttaches( context, data.attaches, passIndex )
			&& data.previousState == previousState
			&& data.nextState == nextState )
		{
			return false;
		}

		data.cleanup( m_context );
		doCreateRenderPass( context
			, runnable
			, previousState
			, nextState
			, passIndex );
		doInitialiseRenderArea( passIndex );
		return true;
	}

	VkRenderPassBeginInfo RenderPassHolder::getBeginInfo( uint32_t index )const
	{
		auto frameBuffer = getFramebuffer( index );
		return VkRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
			, nullptr
			, getRenderPass( index )
			, frameBuffer
			, getRenderArea( index )
			, uint32_t( getClearValues( index ).size() )
			, getClearValues( index ).data() };
	}

	void RenderPassHolder::begin( RecordContext & context
		, VkCommandBuffer commandBuffer
		, VkSubpassContents subpassContents
		, uint32_t index )
	{
		m_index = index;
		m_currentPass = &m_passes[m_index];

		for ( auto & attach : m_currentPass->attaches )
		{
			context.setLayoutState( resolveView( attach.view, m_index )
				, attach.input );
		}

		auto beginInfo = getBeginInfo( m_index );
		m_context.vkCmdBeginRenderPass( commandBuffer
			, &beginInfo
			, subpassContents );
	}

	void RenderPassHolder::end( RecordContext & context
			, VkCommandBuffer commandBuffer )
	{
		m_context.vkCmdEndRenderPass( commandBuffer );

		for ( auto & attach : m_currentPass->attaches )
		{
			context.setLayoutState( resolveView( attach.view, m_index )
				, attach.output );
		}

		m_currentPass = nullptr;
	}

	void RenderPassHolder::doCreateRenderPass( RecordContext & context
			, crg::RunnablePass const & runnable
			, PipelineState const & previousState
			, PipelineState const & nextState
			, uint32_t passIndex )
	{
		VkAttachmentDescriptionArray attaches;
		VkAttachmentReferenceArray colorReferences;
		VkAttachmentReference depthReference{};
		auto & data = m_passes[passIndex];
		m_blendAttachs.clear();

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isDepthAttach()
				|| attach.isStencilAttach()
				|| attach.isColourAttach() )
			{
				auto view = attach.view( passIndex );
				auto resolved = resolveView( view, passIndex );
				auto currentLayout = m_graph.getCurrentLayoutState( context, resolved );
				auto nextLayout = m_graph.getNextLayoutState( context, runnable, resolved );
				auto from = ( !attach.isInput()
					? crg::makeLayoutState( ImageLayout::eUndefined )
					: currentLayout );
				checkUndefinedInput( "RenderPass", attach, resolved, from.layout );

				if ( attach.isDepthAttach() || attach.isStencilAttach() )
				{
					depthReference = rpHolder::addAttach( context
						, attach
						, view
						, attaches
						, data.attaches
						, data.clearValues
						, from
						, nextLayout
						, m_context.separateDepthStencilLayouts );
				}
				else if ( attach.isColourAttach() )
				{
					colorReferences.push_back( rpHolder::addAttach( context
						, attach
						, view
						, attaches
						, data.attaches
						, data.clearValues
						, m_blendAttachs
						, from
						, nextLayout
						, m_context.separateDepthStencilLayouts ) );
				}
			}
		}

		VkSubpassDescription subpassDesc{ 0u
			, VK_PIPELINE_BIND_POINT_GRAPHICS
			, 0u
			, nullptr
			, uint32_t( colorReferences.size() )
			, colorReferences.data()
			, nullptr
			, depthReference.layout ? &depthReference : nullptr
			, 0u
			, nullptr };
		data.previousState = previousState;
		data.nextState = nextState;
		VkSubpassDependencyArray dependencies{
			{ VK_SUBPASS_EXTERNAL
				, 0u
				, convert( previousState.pipelineStage )
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, convert( previousState.access )
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, VK_DEPENDENCY_BY_REGION_BIT }
			, { 0u
				, VK_SUBPASS_EXTERNAL
				, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				, convert( nextState.pipelineStage )
				, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
				, convert( nextState.access )
				, VK_DEPENDENCY_BY_REGION_BIT } };
		VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( attaches.size() )
			, attaches.data()
			, 1u
			, &subpassDesc
			, uint32_t( dependencies.size() )
			, dependencies.data() };
		auto res = m_context.vkCreateRenderPass( m_context.device
			, &createInfo
			, m_context.allocator
			, &data.renderPass );
		checkVkResult( res, m_pass.getGroupName() + " - RenderPass creation" );
		crgRegisterObject( m_context, m_pass.getGroupName() + std::to_string( m_count++ ), data.renderPass );
	}

	VkPipelineColorBlendStateCreateInfo RenderPassHolder::createBlendState()
	{
		return { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
			, nullptr
			, 0u
			, VK_FALSE
			, VK_LOGIC_OP_COPY
			, uint32_t( m_blendAttachs.size() )
			, m_blendAttachs.data()
			, {} };
	}

	VkFramebuffer RenderPassHolder::getFramebuffer( uint32_t index )const
	{
		return doCreateFramebuffer( index );
	}

	void RenderPassHolder::doInitialiseRenderArea( uint32_t index )
	{
		uint32_t width{ m_size.width };
		uint32_t height{ m_size.height };
		m_passes[index].attachments.clear();
		m_layers = 1u;

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isColourAttach()
				|| attach.isDepthAttach()
				|| attach.isStencilAttach() )
			{
				auto view = attach.view();
				m_passes[index].attachments.push_back( &attach );
				width = std::max( width
					, view.data->image.data->info.extent.width >> view.data->info.subresourceRange.baseMipLevel );
				height = std::max( height
					, view.data->image.data->info.extent.height >> view.data->info.subresourceRange.baseMipLevel );
				m_layers = std::max( m_layers
					, view.data->info.subresourceRange.layerCount );
			}
		}

		m_passes[index].renderArea.extent.width = width;
		m_passes[index].renderArea.extent.height = height;
	}

	VkFramebuffer RenderPassHolder::doCreateFramebuffer( uint32_t passIndex )const
	{
		auto & data = m_passes[passIndex];
		auto frameBuffer = &data.frameBuffer;

		if ( !*frameBuffer )
		{
			VkImageViewArray attachments;

			for ( auto & attach : data.attachments )
			{
				attachments.push_back( m_graph.createImageView( attach->view( passIndex ) ) );
			}

			VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
				, nullptr
				, 0u
				, data.renderPass
				, uint32_t( attachments.size() )
				, attachments.data()
				, data.renderArea.extent.width
				, data.renderArea.extent.height
				, m_layers };
			auto res = m_context.vkCreateFramebuffer( m_context.device
				, &createInfo
				, m_context.allocator
				, frameBuffer );
			auto name = m_pass.getGroupName() + std::string( "[" ) + std::to_string( passIndex ) + std::string( "]" );
			checkVkResult( res, name + " - Framebuffer creation" );
			crgRegisterObject( m_context, name, *frameBuffer );
		}

		return *frameBuffer;
	}
}
