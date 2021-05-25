/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "PipelinePass.hpp"
#include "RenderPass.hpp"

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
		struct ConfigT
		{
			WrapperT< VkPipelineShaderStageCreateInfoArray > program;
			WrapperT< Texcoord > texcoordConfig;
			WrapperT< VkExtent2D > renderSize;
			WrapperT< VkOffset2D > renderPosition;
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
		: public RenderPass
	{
	public:
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

	public:
		RenderQuad( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, rq::Config config );
		~RenderQuad();

		void resetPipeline( VkPipelineShaderStageCreateInfoArray config );

	protected:
		void doSubInitialise()override;
		void doSubRecordInto( VkCommandBuffer commandBuffer )const override;

		void doFillDescriptorBindings();
		void doCreateDescriptorSetLayout();
		void doCreatePipelineLayout();
		void doCreateDescriptorPool();
		void doCreateDescriptorSet();
		void doCreatePipeline();
		VkPipelineViewportStateCreateInfo doCreateViewportState( VkViewportArray & viewports
			, VkScissorArray & scissors );

	protected:
		rq::ConfigData m_config;

	private:
		bool m_useTexCoord{ true };
		VertexBuffer const * m_vertexBuffer{};
		WriteDescriptorSetArray m_descriptorWrites;
		VkDescriptorSetLayoutBindingArray m_descriptorBindings;
		VkDescriptorSetLayout m_descriptorSetLayout{ VK_NULL_HANDLE };
		VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline m_pipeline{ VK_NULL_HANDLE };
		VkDescriptorPool m_descriptorSetPool{ VK_NULL_HANDLE };
		VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	};

	template< typename ConfigT, typename BuilderT >
	class RenderQuadBuilderT
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
		*\param[in] config
		*	The pipeline program.
		*/
		auto & program( VkPipelineShaderStageCreateInfoArray config )
		{
			m_config.program = std::move( config );
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
			, RunnableGraph & graph )
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
