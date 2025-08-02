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
		int32_t x{};
		int32_t y{};

	private:
		friend bool operator==( Offset2D const & lhs, Offset2D const & rhs ) = default;
	};

	struct Extent2D
	{
		uint32_t width{};
		uint32_t height{};

	private:
		friend bool operator==( Extent2D const & lhs, Extent2D const & rhs ) = default;
	};

	struct Rect2D
	{
		Offset2D offset{};
		Extent2D extent{};

	private:
		friend bool operator==( Rect2D const & lhs, Rect2D const & rhs ) = default;
	};

	struct Offset3D
	{
		int32_t x{};
		int32_t y{};
		int32_t z{};

	private:
		friend bool operator==( Offset3D const & lhs, Offset3D const & rhs ) = default;
	};

	struct Extent3D
	{
		uint32_t width{};
		uint32_t height{};
		uint32_t depth{};

	private:
		friend bool operator==( Extent3D const & lhs, Extent3D const & rhs ) = default;
	};

	struct Rect3D
	{
		Offset3D offset{};
		Extent3D extent{};

	private:
		friend bool operator==( Rect3D const & lhs, Rect3D const & rhs ) = default;
	};

	struct BufferSubresourceRange
	{
		DeviceSize offset{};
		DeviceSize size{};

	private:
		friend bool operator==( BufferSubresourceRange const & lhs, BufferSubresourceRange const & rhs ) = default;
	};

	struct ImageSubresourceRange
	{
		ImageAspectFlags aspectMask{};
		uint32_t baseMipLevel{};
		uint32_t levelCount{};
		uint32_t baseArrayLayer{};
		uint32_t layerCount{};

	private:
		friend bool operator==( ImageSubresourceRange const & lhs, ImageSubresourceRange const & rhs ) = default;
	};

	struct BufferCreateInfo
	{
		BufferCreateFlags flags{};
		DeviceSize size{};
		BufferUsageFlags usage{};
		MemoryPropertyFlags memory{};

	private:
		friend bool operator==( BufferCreateInfo const & lhs, BufferCreateInfo const & rhs ) = default;
	};

	struct BufferViewCreateInfo
	{
		PixelFormat format{};
		BufferSubresourceRange subresourceRange{};

	private:
		friend bool operator==( BufferViewCreateInfo const & lhs, BufferViewCreateInfo const & rhs ) = default;
	};

	struct ImageCreateInfo
	{
		ImageCreateFlags flags{};
		ImageType imageType{};
		PixelFormat format{};
		Extent3D extent{};
		uint32_t mipLevels{};
		uint32_t arrayLayers{};
		SampleCount samples{};
		ImageTiling tiling{};
		ImageUsageFlags usage{};
		MemoryPropertyFlags memory{};

	private:
		friend bool operator==( ImageCreateInfo const & lhs, ImageCreateInfo const & rhs ) = default;
	};

	struct ImageViewCreateInfo
	{
		ImageViewCreateFlags flags{};
		ImageViewType viewType{};
		PixelFormat format{};
		ImageSubresourceRange subresourceRange{};

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
		float depth{};
		uint32_t stencil{};

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
		AccessFlags access{};
		PipelineStageFlags pipelineStage{};

	private:
		friend bool operator==( PipelineState const & lhs, PipelineState const & rhs ) = default;
	};

	struct LayoutState
	{
		ImageLayout layout{};
		PipelineState state{};
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

	struct SemaphoreWait
	{
		VkSemaphore semaphore{};
		PipelineStageFlags dstStageMask{};
	};

	template< typename TypeT >
	struct RawTyperT
	{
		using Type = TypeT;
	};
}
