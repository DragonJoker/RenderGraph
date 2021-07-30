/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnablePasses/RenderQuadHolder.hpp"
#include "RenderGraph/RunnablePasses/RenderPass.hpp"

namespace crg
{
	class RenderQuad
		: public RunnablePass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

	public:
		CRG_API RenderQuad( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, uint32_t maxPassCount
			, rq::Config config );
		CRG_API ~RenderQuad();

		CRG_API void resetPipeline( VkPipelineShaderStageCreateInfoArray config );

	private:
		void doInitialise();
		void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		void doRecordDisabledInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		VkPipelineStageFlags doGetSemaphoreWaitFlags()const;
		uint32_t doGetPassIndex()const;
		bool doIsEnabled()const;

	private:
		bool m_recordDisabledRenderPass{ true };
		RenderQuadHolder m_renderQuad;
		RenderPassHolder m_renderPass;
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
		auto & texcoordConfig( Texcoord config )
		{
			m_config.texcoordConfig( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The render area size.
		*/
		auto & renderSize( VkExtent2D config )
		{
			m_config.renderSize( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The render position.
		*/
		auto & renderPosition( VkOffset2D config )
		{
			m_config.renderPosition( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The depth stencil state.
		*/
		auto & depthStencilState( VkPipelineDepthStencilStateCreateInfo config )
		{
			m_config.depthStencilState( std::move( config ) );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The pass index.
		*/
		auto & passIndex( uint32_t const * config )
		{
			m_config.passIndex( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The enabled control variable.
		*/
		auto & enabled( bool const * config )
		{
			m_config.enabled( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The callback to recording the pass.
		*/
		auto & recordInto( RunnablePass::RecordCallback config )
		{
			m_config.recordInto( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	The callback to recording the disabled pass.
		*/
		auto & recordDisabledInto( rq::RecordDisabledIntoFunc config )
		{
			m_config.recordDisabledInto( config );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\param[in] config
		*	Tells if disabled pass should record render pass begin/end.
		*/
		auto & recordDisabledRenderPass( bool config )
		{
			m_config.recordDisabledRenderPass( config );
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
			, GraphContext & context
			, RunnableGraph & graph
			, uint32_t maxPassCount = 1u )
		{
			m_config.baseConfig( std::move( this->m_baseConfig ) );
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
