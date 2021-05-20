/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	FramePass::FramePass( std::string const & name
		, RunnablePassCreator runnableCreator )
		: name{ name }
		, runnableCreator{ runnableCreator }
	{
	}

	bool FramePass::dependsOn( FramePass const & pass )const
	{
		auto it = std::find_if( depends.begin()
			, depends.end()
			, [&pass]( FramePass const * lookup )
			{
				return pass.name == lookup->name
					|| lookup->dependsOn( pass );
			} );
		return it != depends.end();
	}

	bool FramePass::directDependsOn( FramePass const & pass )const
	{
		auto it = std::find_if( depends.begin()
			, depends.end()
			, [&pass]( FramePass const * lookup )
			{
				return pass.name == lookup->name;
			} );
		return it != depends.end();
	}

	void FramePass::addUniformBuffer( VkBuffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		WriteDescriptorSet write{ binding
			, 0u
			, 1u
			, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		write.bufferInfo.push_back( { buffer, offset, range } );
		buffers.push_back( write );
	}

	void FramePass::addStorageBuffer( VkBuffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		WriteDescriptorSet write{ binding
			, 0u
			, 1u
			, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
		write.bufferInfo.push_back( { buffer, offset, range } );
		buffers.push_back( write );
	}

	void FramePass::addUniformBufferView( VkBuffer buffer
		, VkBufferView view
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		WriteDescriptorSet write{ binding
			, 0u
			, 1u
			, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER };
		write.bufferInfo.push_back( { buffer, offset, range } );
		write.texelBufferView.push_back( view );
		bufferViews.push_back( write );
	}

	void FramePass::addStorageBufferView( VkBuffer buffer
		, VkBufferView view
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		WriteDescriptorSet write{ binding
			, 0u
			, 1u
			, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER };
		write.bufferInfo.push_back( { buffer, offset, range } );
		write.texelBufferView.push_back( view );
		bufferViews.push_back( write );
	}

	void FramePass::addSampledView( std::string name
		, ImageViewId view
		, VkImageLayout initialLayout
		, uint32_t binding
		, VkFilter filter )
	{
		sampled.push_back( { Attachment::FlagKind( Attachment::Flag::Sampled )
			, *this
			, std::move( name )
			, std::move( view )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, initialLayout
			, binding
			, filter
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addStorageView( std::string name
		, ImageViewId view
		, VkImageLayout initialLayout
		, uint32_t binding )
	{
		sampled.push_back( { Attachment::FlagKind( Attachment::Flag::Storage )
			, *this
			, std::move( name )
			, std::move( view )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, initialLayout
			, binding
			, VkFilter{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addColourView( std::string name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		colourInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, std::move( name )
			, std::move( view )
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, finalLayout
			, uint32_t{}
			, VkFilter{}
			, std::move( clearValue )
			, std::move( blendState ) } );
	}

	void FramePass::addDepthStencilView( std::string name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		depthStencilInOut = { Attachment::FlagKind( Attachment::Flag::Depth )
			, *this
			, std::move( name )
			, std::move( view )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, finalLayout
			, uint32_t{}
			, VkFilter{}
			, std::move( clearValue )
			, VkPipelineColorBlendAttachmentState{} };
	}

	void FramePass::addTransferInputView( std::string name
		, ImageViewId view )
	{
		transferInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, std::move( name )
			, std::move( view )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkImageLayout{}
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, uint32_t{}
			, VkFilter{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferOutputView( std::string name
		, ImageViewId view )
	{
		transferInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, std::move( name )
			, std::move( view )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkImageLayout{}
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, uint32_t{}
			, VkFilter{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext const & context
		, RunnableGraph & graph )const
	{
		auto result = runnableCreator( *this, context, graph );
		result->initialise();
		return result;
	}
}
