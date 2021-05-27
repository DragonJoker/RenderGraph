/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePass.hpp"

#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	namespace
	{
		AttachmentArray splitAttach( Attachment const & attach )
		{
			AttachmentArray result;

			if ( attach.view.data->source.empty() )
			{
				result.push_back( attach );
			}
			else
			{
				for ( auto & view : attach.view.data->source )
				{
					result.push_back( Attachment{ view, attach } );
				}
			}

			return result;
		}

		ImageViewIdArray splitView( ImageViewId const & view )
		{
			ImageViewIdArray result;

			if ( view.data->source.empty() )
			{
				result.push_back( view );
			}
			else
			{
				for ( auto & view : view.data->source )
				{
					result.push_back( view );
				}
			}

			return result;
		}

		template< typename PredT >
		bool matchView( Attachment const & lhs
			, ImageViewId const & rhs
			, PredT predicate )
		{
			auto lhsAttaches = splitAttach( lhs );
			auto rhsViews = splitView( rhs );

			for ( auto & lhsAttach : lhsAttaches )
			{
				for ( auto & rhsView : rhsViews )
				{
					if ( predicate( lhsAttach, rhsView ) )
					{
						return true;
					}
				}
			}

			return false;
		}

		bool isInOutputs( FramePass const & pass
			, ImageViewId const & view )
		{
			auto it = std::find_if( pass.colourInOuts.begin()
				, pass.colourInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs
								&& lhs.isColourOutput();
						} );
				} );

			if ( it != pass.colourInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.transferInOuts.begin()
				, pass.transferInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs
								&& lhs.isTransferOutput();
						} );
				} );

			if ( it != pass.transferInOuts.end() )
			{
				return true;
			}

			if ( pass.depthStencilInOut )
			{
				return matchView( *pass.depthStencilInOut
					, view
					, []( Attachment const & lhs
						, ImageViewId const & rhs )
					{
						return lhs.view == rhs
							&& ( lhs.isDepthOutput()
								|| lhs.isStencilOutput() );
					} );
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
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs
								&& lhs.isColourInput();
						} );
				} );

			if ( it != pass.colourInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.transferInOuts.begin()
				, pass.transferInOuts.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs
								&& lhs.isTransferInput();
						} );
				} );

			if ( it != pass.transferInOuts.end() )
			{
				return true;
			}

			it = std::find_if( pass.sampled.begin()
				, pass.sampled.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs;
						} );
				} );

			if ( it != pass.sampled.end() )
			{
				return true;
			}

			it = std::find_if( pass.storage.begin()
				, pass.storage.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
						, view
						, []( Attachment const & lhs
							, ImageViewId const & rhs )
						{
							return lhs.view == rhs;
						} );
				} );

			if ( it != pass.storage.end() )
			{
				return true;
			}

			if ( pass.depthStencilInOut )
			{
				return matchView( *pass.depthStencilInOut
					, view
					, []( Attachment const & lhs
						, ImageViewId const & rhs )
					{
						return lhs.view == rhs
							&& ( lhs.isDepthInput()
								|| lhs.isStencilInput() );
					} );
			}

			return false;
		}
	}

	FramePass::FramePass( FrameGraph & graph
		, std::string const & name
		, RunnablePassCreator runnableCreator )
		: graph{ graph }
		, name{ name }
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

	ImageViewId FramePass::mergeViews( ImageViewIdArray const & views
		, bool mergeMipLevels
		, bool mergeArrayLayers )
	{
		ImageViewData data;

		for ( auto & view : views )
		{
			if ( data.image.id == 0 )
			{
				data.image = view.data->image;
				data.name = data.image.data->name;
				data.info.components = view.data->info.components;
				data.info.flags = view.data->info.flags;
				data.info.format = view.data->info.format;
				data.info.viewType = view.data->info.viewType;
				data.info.subresourceRange = view.data->info.subresourceRange;
			}
			else
			{
				assert( data.image == view.data->image );

				if ( mergeMipLevels )
				{
					auto maxLevel = std::max( data.info.subresourceRange.levelCount + data.info.subresourceRange.baseMipLevel
						, view.data->info.subresourceRange.levelCount + view.data->info.subresourceRange.baseMipLevel );
					data.info.subresourceRange.baseMipLevel = std::min( view.data->info.subresourceRange.baseMipLevel
						, data.info.subresourceRange.baseMipLevel );
					data.info.subresourceRange.levelCount = maxLevel - data.info.subresourceRange.baseMipLevel;
				}
				else
				{
					data.info.subresourceRange.baseMipLevel = std::min( view.data->info.subresourceRange.baseMipLevel
						, data.info.subresourceRange.baseMipLevel );
					data.info.subresourceRange.levelCount = 1u;
				}

				if ( mergeArrayLayers )
				{
					auto maxLayer = std::max( data.info.subresourceRange.layerCount + data.info.subresourceRange.baseArrayLayer
						, view.data->info.subresourceRange.layerCount + view.data->info.subresourceRange.baseArrayLayer );
					data.info.subresourceRange.baseArrayLayer = std::min( view.data->info.subresourceRange.baseArrayLayer
						, data.info.subresourceRange.baseArrayLayer );
					data.info.subresourceRange.layerCount = maxLayer - data.info.subresourceRange.baseArrayLayer;
				}
				else
				{
					data.info.subresourceRange.baseArrayLayer = std::min( view.data->info.subresourceRange.baseArrayLayer
						, data.info.subresourceRange.baseArrayLayer );
					data.info.subresourceRange.layerCount = 1u;
				}
			}

			data.source.push_back( view );
		}

		return graph.createView( data );
	}

	void FramePass::addSampledView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc )
	{
		sampled.push_back( { Attachment::FlagKind( Attachment::Flag::Sampled )
			, *this
			, name + view.data->name + "Spl"
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
			, name + view.data->name + "Str"
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
			, this->name + view.data->name + "It"
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
			, this->name + view.data->name + "Ot"
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

	void FramePass::addTransferInOutView( ImageViewId view )
	{
		transferInOuts.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, this->name + view.data->name + "IOt"
			, std::move( view )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkImageLayout{}
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
