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

			if ( attach.view().data->source.empty() )
			{
				result.push_back( attach );
			}
			else
			{
				for ( auto & view : attach.view().data->source )
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
			auto it = std::find_if( pass.images.begin()
				, pass.images.end()
				, [&view]( Attachment const & lookup )
				{
					return matchView( lookup
							, view
							, []( Attachment const & lhs
								, ImageViewId const & rhs )
							{
								return lhs.view() == rhs
									&& lhs.isOutput();
							} );
				} );

			return it != pass.images.end();
		}

		bool isInInputs( FramePass const & pass
			, ImageViewId const & view )
		{
			auto it = std::find_if( pass.images.begin()
				, pass.images.end()
				, [&view]( Attachment const & lookup )
				{
					return ( lookup.isInput() )
						&& matchView( lookup
							, view
							, []( Attachment const & lhs
								, ImageViewId const & rhs )
							{
								return lhs.view() == rhs
									&& ( lhs.isInput() );
							} );
				} );

			return it != pass.images.end();
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

		if ( data.info.subresourceRange.layerCount > 1u )
		{
			switch ( data.info.viewType )
			{
			case VK_IMAGE_VIEW_TYPE_1D:
				data.info.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			case VK_IMAGE_VIEW_TYPE_2D:
				if ( ( data.image.data->info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT )
					&& ( data.info.subresourceRange.layerCount % 6u ) == 0u
					&& data.info.subresourceRange.baseArrayLayer == 0u )
				{
					if ( data.info.subresourceRange.layerCount > 6u )
					{
						data.info.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					}
					else
					{
						data.info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					}
				}
				else
				{
					data.info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				}
				break;
			default:
				break;
			}
		}

		return graph.createView( data );
	}

	void FramePass::addSampledView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc )
	{
		auto attachName = name + view.data->name + "Spl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Sampled ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, { view }
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

	void FramePass::addInputStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, { view }
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

	void FramePass::addOutputStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, { view }
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

	void FramePass::addInOutStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, { view }
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
		auto attachName = name + view.data->name + "It";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferOutputView( ImageViewId view
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Ot";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, uint32_t{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInOutView( ImageViewId view )
	{
		auto attachName = name + view.data->name + "IOt";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
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
		auto attachName = this->name + view.data->name + name;
		images.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, attachName
			, { view }
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

	void FramePass::addDepthView( std::string const & name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Depth )
				, *this
				, attachName
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addStencilView( std::string const & name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Stencil )
				, *this
				, attachName
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
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
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Depth ) | Attachment::FlagKind( Attachment::Flag::Stencil )
				, *this
				, attachName
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addSampledView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc )
	{
		auto attachName = name + views.front().data->name + "Spl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Sampled ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addInputStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addOutputStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addInOutStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Storage ) | Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addTransferInputView( ImageViewIdArray views
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "It";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addTransferOutputView( ImageViewIdArray views
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Ot";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addTransferInOutView( ImageViewIdArray views )
	{
		auto attachName = name + views.front().data->name + "IOt";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Transfer ) | Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, attachName
			, std::move( views )
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
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, attachName
			, std::move( views )
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

	void FramePass::addDepthView( std::string const & name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Depth )
				, *this
				, attachName
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addStencilView( std::string const & name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Stencil )
				, *this
				, attachName
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addDepthStencilView( std::string const & name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::Depth ) | Attachment::FlagKind( Attachment::Flag::Stencil )
				, *this
				, attachName
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, uint32_t{}
				, SamplerDesc{}
				, std::move( clearValue )
				, VkPipelineColorBlendAttachmentState{} } );
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext const & context
		, RunnableGraph & graph )const
	{
		return runnableCreator( *this, context, graph );
	}
}
