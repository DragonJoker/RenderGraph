/*
See LICENSE file in root folder.
*/
#include "RenderGraph/ResourceHandler.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/BufferData.hpp"
#include "RenderGraph/BufferViewData.hpp"
#include "RenderGraph/Exception.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Hash.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <sstream>
#pragma warning( pop )

namespace crg
{
	using lock_type = std::unique_lock< std::mutex >;

	//*********************************************************************************************

	namespace reshdl
	{
		struct Quad
		{
			using Data = std::array< float, 2u >;
			struct Vertex
			{
				Data position;
				Data texture;
			};
		};

		static VkBufferCreateInfo convert( BufferData const & data )
		{
			return convert( data.info );
		}

		static VkBufferViewCreateInfo convert( BufferViewData const & data
			, VkBuffer buffer )
		{
			auto result = convert( data.info );
			result.buffer = buffer;
			return result;
		}

		static VkImageCreateInfo convert( ImageData const & data )
		{
			return convert( data.info );
		}

		static VkImageViewCreateInfo convert( ImageViewData const & data
			, VkImage image )
		{
			auto result = convert( data.info );
			result.image = image;
			return result;
		}

		static size_t makeHash( SamplerDesc const & samplerDesc )
		{
			auto result = std::hash< FilterMode >{}( samplerDesc.magFilter );
			result = hashCombine( result, samplerDesc.minFilter );
			result = hashCombine( result, samplerDesc.mipmapMode );
			result = hashCombine( result, samplerDesc.addressModeU );
			result = hashCombine( result, samplerDesc.addressModeV );
			result = hashCombine( result, samplerDesc.addressModeW );
			result = hashCombine( result, samplerDesc.mipLodBias );
			result = hashCombine( result, samplerDesc.minLod );
			result = hashCombine( result, samplerDesc.maxLod );
			return result;
		}

		static size_t makeHash( bool texCoords
			, Texcoord const & config )
		{
			size_t result{ ( ( texCoords ? 0x01u : 0x00u ) << 0u )
				| ( ( config.invertU ? 0x01u : 0x00u ) << 1u )
				| ( ( config.invertV ? 0x01u : 0x00u ) << 2u ) };
			return result;
		}
	}

	//*********************************************************************************************

	Buffer::Buffer( ResourceHandler & handler
		, GraphContext & context
		, BufferId bufferId
		, BufferMemory firstPage )noexcept
		: m_handler{ &handler }
		, m_context{ &context }
		, m_bufferId{ bufferId }
	{
		m_pages.emplace_back( std::move( firstPage ) );
	}

	DeviceSize Buffer::getPageSize()const noexcept
	{
		return m_bufferId.data->info.size;
	}

	DeviceSize Buffer::getMaxSize()const noexcept
	{
		return m_bufferId.data->maxPages * getPageSize();
	}

	DeviceSize Buffer::getAllocatedSize()const noexcept
	{
		return m_bufferId.data->allocatedPages * getPageSize();
	}

	uint32_t Buffer::getAllocatedPageCount()const noexcept
	{
		return uint32_t( m_pages.size() );
	}

	uint32_t Buffer::getMaxPageCount()const noexcept
	{
		return m_bufferId.data->maxPages;
	}

	void Buffer::resize( DeviceSize newSize )
	{
		if ( newSize > getMaxSize() )
		{
			Logger::logError( "Can't resize to a sizer larger than maximum allocatable size for buffer [" + m_bufferId.data->name + "]" );
		}

		m_neededSize = std::min( std::max( m_neededSize, newSize ), getMaxSize() );
	}

	void Buffer::update()
	{
		while ( m_neededSize > getAllocatedSize() )
		{
			m_pages.push_back( m_handler->createBufferMemory( *m_context, m_bufferId ) );
		}
	}

	//*********************************************************************************************

	Image::Image( ResourceHandler & handler
		, GraphContext & context
		, ImageId imageId
		, ImageMemory imageMemory )
		: m_handler{ &handler }
		, m_context{ &context }
		, m_imageId{ imageId }
		, m_imageMemory{ imageMemory }
	{
	}

	//*********************************************************************************************

	ResourceHandler::~ResourceHandler()noexcept
	{
		for ( auto const & [data, _] : m_bufferViews )
		{
			std::stringstream stream;
			stream << "Leaked [VkBufferView](" << data.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto const & [data, _] : m_buffers )
		{
			std::stringstream stream;
			stream << "Leaked [VkBuffer](" << data.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto const & [data, _] : m_imageViews )
		{
			std::stringstream stream;
			stream << "Leaked [VkImageView](" << data.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto const & [data, _] : m_images )
		{
			std::stringstream stream;
			stream << "Leaked [VkImage](" << data.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto const & [_, data] : m_samplers )
		{
			std::stringstream stream;
			stream << "Leaked [VkSampler](" << data.name << ")";
			Logger::logError( stream.str() );
		}
	}

	BufferId ResourceHandler::createBufferId( BufferData const & buf )
	{
		if ( buf.maxPages < 1u )
		{
			Logger::logError( "createBufferId - At least one page is needed to create a buffer" );
			CRG_Exception( "createBufferId - At least one page is needed to create a buffer" );
		}

		lock_type lock( m_buffersMutex );
		auto data = std::make_unique< BufferData >( buf );
		BufferId result{ uint32_t( m_bufferIds.size() + 1u ), data.get() };
		m_bufferIds.try_emplace( result, std::move( data ) );
		return result;
	}

	BufferViewId ResourceHandler::createViewId( BufferViewData const & view )
	{
		lock_type lock( m_bufferViewsMutex );
		auto it = std::find_if( m_bufferViewIds.begin()
			, m_bufferViewIds.end()
			, [&view]( BufferViewIdDataOwnerCont::value_type const & lookup )
			{
				return *lookup.second == view;
			} );
		BufferViewId result{};

		if ( it == m_bufferViewIds.end() )
		{
			auto data = std::make_unique< BufferViewData >( view );
			result = BufferViewId{ uint32_t( m_bufferViewIds.size() + 1u ), data.get() };
			m_bufferViewIds.try_emplace( result, std::move( data ) );
		}
		else
		{
			result = it->first;
		}

		return result;
	}

	ResourceHandler::CreatedT< Buffer > ResourceHandler::createBuffer( GraphContext & context
		, BufferId bufferId )
	{
		CreatedT< Buffer > result{};
		lock_type lock( m_buffersMutex );
		auto [it, ins] = m_buffers.try_emplace( bufferId, nullptr );

		if ( ins )
		{
			it->second = std::make_unique< Buffer >( *this, context, bufferId, createBufferMemory( context, bufferId ) );
			result.created = true;

			if ( bufferId.data->maxPages > 1u )
			{
				m_pagedBuffers.push_back( it->second.get() );
			}
		}

		result.resource = it->second.get();
		return result;
	}

	ResourceHandler::CreatedViewT< VkBufferView > ResourceHandler::createBufferView( GraphContext & context
		, BufferViewId view )
	{
		ResourceHandler::CreatedViewT< VkBufferView > result{};

		if ( view.data->buffer.data->maxPages > 1 )
		{
			Logger::logError( "Can't create a vertex buffer from a paged buffer" );
			return result;
		}

		if ( context.vkCreateBufferView )
		{
			lock_type lock( m_bufferViewsMutex );
			auto [it, ins] = m_bufferViews.try_emplace( view, VkBufferView{} );

			if ( ins )
			{
				auto buffer = createBuffer( context, view.data->buffer ).resource->getBuffer();
				auto createInfo = reshdl::convert( *view.data, buffer );
				auto res = context.vkCreateBufferView( context.device
					, &createInfo
					, context.allocator
					, &it->second );
				checkVkResult( res, "BufferView creation" );
				crgRegisterObjectName( context, view.data->name, it->second );
				result.view = it->second;
				result.created = true;
			}
			else
			{
				result.view = it->second;
			}
		}

		return result;
	}

	ImageId ResourceHandler::createImageId( ImageData const & img )
	{
		lock_type lock( m_imagesMutex );
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_imageIds.size() + 1u ), data.get() };
		m_imageIds.try_emplace( result, std::move( data ) );
		return result;
	}

	ImageViewId ResourceHandler::createViewId( ImageViewData const & view )
	{
		lock_type lock( m_imageViewsMutex );
		auto it = std::find_if( m_imageViewIds.begin()
			, m_imageViewIds.end()
			, [&view]( ImageViewIdDataOwnerCont::value_type const & lookup )
			{
				return *lookup.second == view;
			} );
		ImageViewId result{};

		if ( it == m_imageViewIds.end() )
		{
			auto data = std::make_unique< ImageViewData >( view );
			result = ImageViewId{ uint32_t( m_imageViewIds.size() + 1u ), data.get() };
			m_imageViewIds.try_emplace( result, std::move( data ) );
		}
		else
		{
			result = it->first;
		}

		return result;
	}

	ResourceHandler::CreatedT< Image > ResourceHandler::createImage( GraphContext & context
		, ImageId imageId )
	{
		CreatedT< Image > result{};
		lock_type lock( m_imagesMutex );
		auto [it, ins] = m_images.try_emplace( imageId, nullptr );

		if ( ins )
		{
			it->second = std::make_unique< Image >( *this, context, imageId, createImageMemory( context, imageId ) );
			result.created = true;
		}

		result.resource = it->second.get();
		return result;
	}

	ResourceHandler::CreatedViewT< VkImageView > ResourceHandler::createImageView( GraphContext & context
		, ImageViewId view )
	{
		ResourceHandler::CreatedViewT< VkImageView > result{};

		if ( context.vkCreateImageView )
		{
			lock_type lock( m_bufferViewsMutex );
			auto [it, ins] = m_imageViews.try_emplace( view, VkImageView{} );

			if ( ins )
			{
				auto image = createImage( context, view.data->image ).resource->getImage();
				auto createInfo = reshdl::convert( *view.data, image );
				auto res = context.vkCreateImageView( context.device
					, &createInfo
					, context.allocator
					, &it->second );
				checkVkResult( res, "ImageView creation" );
				crgRegisterObjectName( context, view.data->name, it->second );
				result.view = it->second;
				result.created = true;
			}
			else
			{
				result.view = it->second;
			}
		}

		return result;
	}

	VkSampler ResourceHandler::createSampler( GraphContext & context
		, std::string const & suffix
		, SamplerDesc const & samplerDesc )
	{
		VkSampler result{};

		if ( context.vkCreateSampler )
		{
			lock_type lock( m_samplersMutex );
			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, convert( samplerDesc.magFilter )
				, convert( samplerDesc.minFilter )
				, convert( samplerDesc.mipmapMode )
				, convert( samplerDesc.addressModeU )
				, convert( samplerDesc.addressModeV )
				, convert( samplerDesc.addressModeW )
				, samplerDesc.mipLodBias // mipLodBias
				, VK_FALSE // anisotropyEnable
				, 0.0f // maxAnisotropy
				, VK_FALSE // compareEnable
				, VK_COMPARE_OP_ALWAYS // compareOp
				, samplerDesc.minLod
				, samplerDesc.maxLod
				, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
				, VK_FALSE };
			auto res = context.vkCreateSampler( context.device
				, &createInfo
				, context.allocator
				, &result );
			auto & sampler = m_samplers.try_emplace( result, Sampler{ result, {} } ).first->second;
			checkVkResult( res, "Sampler creation" );
			sampler.name = "Sampler_" + suffix;
			crgRegisterObject( context, sampler.name, result );
		}

		return result;
	}

	VertexBuffer const * ResourceHandler::createQuadTriVertexBuffer( GraphContext & context
		, std::string const & suffix
		, bool texCoords
		, Texcoord const & config )
	{
		VertexBuffer * vertexBuffer{};
		lock_type lock( m_vertexBuffersMutex );

		if ( context.vkCreateBuffer )
		{
			auto bufferId = createBufferId( BufferData{ "QuadVertexMemory_" + suffix
				, BufferCreateFlags::eNone
				, 3u * sizeof( reshdl::Quad::Vertex )
				, BufferUsageFlags::eVertexBuffer
				, MemoryPropertyFlags::eHostVisible } );
			auto result = std::make_unique< VertexBuffer >( createViewId( BufferViewData{ "QuadVertexMemory_" + suffix
				, bufferId
				, { 0u, bufferId.data->info.size } } ) );
			vertexBuffer = result.get();
			auto buffer = createBuffer( context, vertexBuffer->buffer.data->buffer ).resource->m_pages[0];

			if ( context.device )
			{
				reshdl::Quad::Vertex * bufferData{};
				auto res = context.vkMapMemory( context.device
					, buffer.memory
					, 0u
					, VK_WHOLE_SIZE
					, 0u
					, reinterpret_cast< void ** >( &bufferData ) );
				checkVkResult( res, "Buffer memory mapping" );

				if ( bufferData )
				{
					auto rangeU = 1.0;
					auto minU = 0.0;
					auto maxU = minU + 2.0 * rangeU;
					auto rangeV = 1.0;
					auto minV = -rangeV;
					auto maxV = minV + 2.0 * rangeV;
					auto realMinU = float( config.invertU ? maxU : minU );
					auto realMaxU = float( config.invertU ? minU : maxU );
					auto realMinV = float( config.invertV ? maxV : minV );
					auto realMaxV = float( config.invertV ? minV : maxV );
					std::array<reshdl::Quad::Vertex, 3u > vertexData
						{ reshdl::Quad::Vertex{ reshdl::Quad::Data{ -1.0f, -3.0f }, reshdl::Quad::Data{ realMinU, realMinV } }
						, reshdl::Quad::Vertex{ reshdl::Quad::Data{ -1.0f, +1.0f }, reshdl::Quad::Data{ realMinU, realMaxV } }
						, reshdl::Quad::Vertex{ reshdl::Quad::Data{ +3.0f, +1.0f }, reshdl::Quad::Data{ realMaxU, realMaxV } } };
					std::copy( vertexData.begin(), vertexData.end(), bufferData );

					VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
						, nullptr
						, buffer.memory
						, 0u
						, VK_WHOLE_SIZE };
					context.vkFlushMappedMemoryRanges( context.device, 1u, &memoryRange );
					context.vkUnmapMemory( context.device, buffer.memory );
				}
			}

			vertexBuffer->vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( reshdl::Quad::Vertex, position ) } );

			if ( texCoords )
			{
				vertexBuffer->vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( reshdl::Quad::Vertex, texture ) } );
			}

			vertexBuffer->vertexBindings.push_back( { 0u, sizeof( reshdl::Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
			vertexBuffer->inputState = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( vertexBuffer->vertexBindings.size() )
				, vertexBuffer->vertexBindings.data()
				, uint32_t( vertexBuffer->vertexAttribs.size() )
				, vertexBuffer->vertexAttribs.data() };

			m_vertexBuffers.emplace( std::move( result ) );
		}

		return vertexBuffer;
	}

	void ResourceHandler::destroyBuffer( GraphContext & context
		, BufferId bufferId )
	{
		lock_type lock( m_buffersMutex );
		auto it = m_buffers.find( bufferId );

		if ( it != m_buffers.end() )
		{
			if ( bufferId.data->maxPages > 1u )
			{
				if ( auto itPaged = std::find( m_pagedBuffers.begin(), m_pagedBuffers.end(), it->second.get() );
					itPaged != m_pagedBuffers.end() )
				{
					m_pagedBuffers.erase( itPaged );
				}
			}

			for ( uint32_t pageIndex = 0u; pageIndex < it->second->getAllocatedPageCount(); ++pageIndex )
			{
				auto page = it->second->getPage( pageIndex );

				if ( context.vkDestroyBuffer && page.buffer )
				{
					context.vkDestroyBuffer( context.device, page.buffer, context.allocator );
				}

				if ( context.vkFreeMemory && page.memory )
				{
					context.vkFreeMemory( context.device, page.memory, context.allocator );
				}
			}

			m_buffers.erase( it );
		}
	}

	void ResourceHandler::destroyBufferView( GraphContext & context
		, BufferViewId viewId )
	{
		lock_type lock( m_bufferViewsMutex );
		auto it = m_bufferViews.find( viewId );

		if ( it != m_bufferViews.end() )
		{
			if ( context.vkDestroyBufferView && it->second )
			{
				context.vkDestroyBufferView( context.device, it->second, context.allocator );
			}

			m_bufferViews.erase( it );
		}
	}

	void ResourceHandler::destroyImage( GraphContext & context
		, ImageId imageId )
	{
		lock_type lock( m_imagesMutex );
		auto it = m_images.find( imageId );

		if ( it != m_images.end() )
		{
			if ( auto image = it->second->getImage();
				context.vkDestroyImage && image )
			{
				context.vkDestroyImage( context.device, image, context.allocator );
			}

			if ( auto memory = it->second->getMemory();
				context.vkFreeMemory && memory )
			{
				context.vkFreeMemory( context.device, memory, context.allocator );
			}

			m_images.erase( it );
		}
	}

	void ResourceHandler::destroyImageView( GraphContext & context
		, ImageViewId viewId )
	{
		lock_type lock( m_bufferViewsMutex );
		auto it = m_imageViews.find( viewId );

		if ( it != m_imageViews.end() )
		{
			if ( context.vkDestroyImageView && it->second )
			{
				context.vkDestroyImageView( context.device, it->second, context.allocator );
			}

			m_imageViews.erase( it );
		}
	}

	void ResourceHandler::destroySampler( GraphContext & context
		, VkSampler sampler )
	{
		lock_type lock( m_samplersMutex );
		auto it = m_samplers.find( sampler );

		if ( it != m_samplers.end() )
		{
			if ( context.vkDestroySampler && it->first )
			{
				crgUnregisterObject( context, it->first );
				context.vkDestroySampler( context.device
					, it->first
					, context.allocator );
			}

			m_samplers.erase( it );
		}
	}

	void ResourceHandler::destroyVertexBuffer( GraphContext & context
		, VertexBuffer const * buffer )
	{
		lock_type lock( m_vertexBuffersMutex );
		auto it = std::find_if( m_vertexBuffers.begin()
			, m_vertexBuffers.end()
			, [buffer]( VertexBufferPtr const & lookup )
			{
				return lookup.get() == buffer;
			} );

		if ( it != m_vertexBuffers.end() )
		{
			auto const & vertexBuffer = **it;
			destroyBuffer( context, vertexBuffer.buffer.data->buffer );
			m_vertexBuffers.erase( it );
		}
	}

	BufferMemory ResourceHandler::createBufferMemory( GraphContext & context
		, BufferId bufferId )
	{
		auto itId = m_bufferIds.find( bufferId );

		if ( itId == m_bufferIds.end() )
		{
			Logger::logError( "Can't create a buffer from a buffer ID not handled by this resource handler" );
			CRG_Exception( "Can't create a buffer from a buffer ID not handled by this resource handler" );
		}

		BufferMemory result{};

		if ( context.vkCreateBuffer
			&& context.vkGetBufferMemoryRequirements
			&& context.vkAllocateMemory
			&& context.vkBindBufferMemory )
		{
			// Create buffer
			auto createInfo = reshdl::convert( *bufferId.data );
			auto res = context.vkCreateBuffer( context.device
				, &createInfo
				, context.allocator
				, &result.buffer );
			checkVkResult( res, "Buffer creation" );
			crgRegisterObjectName( context, bufferId.data->name, result.buffer );

			// Create Buffer memory
			VkMemoryRequirements requirements{};
			context.vkGetBufferMemoryRequirements( context.device
				, result.buffer
				, &requirements );
			uint32_t deduced = context.deduceMemoryType( requirements.memoryTypeBits
				, getMemoryPropertyFlags( bufferId.data->info.memory ) );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = context.vkAllocateMemory( context.device
				, &allocateInfo
				, context.allocator
				, &result.memory );
			checkVkResult( res, "Buffer memory allocation" );
			crgRegisterObjectName( context, bufferId.data->name, result.memory );

			// Bind buffer and memory
			res = context.vkBindBufferMemory( context.device
				, result.buffer
				, result.memory
				, 0u );
			checkVkResult( res, "Buffer memory binding" );

			// Update allocated pages count
			auto bufferData = itId->second.get();
			++bufferData->allocatedPages;
		}

		return result;
	}

	ImageMemory ResourceHandler::createImageMemory( GraphContext & context
		, ImageId imageId )
	{
		if ( auto itId = m_imageIds.find( imageId );
			itId == m_imageIds.end() )
		{
			Logger::logError( "Can't create an image from an image ID not handled by this resource handler" );
			CRG_Exception( "Can't create an image from an image ID not handled by this resource handler" );
		}

		ImageMemory result{};

		if ( context.vkCreateImage
			&& context.vkGetImageMemoryRequirements
			&& context.vkAllocateMemory
			&& context.vkBindImageMemory )
		{
			// Create image
			auto createInfo = reshdl::convert( *imageId.data );
			auto res = context.vkCreateImage( context.device
				, &createInfo
				, context.allocator
				, &result.image );
			checkVkResult( res, "Image creation" );
			crgRegisterObjectName( context, imageId.data->name, result.image );

			// Create Image memory
			VkMemoryRequirements requirements{};
			context.vkGetImageMemoryRequirements( context.device
				, result.image
				, &requirements );
			uint32_t deduced = context.deduceMemoryType( requirements.memoryTypeBits
				, getMemoryPropertyFlags( imageId.data->info.memory ) );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = context.vkAllocateMemory( context.device
				, &allocateInfo
				, context.allocator
				, &result.memory );
			checkVkResult( res, "Image memory allocation" );
			crgRegisterObjectName( context, imageId.data->name, result.memory );

			// Bind image and memory
			res = context.vkBindImageMemory( context.device
				, result.image
				, result.memory
				, 0u );
			checkVkResult( res, "Image memory binding" );
		}

		return result;
	}

	//*********************************************************************************************

	ContextResourcesCache::ContextResourcesCache( ResourceHandler & handler
		, GraphContext & context )
		: m_handler{ handler }
		, m_context{ context }
	{
	}

	ContextResourcesCache::~ContextResourcesCache()noexcept
	{
		for ( auto const & [bufferView, _] : m_bufferViews )
		{
			m_handler.destroyBufferView( m_context, bufferView );
		}

		for ( auto const & [buffer, _] : m_buffers )
		{
			m_handler.destroyBuffer( m_context, buffer );
		}

		for ( auto const & [imageView, _] : m_imageViews )
		{
			m_handler.destroyImageView( m_context, imageView );
		}

		for ( auto const & [image, _] : m_images )
		{
			m_handler.destroyImage( m_context, image );
		}

		for ( auto const & [_, sampler] : m_samplers )
		{
			m_handler.destroySampler( m_context, sampler );
		}

		for ( auto const & [_, buffer] : m_vertexBuffers )
		{
			m_handler.destroyVertexBuffer( m_context, buffer );
		}
	}

	Buffer & ContextResourcesCache::createBuffer( BufferId const & bufferId )
	{
		auto [created, buffer] = m_handler.createBuffer( m_context, bufferId );

		if ( created )
		{
			m_buffers.try_emplace( bufferId, buffer );
		}

		return *buffer;
	}

	VkBufferView ContextResourcesCache::createBufferView( BufferViewId const & view )
	{
		auto [created, result] = m_handler.createBufferView( m_context, view );

		if ( created )
		{
			m_bufferViews.try_emplace( view, result );
		}

		return result;
	}

	bool ContextResourcesCache::destroyBuffer( Buffer const & buffer )
	{
		return destroyBuffer( buffer.getBufferId() );
	}

	bool ContextResourcesCache::destroyBuffer( BufferId const & bufferId )
	{
		bool result{};

		if ( auto it = m_buffers.find( bufferId );
			it != m_buffers.end() )
		{
			result = true;
			m_handler.destroyBuffer( m_context, bufferId );
		}

		return result;
	}

	bool ContextResourcesCache::destroyBufferView( BufferViewId const & viewId )
	{
		auto it = m_bufferViews.find( viewId );
		auto result = it != m_bufferViews.end();

		if ( result )
		{
			m_handler.destroyBufferView( m_context, viewId );
		}

		return result;
	}

	Image & ContextResourcesCache::createImage( ImageId const & imageId )
	{
		auto [created, image] = m_handler.createImage( m_context, imageId );

		if ( created )
		{
			m_images.try_emplace( imageId, image );
		}

		return *image;
	}

	VkImageView ContextResourcesCache::createImageView( ImageViewId const & view )
	{
		auto [created, result] = m_handler.createImageView( m_context, view );

		if ( created )
		{
			m_imageViews[view] = result;
		}

		return result;
	}

	bool ContextResourcesCache::destroyImage( Image const & image )
	{
		return destroyImage( image.getImageId() );
	}

	bool ContextResourcesCache::destroyImage( ImageId const & imageId )
	{
		auto it = m_images.find( imageId );
		auto result = it != m_images.end();

		if ( result )
		{
			m_handler.destroyImage( m_context, imageId );
		}

		return result;
	}

	bool ContextResourcesCache::destroyImageView( ImageViewId const & viewId )
	{
		auto it = m_imageViews.find( viewId );
		auto result = it != m_imageViews.end();

		if ( result )
		{
			m_handler.destroyImageView( m_context, viewId );
		}

		return result;
	}

	VkSampler ContextResourcesCache::createSampler( SamplerDesc const & samplerDesc )
	{
		auto hash = reshdl::makeHash( samplerDesc );
		auto [it, res] = m_samplers.try_emplace( hash, VkSampler{} );

		if ( res )
		{
			it->second = m_handler.createSampler( m_context
				, std::to_string( hash )
				, samplerDesc );
		}

		return it->second;
	}

	VertexBuffer const & ContextResourcesCache::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		auto hash = reshdl::makeHash( texCoords, config );
		auto [it, res] = m_vertexBuffers.emplace( hash, nullptr );

		if ( res )
		{
			it->second = m_handler.createQuadTriVertexBuffer( m_context
				, std::to_string( hash )
				, texCoords
				, config );
		}

		return *it->second;
	}

	//*********************************************************************************************

	ResourcesCache::ResourcesCache( ResourceHandler & handler )
		: m_handler{ handler }
	{
	}

	void ResourcesCache::destroyContext( GraphContext & context )
	{
		if ( auto it = m_caches.find( &context );
			it != m_caches.end() )
		{
			m_caches.erase( it );
		}
	}

	Buffer & ResourcesCache::createBuffer( GraphContext & context
		, BufferId const & bufferId )
	{
		auto & cache = getContextCache( context );
		return cache.createBuffer( bufferId );
	}

	VkBufferView ResourcesCache::createBufferView( GraphContext & context
		, BufferViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.createBufferView( viewId );
	}

	bool ResourcesCache::destroyBuffer( BufferId const & bufferId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&bufferId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyBuffer( bufferId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyBufferView( BufferViewId const & viewId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&viewId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyBufferView( viewId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyBuffer( GraphContext & context
		, BufferId const & bufferId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyBuffer( bufferId );
	}

	bool ResourcesCache::destroyBufferView( GraphContext & context
		, BufferViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyBufferView( viewId );
	}

	Image & ResourcesCache::createImage( GraphContext & context
		, ImageId const & imageId )
	{
		auto & cache = getContextCache( context );
		return cache.createImage( imageId );
	}

	VkImageView ResourcesCache::createImageView( GraphContext & context
		, ImageViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.createImageView( viewId );
	}

	bool ResourcesCache::destroyImage( ImageId const & imageId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&imageId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyImage( imageId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyImageView( ImageViewId const & viewId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&viewId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyImageView( viewId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyImage( GraphContext & context
		, ImageId const & imageId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyImage( imageId );
	}

	bool ResourcesCache::destroyImageView( GraphContext & context
		, ImageViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyImageView( viewId );
	}

	VkSampler ResourcesCache::createSampler( GraphContext & context
		, SamplerDesc const & samplerDesc )
	{
		auto & cache = getContextCache( context );
		return cache.createSampler( samplerDesc );
	}

	VertexBuffer const & ResourcesCache::createQuadTriVertexBuffer( GraphContext & context
		, bool texCoords
		, Texcoord const & config )
	{
		auto & cache = getContextCache( context );
		return cache.createQuadTriVertexBuffer( texCoords, config );
	}

	ContextResourcesCache & ResourcesCache::getContextCache( GraphContext & context )
	{
		auto it = m_caches.find( &context );

		if ( it == m_caches.end() )
		{
			it = m_caches.try_emplace( &context, m_handler, context ).first;
		}

		return it->second;
	}

	//*********************************************************************************************
}
