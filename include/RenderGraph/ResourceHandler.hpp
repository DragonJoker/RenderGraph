/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

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

	class ResourceHandler
	{
		template< typename ValueT >
		struct CreatedT
		{
			bool created{};
			ValueT resource{};
			VkDeviceMemory memory{};
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
		CRG_API ResourceHandler() = default;
		CRG_API ~ResourceHandler()noexcept;

		CRG_API BufferId createBufferId( BufferData const & img );
		CRG_API BufferViewId createViewId( BufferViewData const & view );
		CRG_API ImageId createImageId( ImageData const & img );
		CRG_API ImageViewId createViewId( ImageViewData const & view );

		CRG_API CreatedT< VkBuffer > createBuffer( GraphContext & context
			, BufferId bufferId );
		CRG_API CreatedViewT< VkBufferView > createBufferView( GraphContext & context
			, BufferViewId viewId );
		CRG_API CreatedT< VkImage > createImage( GraphContext & context
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

	private:
		mutable std::mutex m_buffersMutex;
		BufferIdDataOwnerCont m_bufferIds;
		mutable std::mutex m_bufferViewsMutex;
		BufferViewIdDataOwnerCont m_bufferViewIds;
		BufferMemoryMap m_buffers;
		BufferViewMap m_bufferViews;
		mutable std::mutex m_imagesMutex;
		ImageIdDataOwnerCont m_imageIds;
		mutable std::mutex m_imageViewsMutex;
		ImageViewIdDataOwnerCont m_imageViewIds;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
		std::mutex m_samplersMutex;
		std::unordered_map< VkSampler, Sampler > m_samplers;
		std::mutex m_vertexBuffersMutex;
		std::unordered_set< VertexBufferPtr > m_vertexBuffers;
	};

	class ContextResourcesCache
	{
	public:
		ContextResourcesCache( ContextResourcesCache const & ) = delete;
		ContextResourcesCache & operator=( ContextResourcesCache const & ) = delete;
		ContextResourcesCache( ContextResourcesCache && )noexcept = delete;
		ContextResourcesCache & operator=( ContextResourcesCache && )noexcept = delete;

		CRG_API ContextResourcesCache( ResourceHandler & handler
			, GraphContext & context );
		CRG_API ~ContextResourcesCache()noexcept;

		CRG_API VkBuffer createBuffer( BufferId const & bufferId );
		CRG_API VkBuffer createBuffer( BufferId const & bufferId, VkDeviceMemory & memory );
		CRG_API VkBufferView createBufferView( BufferViewId const & viewId );
		CRG_API bool destroyBuffer( BufferId const & imageId );
		CRG_API bool destroyBufferView( BufferViewId const & viewId );

		CRG_API VkImage createImage( ImageId const & imageId );
		CRG_API VkImage createImage( ImageId const & imageId, VkDeviceMemory & memory );
		CRG_API VkImageView createImageView( ImageViewId const & viewId );
		CRG_API bool destroyImage( ImageId const & imageId );
		CRG_API bool destroyImageView( ImageViewId const & viewId );

		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );

		GraphContext * operator->()const noexcept
		{
			return &m_context;
		}

		operator GraphContext & ()const noexcept
		{
			return m_context;
		}

		ResourceHandler & getHandler()const
		{
			return m_handler;
		}

	private:
		using VkBufferIdMap = std::map< BufferId, VkBuffer >;
		using VkBufferViewIdMap = std::map< BufferViewId, VkBufferView >;
		using VkImageIdMap = std::map< ImageId, VkImage >;
		using VkImageViewIdMap = std::map< ImageViewId, VkImageView >;

		ResourceHandler & m_handler;
		GraphContext & m_context;
		VkBufferIdMap m_buffers;
		VkBufferViewIdMap m_bufferViews;
		VkImageIdMap m_images;
		VkImageViewIdMap m_imageViews;
		std::unordered_map< size_t, VkSampler > m_samplers;
		std::unordered_map< size_t, VertexBuffer const * > m_vertexBuffers;
	};

	class ResourcesCache
	{
	public:
		CRG_API explicit ResourcesCache( ResourceHandler & handler );

		CRG_API VkBuffer createBuffer( GraphContext & context
			, BufferId const & bufferId );
		CRG_API VkBuffer createBuffer( GraphContext & context
			, BufferId const & bufferId
			, VkDeviceMemory & memory );
		CRG_API VkBufferView createBufferView( GraphContext & context
			, BufferViewId const & viewId );
		CRG_API bool destroyBuffer( BufferId const & bufferId );
		CRG_API bool destroyBufferView( BufferViewId const & viewId );
		CRG_API bool destroyBuffer( GraphContext & context
			, BufferId const & bufferId );
		CRG_API bool destroyBufferView( GraphContext & context
			, BufferViewId const & viewId );

		CRG_API VkImage createImage( GraphContext & context
			, ImageId const & imageId );
		CRG_API VkImage createImage( GraphContext & context
			, ImageId const & imageId
			, VkDeviceMemory & memory );
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
