/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphEnums.hpp"

#include <array>
#include <variant>

namespace crg
{
	struct Offset2D
	{
		int32_t x;
		int32_t y;

	private:
		friend bool operator==( Offset2D const & lhs, Offset2D const & rhs ) = default;
	};

	struct Extent2D
	{
		uint32_t width;
		uint32_t height;

	private:
		friend bool operator==( Extent2D const & lhs, Extent2D const & rhs ) = default;
	};

	struct Rect2D
	{
		Offset2D offset;
		Extent2D extent;

	private:
		friend bool operator==( Rect2D const & lhs, Rect2D const & rhs ) = default;
	};

	struct Offset3D
	{
		int32_t x;
		int32_t y;
		int32_t z;

	private:
		friend bool operator==( Offset3D const & lhs, Offset3D const & rhs ) = default;
	};

	struct Extent3D
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;

	private:
		friend bool operator==( Extent3D const & lhs, Extent3D const & rhs ) = default;
	};

	struct ImageSubresourceRange
	{
		ImageAspectFlags aspectMask;
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;

	private:
		friend bool operator==( ImageSubresourceRange const & lhs, ImageSubresourceRange const & rhs ) = default;
	};

	struct ImageCreateInfo
	{
		ImageCreateFlags flags;
		ImageType imageType;
		PixelFormat format;
		Extent3D extent;
		uint32_t mipLevels;
		uint32_t arrayLayers;
		SampleCount samples;
		ImageTiling tiling;
		ImageUsageFlags usage;

	private:
		friend bool operator==( ImageCreateInfo const & lhs, ImageCreateInfo const & rhs ) = default;
	};

	struct ImageViewCreateInfo
	{
		ImageViewCreateFlags flags;
		ImageViewType viewType;
		PixelFormat format;
		ImageSubresourceRange subresourceRange;

	private:
		friend bool operator==( ImageViewCreateInfo const & lhs, ImageViewCreateInfo const & rhs ) = default;
	};

	struct ClearColorValue
	{
		enum ValueIndex
		{
			eFloat32,
			eInt32,
			eUInt32,
		};

		template< typename ValueT >
		constexpr ClearColorValue( ValueT r, ValueT g, ValueT b, ValueT a )noexcept
			: m_value{ std::array< ValueT, 4u >{ r, g, b, a } }
		{
		}

		constexpr explicit ClearColorValue( std::array< float, 4u > v = { 0.0f, 0.0f, 0.0f, 0.0f } )noexcept
			: m_value{ std::move( v ) }
		{
		}

		constexpr explicit ClearColorValue( std::array< int32_t, 4u > v )noexcept
			: m_value{ std::move( v ) }
		{
		}

		constexpr explicit ClearColorValue( std::array< uint32_t, 4u > v )noexcept
			: m_value{ std::move( v ) }
		{
		}

		constexpr bool isFloat32()const noexcept
		{
			return m_value.index() == eFloat32;
		}

		constexpr bool isInt32()const noexcept
		{
			return m_value.index() == eInt32;
		}

		constexpr bool isUInt32()const noexcept
		{
			return m_value.index() == eUInt32;
		}

		constexpr std::array< float, 4u > const & float32()const noexcept
		{
			return std::get< eFloat32 >( m_value );
		}

		constexpr std::array< int32_t, 4u > const & int32()const noexcept
		{
			return std::get< eInt32 >( m_value );
		}

		constexpr std::array< uint32_t, 4u > const & uint32()const noexcept
		{
			return std::get< eUInt32 >( m_value );
		}

	private:
		std::variant< std::array< float, 4u >
			, std::array< int32_t, 4u >
			, std::array< uint32_t, 4u > > m_value;

		friend bool operator==( ClearColorValue const & lhs, ClearColorValue const & rhs ) = default;
	};

	struct ClearDepthStencilValue
	{
		float depth;
		uint32_t stencil;

	private:
		friend bool operator==( ClearDepthStencilValue const & lhs, ClearDepthStencilValue const & rhs ) = default;
	};

	struct ClearValue
	{
		enum ValueIndex
		{
			eColor,
			eDepthStencil,
		};

		constexpr explicit ClearValue( ClearColorValue v = ClearColorValue{} )noexcept
			: m_value{ std::move( v ) }
		{
		}

		constexpr explicit ClearValue( ClearDepthStencilValue v )noexcept
			: m_value{ std::move( v ) }
		{
		}

		constexpr bool isColor()const noexcept
		{
			return m_value.index() == eColor;
		}

		constexpr bool isDepthStencil()const noexcept
		{
			return m_value.index() == eDepthStencil;
		}

		constexpr ClearColorValue const & color()const noexcept
		{
			return std::get< eColor >( m_value );
		}

		constexpr ClearDepthStencilValue const & depthStencil()const noexcept
		{
			return std::get< eDepthStencil >( m_value );
		}

	private:
		std::variant< ClearColorValue, ClearDepthStencilValue > m_value;

		friend bool operator==( ClearValue const & lhs, ClearValue const & rhs ) = default;
	};

	struct PipelineColorBlendAttachmentState
	{
		uint32_t blendEnable{ VK_FALSE };
		BlendFactor srcColorBlendFactor{ BlendFactor::eOne };
		BlendFactor dstColorBlendFactor{ BlendFactor::eZero };
		BlendOp colorBlendOp{ BlendOp::eAdd };
		BlendFactor srcAlphaBlendFactor{ BlendFactor::eOne };
		BlendFactor dstAlphaBlendFactor{ BlendFactor::eZero };
		BlendOp alphaBlendOp{ BlendOp::eAdd };
		ColorComponentFlags colorWriteMask{ ColorComponentFlags::eR | ColorComponentFlags::eG | ColorComponentFlags::eB | ColorComponentFlags::eA };

	private:
		friend bool operator==( PipelineColorBlendAttachmentState const & lhs, PipelineColorBlendAttachmentState const & rhs ) = default;
	};

	struct PipelineState
	{
		AccessFlags access;
		PipelineStageFlags pipelineStage;
	};

	struct LayoutState
	{
		ImageLayout layout;
		PipelineState state;
	};

	template< typename VkTypeT >
	struct ContextObjectT
	{
		ContextObjectT( ContextObjectT const & rhs ) = delete;
		ContextObjectT( ContextObjectT && rhs )noexcept = delete;
		ContextObjectT & operator=( ContextObjectT const & rhs ) = delete;
		ContextObjectT & operator=( ContextObjectT && rhs )noexcept = delete;

		explicit ContextObjectT( GraphContext & ctx
			, VkTypeT obj = {}
			, void( *dtor )( GraphContext &, VkTypeT & )noexcept = nullptr )
			: context{ ctx }
			, object{ obj }
			, destroy{ dtor }
		{
		}

		~ContextObjectT()noexcept
		{
			if ( destroy && object )
			{
				destroy( context, object );
			}
		}

		GraphContext & context;
		VkTypeT object;
		void ( *destroy )( GraphContext &, VkTypeT & )noexcept;
	};

	struct Buffer
	{
		std::string name;

		Buffer( VkBufferArray pbuffers
			, std::string pname )noexcept
			: name{ std::move( pname ) }
			, m_buffers{ std::move( pbuffers ) }
		{
		}

		Buffer( VkBuffer buffer
			, std::string name )noexcept
			: Buffer{ VkBufferArray{ buffer }, std::move( name ) }
		{
		}

		VkBuffer const & buffer( uint32_t index = 0 )const noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		VkBuffer & buffer( uint32_t index = 0 )noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		size_t getCount()const noexcept
		{
			return m_buffers.size();
		}

	private:
		VkBufferArray m_buffers;

		friend CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );
	};

	struct VertexBuffer
	{
		VertexBuffer( VertexBuffer const & rhs ) = delete;
		VertexBuffer & operator=( VertexBuffer const & rhs ) = delete;
		~VertexBuffer()noexcept = default;

		explicit VertexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{}
			, VkVertexInputAttributeDescriptionArray pvertexAttribs = {}
			, VkVertexInputBindingDescriptionArray pvertexBindings = {} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
			, vertexAttribs{ std::move( pvertexAttribs ) }
			, vertexBindings{ std::move( pvertexBindings ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer( VertexBuffer && rhs )noexcept
			: buffer{ rhs.buffer }
			, memory{ rhs.memory }
			, vertexAttribs{ std::move( rhs.vertexAttribs ) }
			, vertexBindings{ std::move( rhs.vertexBindings ) }
			, inputState{ std::move( rhs.inputState ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer & operator=( VertexBuffer && rhs )noexcept
		{
			buffer = rhs.buffer;
			memory = rhs.memory;
			vertexAttribs = std::move( rhs.vertexAttribs );
			vertexBindings = std::move( rhs.vertexBindings );
			inputState = std::move( rhs.inputState );

			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();

			return *this;
		}

		Buffer buffer{ VkBuffer{}, std::string{} };
		VkDeviceMemory memory{};
		VkVertexInputAttributeDescriptionArray vertexAttribs{};
		VkVertexInputBindingDescriptionArray vertexBindings{};
		VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, {}, {}, {}, {}, {} };
	};

	struct IndexBuffer
	{
		explicit IndexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
		{
		}

		Buffer buffer;
		VkDeviceMemory memory;
	};

	struct IndirectBuffer
	{
		explicit IndirectBuffer( Buffer pbuffer
			, uint32_t pstride
			, DeviceSize poffset = {} )
			: buffer{ std::move( pbuffer ) }
			, offset{ poffset }
			, stride{ pstride }
		{
		}

		Buffer buffer;
		DeviceSize offset;
		uint32_t stride;
	};

	struct SemaphoreWait
	{
		VkSemaphore semaphore;
		PipelineStageFlags dstStageMask;
	};

	template< typename TypeT >
	struct RawTyperT
	{
		using Type = TypeT;
	};
}
