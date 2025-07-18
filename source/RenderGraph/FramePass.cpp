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
	namespace fpass
	{
		static AttachmentArray splitAttach( Attachment const & attach )
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
					result.emplace_back( view, attach );
				}
			}

			return result;
		}

		static ImageViewIdArray splitView( ImageViewId const & view )
		{
			ImageViewIdArray result;

			if ( view.data->source.empty() )
			{
				result.push_back( view );
			}
			else
			{
				for ( auto & subview : view.data->source )
				{
					result.push_back( subview );
				}
			}

			return result;
		}

		template< typename PredT >
		static bool matchView( Attachment const & lhs
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

		static bool isInOutputs( FramePass const & pass
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
								return lhs.isOutput() && match( lhs.view(), rhs );
							} );
				} );

			return it != pass.images.end();
		}

		static bool isInInputs( FramePass const & pass
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
							return lhs.isInput() && match( lhs.view(), rhs );
						} );
				} );

			return it != pass.images.end();
		}

		static bool isInOutputs( FramePass const & pass
			, Buffer const & buffer )
		{
			auto it = std::find_if( pass.buffers.begin()
				, pass.buffers.end()
				, [&buffer]( Attachment const & lookup )
				{
					return lookup.isStorageBuffer()
						&& lookup.bufferAttach.buffer == buffer;
				} );

			return it != pass.buffers.end();
		}

		static bool isInInputs( FramePass const & pass
			, Buffer const & buffer )
		{
			auto it = std::find_if( pass.buffers.begin()
				, pass.buffers.end()
				, [&buffer]( Attachment const & lookup )
				{
					return lookup.isStorageBuffer()
						&& lookup.bufferAttach.buffer == buffer;
				} );

			return it != pass.buffers.end();
		}

		static std::string adjustName( FramePass const & pass
			, std::string const & dataName )
		{
			auto result = pass.getGroupName() + "/" + dataName;
			uint32_t index = 0u;

			while ( result[index] == '/' )
			{
				++index;
			}

			return result.substr( index );
		}

		static size_t makeHash( FramePass const & pass
			, ImageViewId const & view )
		{
			if constexpr ( sizeof( size_t ) == sizeof( uint64_t ) )
			{
				return size_t( pass.id ) << 32u
					| size_t( view.id );
			}
			else
			{
				return size_t( pass.id ) << 16u
					| size_t( view.id );
			}
		}

		static size_t makeHash( FramePass const & pass
			, Buffer const & buffer )
		{
			if constexpr ( sizeof( size_t ) == sizeof( uint64_t ) )
			{
				return size_t( pass.id ) << 32u
					| ( ptrdiff_t( buffer.buffer() ) & 0xFFFFFFFF );
			}
			else
			{
				return size_t( pass.id ) << 16u
					| ( ptrdiff_t( buffer.buffer() ) & 0x0000FFFF );
			}
		}

		static void mergeViewData( ImageViewId const & view
			, bool mergeMipLevels
			, bool mergeArrayLayers
			, ImageViewData & data )
		{
			if ( data.image.id == 0 )
			{
				data.image = view.data->image;
				data.name = data.image.data->name;
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
	}

	FramePass::FramePass( FramePassGroup const & pgroup
		, FrameGraph & pgraph
		, uint32_t pid
		, std::string const & pname
		, RunnablePassCreator prunnableCreator )
		: group{ pgroup }
		, graph{ pgraph }
		, id{ pid }
		, runnableCreator{ std::move( prunnableCreator ) }
		, m_name{ pname }
	{
	}

	bool FramePass::dependsOn( FramePass const & pass
		, ImageViewId const & view
		, PassDependencyCache & cache )const
	{
		auto & passCache = cache.try_emplace( this ).first->second;
		auto [rit, res] = passCache.emplace( fpass::makeHash( pass, view ), false );

		if ( res )
		{
			auto it = std::find_if( passDepends.begin()
				, passDepends.end()
				, [&pass, &view, &cache]( FramePass const * lookup )
				{
					bool result = false;

					if ( fpass::isInOutputs( *lookup, view ) )
					{
						result = ( pass.id == lookup->id );
					}
					else if ( !fpass::isInInputs( *lookup, view ) )
					{
						result = lookup->dependsOn( pass, view, cache );
					}

					return result;
				} );
			rit->second = it != passDepends.end();
		}

		return rit->second;
	}

	bool FramePass::dependsOn( FramePass const & pass
		, Buffer const & buffer
		, PassDependencyCache & cache )const
	{
		auto & passCache = cache.try_emplace( this ).first->second;
		auto [rit, res] = passCache.emplace( fpass::makeHash( pass, buffer ), false );

		if ( res )
		{
			auto it = std::find_if( passDepends.begin()
				, passDepends.end()
				, [&pass, &buffer, &cache]( FramePass const * lookup )
				{
					bool result = false;

					if ( fpass::isInOutputs( *lookup, buffer ) )
					{
						result = ( pass.id == lookup->id );
					}
					else if ( !fpass::isInInputs( *lookup, buffer ) )
					{
						result = lookup->dependsOn( pass, buffer, cache );
					}

					return result;
				} );
			rit->second = it != passDepends.end();
		}

		return rit->second;
	}

	bool FramePass::dependsOn( FramePass const & pass )const
	{
		auto it = std::find_if( passDepends.begin()
			, passDepends.end()
			, [&pass]( FramePass const * lookup )
			{
				return pass.id == lookup->id;
			} );
		return it != passDepends.end();
	}

	void FramePass::addImplicitBuffer( Buffer buffer
		, DeviceSize offset
		, DeviceSize range
		, AccessState wantedAccess )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/ImplB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transition )
			, std::move( buffer )
			, offset
			, range
			, std::move( wantedAccess ) } );
	}

	void FramePass::addUniformBuffer( Buffer buffer
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/UB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Uniform )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addInputStorageBuffer( Buffer buffer
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/ISB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addOutputStorageBuffer( Buffer buffer
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/OSB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addClearableOutputStorageBuffer( Buffer buffer
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/OSB";
		buffers.push_back( Attachment{ ( Attachment::FlagKind( Attachment::Flag::Output )
				| Attachment::FlagKind( Attachment::Flag::Clearable ) )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addInOutStorageBuffer( Buffer buffer
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/IOSB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::InOut )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addImplicitBufferView( Buffer buffer
		, VkBufferView view
		, DeviceSize offset
		, DeviceSize range
		, AccessState wantedAccess )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/ImplBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::TransitionView )
			, std::move( buffer )
			, view
			, offset
			, range
			, std::move( wantedAccess ) } );
	}

	void FramePass::addUniformBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/UBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::UniformView )
			, std::move( buffer )
			, view
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addInputStorageBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/ISBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::StorageView )
			, std::move( buffer )
			, view
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addOutputStorageBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/OSBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::StorageView )
			, std::move( buffer )
			, view
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addClearableOutputStorageBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/OSBV";
		buffers.push_back( Attachment{ ( Attachment::FlagKind( Attachment::Flag::Output )
				| Attachment::FlagKind( Attachment::Flag::Clearable ) )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::StorageView )
			, std::move( buffer )
			, view
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addInOutStorageBufferView( Buffer buffer
		, VkBufferView view
		, uint32_t binding
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/IOSBV";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::InOut )
			, *this
			, binding
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::StorageView )
			, std::move( buffer )
			, view
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addTransferInputBuffer( Buffer buffer
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/ITB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addTransferOutputBuffer( Buffer buffer
		, DeviceSize offset
		, DeviceSize range )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/OTB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	void FramePass::addTransferInOutBuffer( Buffer buffer
		, DeviceSize offset
		, DeviceSize range
		, Attachment::Flag flag )
	{
		auto attachName = fpass::adjustName( *this, buffer.name ) + "/IOTB";
		buffers.push_back( Attachment{ Attachment::FlagKind( Attachment::FlagKind( Attachment::Flag::InOut ) | Attachment::FlagKind( flag ) )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
			, std::move( buffer )
			, offset
			, range
			, AccessState{} } );
	}

	ImageViewId FramePass::mergeViews( ImageViewIdArray const & views
		, bool mergeMipLevels
		, bool mergeArrayLayers )
	{
		ImageViewData data;
		for ( auto & view : views )
			fpass::mergeViewData( view, mergeMipLevels, mergeArrayLayers, data );

		if ( data.info.subresourceRange.layerCount > 1u )
		{
			switch ( data.info.viewType )
			{
			case ImageViewType::e1D:
				data.info.viewType = ImageViewType::e1DArray;
				break;
			case ImageViewType::e2D:
				if ( checkFlag( data.image.data->info.flags, ImageCreateFlags::eCubeCompatible )
					&& ( data.info.subresourceRange.layerCount % 6u ) == 0u
					&& data.info.subresourceRange.baseArrayLayer == 0u )
				{
					if ( data.info.subresourceRange.layerCount > 6u )
					{
						data.info.viewType = ImageViewType::eCubeArray;
					}
					else
					{
						data.info.viewType = ImageViewType::eCube;
					}
				}
				else
				{
					data.info.viewType = ImageViewType::e2DArray;
				}
				break;
			case ImageViewType::eCube:
				if ( data.info.subresourceRange.layerCount > 6u )
				{
					data.info.viewType = ImageViewType::eCubeArray;
				}
				break;
			default:
				break;
			}
		}

		return graph.createView( data );
	}

	void FramePass::addSampledView( ImageViewIdArray views
		, uint32_t binding
		, SamplerDesc samplerDesc )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Spl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Sampled )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, std::move( samplerDesc )
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eShaderReadOnly } );
	}

	void FramePass::addImplicitColourView( ImageViewIdArray views
		, ImageLayout wantedLayout )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Impl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transition )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, wantedLayout } );
	}

	void FramePass::addImplicitDepthView( ImageViewIdArray views
		, ImageLayout wantedLayout )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Impl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ( ImageAttachment::FlagKind( ImageAttachment::Flag::Transition )
				| ImageAttachment::FlagKind( ImageAttachment::Flag::Depth ) )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, wantedLayout } );
	}

	void FramePass::addImplicitDepthStencilView( ImageViewIdArray views
		, ImageLayout wantedLayout )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Impl";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ( ImageAttachment::FlagKind( ImageAttachment::Flag::Transition )
				| ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil ) )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, wantedLayout } );
	}

	void FramePass::addInputStorageView( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IStr";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral } );
	}

	void FramePass::addOutputStorageView( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/OStr";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral } );
	}

	void FramePass::addClearableOutputStorageView( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/COStr";
		images.push_back( { ( Attachment::FlagKind( Attachment::Flag::Output )
				| Attachment::FlagKind( Attachment::Flag::Clearable ) )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral } );
	}

	void FramePass::addInOutStorageView( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IOStr";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::InOut )
			, *this
			, binding
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral } );
	}

	void FramePass::addTransferInputView( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/It";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Input )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eTransferSrc } );
	}

	void FramePass::addTransferOutputView( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Ot";
		images.push_back( { Attachment::FlagKind( Attachment::Flag::Output )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eTransferDst } );
	}

	void FramePass::addTransferInOutView( ImageViewIdArray views
		, crg::Attachment::Flag flag )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IOt";
		images.push_back( { Attachment::FlagKind( Attachment::FlagKind( Attachment::Flag::InOut ) | Attachment::FlagKind( flag ) )
			, *this
			, InvalidBindingId
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, std::move( views )
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eTransferSrc } );
	}

	void FramePass::addColourView( std::string const & pname
		, crg::Attachment::FlagKind flags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, ImageLayout wantedLayout
		, ClearColorValue clearValue
		, PipelineColorBlendAttachmentState blendState )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/" + pname;
		images.push_back( { flags
			, *this
			, uint32_t{}
			, std::move( attachName )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, std::move( views )
			, loadOp
			, storeOp
			, AttachmentLoadOp::eDontCare
			, AttachmentStoreOp::eDontCare
			, SamplerDesc{}
			, ClearValue{ std::move( clearValue ) }
			, std::move( blendState )
			, wantedLayout } );
	}

	void FramePass::addDepthView( std::string const & pname
		, crg::Attachment::FlagKind flags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp
		, AttachmentStoreOp stencilStoreOp
		, ImageLayout wantedLayout
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/" + pname;
		images.insert( images.begin()
			, { flags
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, SamplerDesc{}
				, ClearValue{ std::move( clearValue ) }
				, PipelineColorBlendAttachmentState{}
				, wantedLayout } );
	}

	void FramePass::addStencilView( std::string const & pname
		, crg::Attachment::FlagKind flags
		, ImageAttachment::FlagKind stencilFlags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp
		, AttachmentStoreOp stencilStoreOp
		, ImageLayout wantedLayout
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/" + pname;
		images.insert( images.begin()
			, { flags
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( stencilFlags
					| ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil ) )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, SamplerDesc{}
				, ClearValue{ std::move( clearValue ) }
				, PipelineColorBlendAttachmentState{}
				, wantedLayout } );
	}

	void FramePass::addDepthStencilView( std::string const & pname
		, crg::Attachment::FlagKind flags
		, ImageAttachment::FlagKind stencilFlags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp
		, AttachmentStoreOp stencilStoreOp
		, ImageLayout wantedLayout
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/" + pname;
		images.insert( images.begin()
			, { flags
				, *this
				, uint32_t{}
				, std::move( attachName )
				, ImageAttachment::FlagKind( stencilFlags
					| ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil ) )
				, std::move( views )
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, SamplerDesc{}
				, ClearValue{ std::move( clearValue ) }
				, PipelineColorBlendAttachmentState{}
				, wantedLayout } );
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext & context
		, RunnableGraph & pgraph )const
	{
		return runnableCreator( *this, context, pgraph );
	}

	std::string FramePass::getFullName()const
	{
		return group.getFullName() + "/" + getName();
	}

	std::string FramePass::getGroupName()const
	{
		return group.getName() + "/" + getName();
	}
}
