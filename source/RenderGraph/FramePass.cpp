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
					return matchView( lookup
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

		bool isInOutputs( FramePass const & pass
			, Buffer const & buffer )
		{
			auto it = std::find_if( pass.buffers.begin()
				, pass.buffers.end()
				, [&buffer]( Attachment const & lookup )
				{
					return lookup.isStorageBuffer()
						&& lookup.buffer.buffer == buffer;
				} );

			return it != pass.buffers.end();
		}

		bool isInInputs( FramePass const & pass
			, Buffer const & buffer )
		{
			auto it = std::find_if( pass.buffers.begin()
				, pass.buffers.end()
				, [&buffer]( Attachment const & lookup )
				{
					return lookup.isStorageBuffer()
						&& lookup.buffer.buffer == buffer;
				} );

			return it != pass.buffers.end();
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

	bool FramePass::dependsOn( FramePass const & pass
		, Buffer const & buffer )const
	{
		auto it = std::find_if( depends.begin()
			, depends.end()
			, [&pass, &buffer]( FramePass const * lookup )
			{
				bool result = false;

				if ( isInOutputs( *lookup, buffer ) )
				{
					result = ( pass.name == lookup->name );
				}
				else if ( !isInInputs( *lookup, buffer ) )
				{
					result = lookup->dependsOn( pass, buffer );
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

	void FramePass::addUniformBuffer( Buffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "UB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Uniform )
			, buffer
			, offset
			, range } );
	}

	void FramePass::addInputStorageBuffer( Buffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "ISB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, buffer
			, offset
			, range } );
	}

	void FramePass::addOutputStorageBuffer( Buffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "OSB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, buffer
			, offset
			, range } );
	}

	void FramePass::addInOutStorageBuffer( Buffer buffer
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "IOSB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, buffer
			, offset
			, range } );
	}

	void FramePass::addUniformBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "UBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Uniform ) | BufferAttachment::FlagKind( BufferAttachment::Flag::View )
			, buffer
			, offset
			, range } );
	}

	void FramePass::addStorageBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, VkDeviceSize offset
		, VkDeviceSize range )
	{
		auto attachName = name + buffer.name + "SBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage ) | BufferAttachment::FlagKind( BufferAttachment::Flag::View )
			, buffer
			, offset
			, range } );
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
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Sampled )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, std::move( samplerDesc )
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addInputStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addOutputStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addInOutStorageView( ImageViewId view
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInputView( ImageViewId view
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "It";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferOutputView( ImageViewId view
		, VkImageLayout initialLayout )
	{
		auto attachName = name + view.data->name + "Ot";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInOutView( ImageViewId view )
	{
		auto attachName = name + view.data->name + "IOt";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, { view }
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VkImageLayout{}
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addColourView( std::string const & name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		auto attachName = this->name + view.data->name + name;
		images.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, { view }
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, Attachment::FlagKind( ImageAttachment::Flag::Depth )
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + view.data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, Attachment::FlagKind( ImageAttachment::Flag::Depth ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
				, { view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Sampled )
			, std::move( views )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, std::move( samplerDesc )
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addInputStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addOutputStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addInOutStorageView( ImageViewIdArray views
		, uint32_t binding
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Str";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInputView( ImageViewIdArray views
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "It";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferOutputView( ImageViewIdArray views
		, VkImageLayout initialLayout )
	{
		auto attachName = name + views.front().data->name + "Ot";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, initialLayout
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addTransferInOutView( ImageViewIdArray views )
	{
		auto attachName = name + views.front().data->name + "IOt";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input ) | Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkAttachmentLoadOp{}
			, VkAttachmentStoreOp{}
			, VkImageLayout{}
			, VkImageLayout{}
			, SamplerDesc{}
			, VkClearValue{}
			, VkPipelineColorBlendAttachmentState{} } );
	}

	void FramePass::addColourView( std::string const & name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.push_back( { Attachment::FlagKind( Attachment::Flag::None )
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, std::move( views )
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto attachName = this->name + views.front().data->name + name;
		images.insert( images.begin()
			, { Attachment::FlagKind( Attachment::Flag::None )
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, initialLayout
				, finalLayout
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
