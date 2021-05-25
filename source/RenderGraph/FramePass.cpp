/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	namespace
	{
		bool isInOutputs( FramePass const & pass
			, ImageViewId const & view )
		{
			auto it = std::find_if( pass.colourInOuts.begin()
				, pass.colourInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view
						&& lookup.isColourOutput();
				} );

			if ( it != pass.colourInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.transferInOuts.begin()
				, pass.transferInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view
						&& lookup.isTransferOutput();
				} );

			if ( it != pass.transferInOuts.end() )
			{
				return true;
			}

			if ( pass.depthStencilInOut
				&& pass.depthStencilInOut->view == view
				&& ( pass.depthStencilInOut->isDepthOutput()
					|| pass.depthStencilInOut->isStencilOutput() ) )
			{
				return true;
			}

			return false;
		}

		bool isInInputs( FramePass const & pass
			, ImageViewId const & view )
		{
			auto it = std::find_if( pass.colourInOuts.begin()
				, pass.colourInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view
						&& lookup.isColourInput();
				} );

			if ( it != pass.colourInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.transferInOuts.begin()
				, pass.transferInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view
						&& lookup.isTransferInput();
				} );

			if ( it != pass.transferInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.sampled.begin()
				, pass.sampled.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view;
				} );

			if ( it != pass.sampled.end() )
			{
				return true;
			}

			it = std::find_if( pass.storage.begin()
				, pass.storage.end()
				, [&view]( Attachment const & lookup )
				{
					return lookup.view == view;
				} );

			if ( it != pass.storage.end() )
			{
				return true;
			}

			if ( pass.depthStencilInOut
				&& pass.depthStencilInOut->view == view
				&& ( pass.depthStencilInOut->isDepthInput()
					|| pass.depthStencilInOut->isStencilInput() ) )
			{
				return true;
			}

			return false;
		}
	}

	FramePass::FramePass( std::string const & name
		, RunnablePassCreator runnableCreator )
		: name{ name }
		, runnableCreator{ runnableCreator }
	{
	}

	bool FramePass::dependsOn( FramePass const & pass
		, ImageViewId const & view )const
	{
		auto it = std::find_if( depends.begin()
			, depends.end()
			, [&pass, &view]( FramePass const * lookup )
			{
				bool result = false;

				if ( isInOutputs( *lookup, view ) )
				{
					result = ( pass.name == lookup->name );
				}
				else if ( !isInInputs( *lookup, view ) )
				{
					result = lookup->dependsOn( pass, view );
				}

				return result;
			} );
		return it != depends.end();
	}

	bool FramePass::dependsOn( FramePass const & pass )const
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

	void FramePass::addSampledView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc )
	{
		sampled.push_back( { Attachment::FlagKind( Attachment::Flag::Sampled )
			, *this
			, name + view.data->name + "Sampled"
			, std::move( view )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, binding
			, std::move( samplerDesc )
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		sampled.push_back( { Attachment::FlagKind( Attachment::Flag::Storage )
			, *this
			, name + view.data->name + "Storage"
			, std::move( view )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, binding
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInputView( ImageViewId view
		, VkImageLayout initialLayout )
	{
		transferInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, this->name + view.data->name + "InTransfer"
			, std::move( view )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferOutputView( ImageViewId view
		, VkImageLayout initialLayout )
	{
		transferInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, this->name + view.data->name + "OutTransfer"
			, std::move( view )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addColourView( std::string const & name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		colourInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, this->name + view.data->name + name
			, std::move( view )
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, std::move( clearValue )
			, std::move( blendState ) } );
	}

	void FramePass::addDepthStencilView( std::string const & name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		depthStencilInOut = { Attachment::FlagKind( Attachment::Flag::Depth )
			, *this
			, this->name + view.data->name + name
			, std::move( view )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, std::move( clearValue )
			, VkPipelineColorBlendAttachmentState{} };
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext const & context
		, RunnableGraph & graph )const
	{
		auto result = runnableCreator( *this, context, graph );
		result->initialise();
		return result;
	}
}
