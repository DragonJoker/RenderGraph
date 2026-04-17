/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/BufferData.hpp"
#include "RenderGraph/ImageData.hpp"

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <mutex>
#pragma warning( pop )
#include <unordered_map>
#include <unordered_set>

namespace crg
{
	struct Sampler
	{
		VkSampler sampler;
		std::string name;
	};

	class Buffer
	{
		friend class ResourceHandler;

	public:
		Buffer( Buffer const & ) = delete;
		Buffer( Buffer && )noexcept = delete;
		Buffer & operator=( Buffer const & ) = delete;
		Buffer & operator=( Buffer && )noexcept = delete;
		~Buffer()noexcept = default;

		CRG_API Buffer( ResourceHandler & handler
			, GraphContext & context
			, BufferId bufferId
			, BufferMemory firstPage )noexcept;

		CRG_API DeviceSize getPageSize()const noexcept;
		CRG_API DeviceSize getMaxSize()const noexcept;
		CRG_API DeviceSize getAllocatedSize()const noexcept;
		CRG_API uint32_t getAllocatedPageCount()const noexcept;
		CRG_API uint32_t getMaxPageCount()const noexcept;
		CRG_API void resize( DeviceSize newSize );
		CRG_API void update();

		VkBuffer getBuffer( uint32_t pageIndex = 0u )const noexcept
		{
			return m_pages[pageIndex].buffer;
		}

		BufferMemory getPage( uint32_t pageIndex = 0u )const noexcept
		{
			return m_pages[pageIndex];
		}

		BufferId getBufferId()const noexcept
		{
			return m_bufferId;
		}

	private:
		friend bool operator==( Buffer const & lhs, Buffer const & rhs ) = default;

	private:
		ResourceHandler * m_handler{};
		GraphContext * m_context{};
		BufferId m_bufferId{};
		std::vector< BufferMemory > m_pages;
		DeviceSize m_neededSize{};
	};

	class Image
	{
	public:
		Image( Image const & ) = delete;
		Image( Image && )noexcept = delete;
		Image & operator=( Image const & ) = delete;
		Image & operator=( Image && )noexcept = delete;
		~Image()noexcept = default;

		CRG_API Image( ResourceHandler & handler
			, GraphContext & context
			, ImageId imageId
			, ImageMemory imageMemory );

		VkImage getImage()const noexcept
		{
			return m_imageMemory.image;
		}

		VkDeviceMemory getMemory()const noexcept
		{
			return m_imageMemory.memory;
		}

		ImageId getImageId()const noexcept
		{
			return m_imageId;
		}

	private:
		friend bool operator==( Image const & lhs, Image const & rhs ) = default;

	private:
		ResourceHandler * m_handler{};
		GraphContext * m_context{};
		ImageId m_imageId{};
		ImageMemory m_imageMemory{};
	};

	class ResourceHandler
	{
	private:
		friend class Buffer;

		template< typename DataT >
		using IdDataOwnerCont = std::map< Id< DataT >, std::unique_ptr< DataT > >;

		using BufferIdDataOwnerCont = IdDataOwnerCont< BufferData >;
		using BufferViewIdDataOwnerCont = IdDataOwnerCont< BufferViewData >;
		using BufferPtr = std::unique_ptr< Buffer >;
		using BufferMap = std::map< BufferId, BufferPtr >;
		using BufferViewMap = std::map< BufferViewId, VkBufferView >;

		using ImageIdDataOwnerCont = IdDataOwnerCont< ImageData >;
		using ImageViewIdDataOwnerCont = IdDataOwnerCont< ImageViewData >;
		using ImagePtr = std::unique_ptr< Image >;
		using ImageMap = std::map< ImageId, ImagePtr >;
		using ImageViewMap = std::map< ImageViewId, VkImageView >;

		template< typename ValueT >
		struct CreatedT
		{
			bool created{};
			ValueT * resource{};
		};

		template< typename ValueT >
		struct CreatedViewT
		{
			bool created{};
			ValueT view{};
		};

	public:
		ResourceHandler( ResourceHandler const & ) = delete;
		ResourceHandler( ResourceHandler && )noexcept = delete;
		ResourceHandler & operator=( ResourceHandler const & ) = delete;
		ResourceHandler & operator=( ResourceHandler && )noexcept = delete;
		ResourceHandler() = default;
		CRG_API ~ResourceHandler()noexcept;

		CRG_API BufferId createBufferId( BufferData const & img );
		CRG_API BufferViewId createViewId( BufferViewData const & view );
		CRG_API ImageId createImageId( ImageData const & img );
		CRG_API ImageViewId createViewId( ImageViewData const & view );

		CRG_API CreatedT< Buffer > createBuffer( GraphContext & context
			, BufferId bufferId );
		CRG_API CreatedViewT< VkBufferView > createBufferView( GraphContext & context
			, BufferViewId viewId );
		CRG_API CreatedT< Image > createImage( GraphContext & context
			, ImageId imageId );
		CRG_API CreatedViewT< VkImageView > createImageView( GraphContext & context
			, ImageViewId viewId );
		CRG_API VkSampler createSampler( GraphContext & context
			, std::string const & suffix
			, SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const * createQuadTriVertexBuffer( GraphContext & context
			, std::string const & suffix
			, bool texCoords
			, Texcoord const & config );
		CRG_API void destroyBuffer( GraphContext & context
			, BufferId bufferId );
		CRG_API void destroyBufferView( GraphContext & context
			, BufferViewId viewId );
		CRG_API void destroyImage( GraphContext & context
			, ImageId imageId );
		CRG_API void destroyImageView( GraphContext & context
			, ImageViewId viewId );
		CRG_API void destroySampler( GraphContext & context
			, VkSampler sampler );
		CRG_API void destroyVertexBuffer( GraphContext & context
			, VertexBuffer const * buffer );

		std::vector< Buffer * > const & getPagedBuffers()const noexcept
		{
			return m_pagedBuffers;
		}

	private:
		BufferMemory createBufferMemory( GraphContext & context, BufferId bufferId );
		ImageMemory createImageMemory( GraphContext & context, ImageId imageId );

		mutable std::mutex m_buffersMutex;
		BufferIdDataOwnerCont m_bufferIds;
		mutable std::mutex m_bufferViewsMutex;
		BufferViewIdDataOwnerCont m_bufferViewIds;
		BufferMap m_buffers;
		BufferViewMap m_bufferViews;
		mutable std::mutex m_imagesMutex;
		ImageIdDataOwnerCont m_imageIds;
		mutable std::mutex m_imageViewsMutex;
		ImageViewIdDataOwnerCont m_imageViewIds;
		ImageMap m_images;
		ImageViewMap m_imageViews;
		std::mutex m_samplersMutex;
		std::unordered_map< VkSampler, Sampler > m_samplers;
		std::mutex m_vertexBuffersMutex;
		std::unordered_set< VertexBufferPtr > m_vertexBuffers;
		std::vector< Buffer * > m_pagedBuffers;
	};

	class ContextResourcesCache
	{
		friend class Buffer;

	public:
		ContextResourcesCache( ContextResourcesCache const & ) = delete;
		ContextResourcesCache & operator=( ContextResourcesCache const & ) = delete;
		ContextResourcesCache( ContextResourcesCache && )noexcept = delete;
		ContextResourcesCache & operator=( ContextResourcesCache && )noexcept = delete;

		CRG_API ContextResourcesCache( ResourceHandler & handler
			, GraphContext & context );
		CRG_API ~ContextResourcesCache()noexcept;

		CRG_API Buffer & createBuffer( BufferId const & bufferId );
		CRG_API VkBufferView createBufferView( BufferViewId const & viewId );
		CRG_API bool destroyBuffer( Buffer const & buffer );
		CRG_API bool destroyBuffer( BufferId const & bufferId );
		CRG_API bool destroyBufferView( BufferViewId const & viewId );

		CRG_API Image & createImage( ImageId const & imageId );
		CRG_API VkImageView createImageView( ImageViewId const & viewId );
		CRG_API bool destroyImage( Image const & image );
		CRG_API bool destroyImage( ImageId const & imageId );
		CRG_API bool destroyImageView( ImageViewId const & viewId );

		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );

		GraphContext * operator->()const noexcept
		{
			return &m_context;
		}

		GraphContext & getContext()const noexcept
		{
			return m_context;
		}

		ResourceHandler & getHandler()const
		{
			return m_handler;
		}

	private:
		using BufferIdMap = std::map< BufferId, Buffer * >;
		using VkBufferViewIdMap = std::map< BufferViewId, VkBufferView >;
		using ImageIdMap = std::map< ImageId, Image * >;
		using VkImageViewIdMap = std::map< ImageViewId, VkImageView >;

		ResourceHandler & m_handler;
		GraphContext & m_context;
		BufferIdMap m_buffers;
		VkBufferViewIdMap m_bufferViews;
		ImageIdMap m_images;
		VkImageViewIdMap m_imageViews;
		std::unordered_map< size_t, VkSampler > m_samplers;
		std::unordered_map< size_t, VertexBuffer const * > m_vertexBuffers;
	};

	class ResourcesCache
	{
	public:
		CRG_API explicit ResourcesCache( ResourceHandler & handler );

		CRG_API void destroyContext( GraphContext & context );

		CRG_API Buffer & createBuffer( GraphContext & context
			, BufferId const & bufferId );
		CRG_API VkBufferView createBufferView( GraphContext & context
			, BufferViewId const & viewId );
		CRG_API bool destroyBuffer( BufferId const & bufferId );
		CRG_API bool destroyBufferView( BufferViewId const & viewId );
		CRG_API bool destroyBuffer( GraphContext & context
			, BufferId const & bufferId );
		CRG_API bool destroyBufferView( GraphContext & context
			, BufferViewId const & viewId );

		CRG_API Image & createImage( GraphContext & context
			, ImageId const & imageId );
		CRG_API VkImageView createImageView( GraphContext & context
			, ImageViewId const & viewId );
		CRG_API bool destroyImage( ImageId const & imageId );
		CRG_API bool destroyImageView( ImageViewId const & viewId );
		CRG_API bool destroyImage( GraphContext & context
			, ImageId const & imageId );
		CRG_API bool destroyImageView( GraphContext & context
			, ImageViewId const & viewId );

		CRG_API VkSampler createSampler( GraphContext & context
			, SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( GraphContext & context
			, bool texCoords
			, Texcoord const & config );

		CRG_API ContextResourcesCache & getContextCache( GraphContext & context );

		ResourceHandler & getHandler()const
		{
			return m_handler;
		}

	private:
		using ContextCacheMap = std::unordered_map< GraphContext *, ContextResourcesCache >;

		ResourceHandler & m_handler;
		ContextCacheMap m_caches;
	};
}
