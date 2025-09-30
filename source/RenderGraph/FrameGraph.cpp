/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FrameGraph.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/FramePassGroup.hpp"
#include "RenderGraph/Hash.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"
#include "GraphBuilder.hpp"

#include <algorithm>

namespace crg
{
	namespace fgph
	{
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
				data.info.subresourceRange = getSubresourceRange( view );
			}
			else
			{
				assert( data.image == view.data->image );

				if ( mergeMipLevels )
				{
					auto maxLevel = std::max( data.info.subresourceRange.levelCount + data.info.subresourceRange.baseMipLevel
						, getSubresourceRange( view ).levelCount + getSubresourceRange( view ).baseMipLevel );
					data.info.subresourceRange.baseMipLevel = std::min( getSubresourceRange( view ).baseMipLevel
						, data.info.subresourceRange.baseMipLevel );
					data.info.subresourceRange.levelCount = maxLevel - data.info.subresourceRange.baseMipLevel;
				}
				else
				{
					data.info.subresourceRange.baseMipLevel = std::min( getSubresourceRange( view ).baseMipLevel
						, data.info.subresourceRange.baseMipLevel );
					data.info.subresourceRange.levelCount = 1u;
				}

				if ( mergeArrayLayers )
				{
					auto maxLayer = std::max( data.info.subresourceRange.layerCount + data.info.subresourceRange.baseArrayLayer
						, getSubresourceRange( view ).layerCount + getSubresourceRange( view ).baseArrayLayer );
					data.info.subresourceRange.baseArrayLayer = std::min( getSubresourceRange( view ).baseArrayLayer
						, data.info.subresourceRange.baseArrayLayer );
					data.info.subresourceRange.layerCount = maxLayer - data.info.subresourceRange.baseArrayLayer;
				}
				else
				{
					data.info.subresourceRange.baseArrayLayer = std::min( getSubresourceRange( view ).baseArrayLayer
						, data.info.subresourceRange.baseArrayLayer );
					data.info.subresourceRange.layerCount = 1u;
				}
			}

			data.source.push_back( view );
		}

		static void mergeViewData( BufferViewId const & view
			, BufferViewData & data )
		{
			if ( data.buffer.id == 0 )
			{
				data.buffer = view.data->buffer;
				data.name = data.buffer.data->name;
				data.info.format = view.data->info.format;
				data.info.subresourceRange = getSubresourceRange( view );
			}
			else
			{
				assert( data.buffer == view.data->buffer );
				auto maxUpperBound = std::max( data.info.subresourceRange.offset + data.info.subresourceRange.size
					, getSubresourceRange( view ).offset + getSubresourceRange( view ).size );
				auto minLowerBound = std::min( data.info.subresourceRange.offset
					, getSubresourceRange( view ).offset );
				data.info.subresourceRange.offset = minLowerBound;
				data.info.subresourceRange.size = maxUpperBound - minLowerBound;
			}

			data.source.push_back( view );
		}

		static size_t makeHash( AttachmentArray const & attachments
			, bool mergeMipLevels
			, bool mergeArrayLayers )
		{
			auto result = std::hash< bool >{}( mergeMipLevels );
			result = hashCombine( result, mergeArrayLayers );
			for ( auto attach : attachments )
				result = hashCombine( result, attach );
			return result;
		}

		static AttachmentPtr mergeAttachments( FrameGraph & graph
			, AttachmentArray const & attachments
			, uint32_t passCount
			, bool mergeMipLevels
			, bool mergeArrayLayers )
		{
			AttachmentPtr result;

			for ( uint32_t passIndex = 0u; passIndex < passCount; ++passIndex )
			{
				ImageViewIdArray views;
				for ( auto attach : attachments )
				{
					views.push_back( attach->view( passIndex ) );
					if ( !result )
					{
						result = std::make_unique< Attachment >( views.back(), *attach );
						result->imageAttach.views.clear();
						result->pass = nullptr;
					}

					result->source.emplace_back( attach, attach->pass, attach->imageAttach );
				}

				result->imageAttach.views.push_back( graph.mergeViews( views, mergeMipLevels, mergeArrayLayers ) );
			}

			return result;
		}

		static AttachmentPtr mergeAttachments( FrameGraph & graph
			, AttachmentArray const & attachments
			, uint32_t passCount )
		{
			AttachmentPtr result;

			for ( uint32_t passIndex = 0u; passIndex < passCount; ++passIndex )
			{
				BufferViewIdArray views;
				for ( auto attach : attachments )
				{
					views.push_back( attach->buffer( passIndex ) );
					if ( !result )
					{
						result = std::make_unique< Attachment >( views.back(), *attach );
						result->bufferAttach.buffers.clear();
						result->pass = nullptr;
					}

					result->source.emplace_back( attach, attach->pass, attach->bufferAttach );
				}

				result->bufferAttach.buffers.push_back( graph.mergeViews( views ) );
			}

			return result;
		}
	}

	FrameGraph::FrameGraph( ResourceHandler & handler
		, std::string name )
		: m_handler{ handler }
		, m_name{ std::move( name ) }
		, m_defaultGroup{ std::make_unique< FramePassGroup >( *this, 0u, m_name, FramePassGroup::Token{} ) }
		, m_finalState{ handler }
	{
	}

	FramePass & FrameGraph::createPass( std::string const & name
		, RunnablePassCreator runnableCreator )
	{
		return m_defaultGroup->createPass( name, std::move( runnableCreator ) );
	}

	FramePassGroup & FrameGraph::createPassGroup( std::string const & groupName )
	{
		return m_defaultGroup->createPassGroup( groupName );
	}

	BufferId FrameGraph::createBuffer( BufferData const & img )
	{
		auto result = m_handler.createBufferId( img );
		m_buffers.insert( result );
		return result;
	}

	BufferViewId FrameGraph::createView( BufferViewData const & view )
	{
		auto result = m_handler.createViewId( view );
		m_bufferViews.insert( result );
		return result;
	}

	ImageId FrameGraph::createImage( ImageData const & img )
	{
		auto result = m_handler.createImageId( img );
		m_images.insert( result );
		return result;
	}

	ImageViewId FrameGraph::createView( ImageViewData const & view )
	{
		auto result = m_handler.createViewId( view );
		m_imageViews.insert( result );
		return result;
	}

	ImageViewId FrameGraph::mergeViews( ImageViewIdArray const & views
		, bool mergeMipLevels
		, bool mergeArrayLayers )
	{
		ImageViewData data;
		for ( auto & view : views )
			fgph::mergeViewData( view, mergeMipLevels, mergeArrayLayers, data );

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
					data.info.viewType = ( data.info.subresourceRange.layerCount > 6u )
						? ImageViewType::eCubeArray
						: ImageViewType::eCube;
				else
					data.info.viewType = ImageViewType::e2DArray;
				break;
			case ImageViewType::eCube:
				if ( data.info.subresourceRange.layerCount > 6u )
					data.info.viewType = ImageViewType::eCubeArray;
				break;
			default:
				break;
			}
		}

		return createView( data );
	}

	BufferViewId FrameGraph::mergeViews( BufferViewIdArray const & views )
	{
		BufferViewData data;
		for ( auto & view : views )
			fgph::mergeViewData( view, data );
		return createView( data );
	}

	Attachment const * FrameGraph::mergeAttachments( AttachmentArray const & attachments
		, bool mergeMipLevels
		, bool mergeArrayLayers )
	{
		if ( attachments.empty() )
		{
			Logger::logWarning( "No attachments to merge" );
			return nullptr;
		}
		if ( attachments.size() == 1 )
		{
			Logger::logDebug( "Single attachment, nothing to merge" );
			return attachments.front();
		}

		auto allImages = std::all_of( attachments.begin(), attachments.end()
			, []( Attachment const * attach )
			{
				return attach->isImage();
			} );
		if ( auto allBuffers = std::all_of( attachments.begin(), attachments.end()
			, []( Attachment const * attach )
			{
				return attach->isBuffer();
			} );
			!allImages && !allBuffers )
		{
			Logger::logWarning( "Can only merge attachments of the same type" );
			CRG_Exception( "Can only merge attachments of the same type" );
		}

		auto passCount = allImages
			? attachments.front()->getViewCount()
			: attachments.front()->getBufferCount();

		if ( passCount == 0 )
		{
			Logger::logWarning( "Can't merge empty attachments" );
			CRG_Exception( "Can't merge empty attachments" );
		}

		if ( allImages )
		{
			if ( !std::all_of( attachments.begin(), attachments.end()
				, [passCount]( Attachment const * attach )
				{
					return attach->getViewCount() == passCount;
				} ) )
			{
				Logger::logWarning( "Can only merge attachments with the same pass count" );
				CRG_Exception( "Can only merge attachments with the same pass count" );
			}
		}
		else
		{
			if ( !std::all_of( attachments.begin(), attachments.end()
				, [passCount]( Attachment const * attach )
				{
					return attach->getBufferCount() == passCount;
				} ) )
			{
				Logger::logWarning( "Can only merge attachments with the same pass count" );
				CRG_Exception( "Can only merge attachments with the same pass count" );
			}
		}

		size_t hash = allImages
			? fgph::makeHash( attachments, mergeMipLevels, mergeArrayLayers )
			: fgph::makeHash( attachments, false, false );
		auto [it, inserted] = m_mergedAttachments.try_emplace( hash, nullptr );

		if ( inserted )
		{
			if ( allImages )
			{
				it->second = fgph::mergeAttachments( *this, attachments, passCount, mergeMipLevels, mergeArrayLayers );
			}
			else
			{
				it->second = fgph::mergeAttachments( *this, attachments, passCount );
			}

			it->second->initSources();
		}

		return it->second.get();
	}

	RunnableGraphPtr FrameGraph::compile( GraphContext & context )
	{
		FramePassArray passes;
		m_defaultGroup->listPasses( passes );

		if ( passes.empty() )
		{
			Logger::logWarning( "No FramePass registered." );
			CRG_Exception( "No FramePass registered." );
		}

		auto endPoints = builder::findEndPoints( passes );
		RootNode root{ *this };
		GraphNodePtrArray nodes;
		builder::buildGraph( endPoints, root, nodes, context.separateDepthStencilLayouts );
		return std::make_unique< RunnableGraph >( *this
			, std::move( nodes )
			, std::move( root )
			, context );
	}

	LayoutState FrameGraph::getFinalLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range )const
	{
		return m_finalState.getLayoutState( image, viewType, range );
	}

	LayoutState FrameGraph::getFinalLayoutState( ImageViewId view
		, uint32_t passIndex )const
	{
		if ( view.data->source.empty() )
		{
			return getFinalLayoutState( view.data->image
				, view.data->info.viewType
				, getSubresourceRange( view ) );
		}

		return getFinalLayoutState( view.data->source[passIndex], 0u );
	}

	AccessState const & FrameGraph::getFinalAccessState( BufferId buffer
		, BufferSubresourceRange const & range )const
	{
		return m_finalState.getAccessState( buffer, range );
	}

	AccessState const & FrameGraph::getFinalAccessState( BufferViewId view
		, uint32_t passIndex )const
	{
		if ( view.data->source.empty() )
		{
			return getFinalAccessState( view.data->buffer
				, getSubresourceRange( view ) );
		}

		return getFinalAccessState( view.data->source[passIndex], 0u );
	}

	void FrameGraph::addInput( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_inputs.setLayoutState( image
			, viewType
			, range
			, outputLayout );
	}

	void FrameGraph::addInput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addInput( view.data->image
			, view.data->info.viewType
			, getSubresourceRange( view )
			, outputLayout );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range )const
	{
		return m_inputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getInputLayoutState( ImageViewId view )const
	{
		return getInputLayoutState( view.data->image
			, view.data->info.viewType
			, getSubresourceRange( view ) );
	}

	void FrameGraph::addOutput( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_outputs.setLayoutState( image
			, viewType
			, range
			, outputLayout );
	}

	void FrameGraph::addOutput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addOutput( view.data->image
			, view.data->info.viewType
			, getSubresourceRange( view )
			, outputLayout );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range )const
	{
		return m_outputs.getLayoutState( image
			, viewType
			, range );
	}

	LayoutState FrameGraph::getOutputLayoutState( ImageViewId view )const
	{
		return getOutputLayoutState( view.data->image
			, view.data->info.viewType
			, getSubresourceRange( view ) );
	}

	LayerLayoutStatesMap const & FrameGraph::getOutputLayoutStates()const
	{
		return m_outputs.images;
	}

	void FrameGraph::registerFinalState( RecordContext const & context )
	{
		m_finalState = context;
	}
}
