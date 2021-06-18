/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/PipelineHolder.hpp"
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

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

		using RecordDisabledIntoFunc = std::function< void( RunnablePass const &, VkCommandBuffer, uint32_t ) >;

		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			pp::ConfigT< WrapperT > baseConfig;
			WrapperT< Texcoord > texcoordConfig;
			WrapperT< VkExtent2D > renderSize;
			WrapperT< VkOffset2D > renderPosition;
			WrapperT< VkPipelineDepthStencilStateCreateInfo > depthStencilState;
			WrapperT< uint32_t const * > passIndex;
			WrapperT< bool const * > enabled;
			WrapperT< RecordDisabledIntoFunc > recordDisabledInto;
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< Texcoord > texcoordConfig;
			RawTypeT< VkExtent2D > renderSize;
			RawTypeT< VkOffset2D > renderPosition;
			RawTypeT< VkPipelineDepthStencilStateCreateInfo > depthStencilState;
			RawTypeT< uint32_t const * > passIndex;
			RawTypeT< bool const * > enabled;
			RawTypeT< RecordDisabledIntoFunc > recordDisabledInto;
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

	template<>
	struct DefaultValueGetterT< VkPipelineDepthStencilStateCreateInfo >
	{
		static VkPipelineDepthStencilStateCreateInfo const & get()
		{
			static VkPipelineDepthStencilStateCreateInfo const result{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
				, nullptr
				, 0u
				, VK_FALSE
				, VK_FALSE };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< uint32_t const * >
	{
		static uint32_t const * const & get()
		{
			static uint32_t const * const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< bool const * >
	{
		static bool const * const & get()
		{
			static bool const * const result{};
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< rq::RecordDisabledIntoFunc >
	{
		static rq::RecordDisabledIntoFunc const & get()
		{
			static rq::RecordDisabledIntoFunc const result{ []( crg::RunnablePass const &, VkCommandBuffer, uint32_t ){} };
			return result;
		}
	};

	class RenderQuad
		: public RenderPass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

	public:
		CRG_API RenderQuad( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, uint32_t maxPassCount
			, rq::Config config );
		CRG_API ~RenderQuad();

		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config
			, uint32_t index = 0u );

	protected:
		CRG_API void doSubInitialise( uint32_t index )override;
		CRG_API void doSubRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index )override;
		CRG_API void doSubRecordDisabledInto( VkCommandBuffer commandBuffer
			, uint32_t index )override;
		CRG_API uint32_t doGetPassIndex()const override;
		CRG_API bool doIsEnabled()const override;
		CRG_API void doCreatePipeline( uint32_t index );
		CRG_API VkPipelineViewportStateCreateInfo doCreateViewportState( VkViewportArray & viewports
			, VkScissorArray & scissors );

	protected:
		rq::ConfigData m_config;

	private:
		bool m_useTexCoord{ true };
		VertexBuffer const * m_vertexBuffer{};
		PipelineHolder m_holder;
	};

	template< typename ConfigT, typename BuilderT >
	class RenderQuadBuilderT
		: public PipelinePassBuilderT< BuilderT >
	{
		static_assert( std::is_same_v< ConfigT, rq::Config >
			|| std::is_base_of_v< rq::Config, ConfigT >
			, "RenderQuadBuilderT::ConfigT must derive from crg::rq::Config" );

	public:
		CRG_API RenderQuadBuilderT() = default;
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
		*\param[in] config
		*	The depth stencil state.
		*/
		auto & depthStencilState( VkPipelineDepthStencilStateCreateInfo config )
		{
			m_config.depthStencilState = std::move( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pass index.
		*/
		auto & passIndex( uint32_t const * config )
		{
			m_config.passIndex = config;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pass index.
		*/
		auto & enabled( bool const * config )
		{
			m_config.enabled = config;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pass index.
		*/
		auto & recordDisabledInto( rq::RecordDisabledIntoFunc config )
		{
			m_config.recordDisabledInto = config;
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
		std::unique_ptr< RenderQuad > build( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, uint32_t maxPassCount = 1u )
		{
			m_config.baseConfig = std::move( PipelinePassBuilderT< BuilderT >::m_baseConfig );
			return std::make_unique< RenderQuad >( pass
				, context
				, graph
				, maxPassCount
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
