/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RunnablePass.hpp"

#include <array>

namespace crg
{
	namespace rq
	{
		/**
		*\brief
		*	Tells how the texture coordinates from the vertex buffer are built.
		*/
		struct Texcoord
		{
			/**
			*\brief
			*	Tells if the U coordinate of UV must be inverted, thus mirroring vertically the resulting image.
			*/
			bool invertU{ false };
			/**
			*\brief
			*	Tells if the U coordinate of UV must be inverted, thus mirroring horizontally the resulting image.
			*/
			bool invertV{ false };
		};

		template< template< typename ValueT > typename WrapperT >
		struct ConfigT;

		template<>
		struct ConfigT< std::optional >
		{
			rp::ConfigT< std::optional > baseConfig;
			std::optional< Texcoord > texcoordConfig;
			std::optional< VkExtent2D > renderSize;
			std::optional< VkOffset2D > renderPosition;
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			Texcoord texcoordConfig;
			VkExtent2D renderSize;
			VkOffset2D renderPosition;
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< rq::Texcoord >
	{
		static rq::Texcoord const & get()
		{
			static rq::Texcoord const result{ false, false };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< VkExtent2D >
	{
		static VkExtent2D const & get()
		{
			static VkExtent2D const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< VkOffset2D >
	{
		static VkOffset2D const & get()
		{
			static VkOffset2D const result{};
			return result;
		}
	};

	class RenderQuad
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

		struct Quad
		{
			using Data = std::array< float, 2u >;
			struct Vertex
			{
				Data position;
				Data texture;
			};

			Vertex vertex[6];
		};

	public:
		RenderQuad( RenderPass const & pass
			, GraphContext const & context
			, RunnableGraph const & graph
			, rq::Config config );
		~RenderQuad();
		/**
		*\copydoc crg::RunnablePass::registerPass
		*/
		void recordInto( VkCommandBuffer commandBuffer )const override;

	private:
		void doInitialise()override;
		void doCreateVertexBuffer();
		void doCreateVertexMemory();
		void doCreateRenderPass();
		void doCreatePipeline();
		void doCreateFramebuffer();
		VkPipelineVertexInputStateCreateInfo doCreateVertexInputState( VkVertexInputAttributeDescriptionArray & vertexAttribs
			, VkVertexInputBindingDescriptionArray & vertexBindings );
		VkPipelineViewportStateCreateInfo doCreateViewportState( VkViewportArray & viewports
			, VkScissorArray & scissors );
		VkPipelineColorBlendStateCreateInfo doCreateBlendState( VkPipelineColorBlendAttachmentStateArray & blendAttachs );

	protected:
		rq::ConfigData m_config;

	private:
		bool m_useTexCoord{ true };
		VkBuffer m_vertexBuffer{ VK_NULL_HANDLE };
		VkDeviceMemory m_vertexMemory{ VK_NULL_HANDLE };
		VkRenderPass m_renderPass{ VK_NULL_HANDLE };
		VkFramebuffer m_frameBuffer{ VK_NULL_HANDLE };
	};

	template< typename ConfigT, typename BuilderT >
	class RenderQuadBuilderT
		: public RunnablePassBuilderT< RenderQuadBuilderT< ConfigT, BuilderT > >
	{
		static_assert( std::is_same_v< ConfigT, rq::Config >
			|| std::is_base_of_v< rq::Config, ConfigT >
			, "RenderQuadBuilderT::ConfigT must derive from crg::rq::Config" );

	public:
		RenderQuadBuilderT() = default;
		/**
		*\param[in] config
		*	The texture coordinates configuration.
		*/
		auto & texcoordConfig( rq::Texcoord config )
		{
			m_config.texcoordConfig = std::move( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The render area size.
		*/
		auto & renderSize( VkExtent2D config )
		{
			m_config.renderSize = std::move( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The render position.
		*/
		auto & renderPosition( VkOffset2D config )
		{
			m_config.renderPosition = std::move( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\brief
		*	Creates the RenderQuad.
		*\param[in] device
		*	The RenderDevice.
		*\param[in] pass
		*	The render pass.
		*/
		RunnablePassPtr build( RenderPass const & pass
			, GraphContext const & context
			, RunnableGraph const & graph )
		{
			return std::make_unique< RenderQuad >( pass
				, context
				, graph
				, m_config );
		}

	protected:
		ConfigT m_config;
	};

	class RenderQuadBuilder
		: public RenderQuadBuilderT< rq::Config, RenderQuadBuilder >
	{
	};
}
