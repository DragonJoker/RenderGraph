/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/RunnablePasses/PipelineConfig.hpp"

namespace crg
{
	struct PrimitiveCountT
	{
	};
	struct VertexCountT
	{
	};
	struct IndexTypeT
	{
	};
	struct CullModeT
	{
	};

	using GetPrimitiveCountCallback = GetValueCallbackT< PrimitiveCountT, uint32_t >;
	using GetVertexCountCallback = GetValueCallbackT< VertexCountT, uint32_t >;
	using GetIndexTypeCallback = GetValueCallbackT< IndexTypeT, VkIndexType >;
	using GetCullModeCallback = GetValueCallbackT< CullModeT, VkCullModeFlags >;

	namespace rm
	{
		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			/**
			*\param[in] config
			*	The pipeline program.
			*/
			auto & program( VkPipelineShaderStageCreateInfoArray config )
			{
				m_baseConfig.program( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline programs.
			*/
			auto & programs( std::vector< VkPipelineShaderStageCreateInfoArray > config )
			{
				m_baseConfig.programs( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The pipeline program creator.
			*/
			auto & programCreator( ProgramCreator config )
			{
				m_baseConfig.programCreator( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layout.
			*/
			auto & layout( VkDescriptorSetLayout config )
			{
				m_baseConfig.layout( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	The descriptor set layouts.
			*/
			auto & layouts( std::vector< VkDescriptorSetLayout > config )
			{
				m_baseConfig.layouts( std::move( config ) );
				return *this;
			}
			/**
			*\param[in] config
			*	Tells if disabled pass should record render pass begin/end.
			*/
			auto & baseConfig( pp::Config config )
			{
				m_baseConfig = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The render area size.
			*/
			auto & renderSize( Extent2D config )
			{
				m_renderSize = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The render position.
			*/
			auto & renderPosition( Offset2D config )
			{
				m_renderPosition = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The depth stencil state.
			*/
			auto & depthStencilState( VkPipelineDepthStencilStateCreateInfo config )
			{
				m_depthStencilState = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The pass index callback.
			*/
			auto & getPassIndex( RunnablePass::GetPassIndexCallback config )
			{
				m_getPassIndex = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The callback checking the enable status of the pass.
			*/
			auto & isEnabled( RunnablePass::IsEnabledCallback config )
			{
				m_isEnabled = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The callback recording the pass.
			*/
			auto & recordInto( RunnablePass::RecordCallback config )
			{
				m_recordInto = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The callback ending the pass.
			*/
			auto & end( RunnablePass::RecordCallback config )
			{
				m_end = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The vertex buffer.
			*/
			auto & vertexBuffer( VertexBuffer config )
			{
				m_vertexBuffer = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The index buffer.
			*/
			auto & indexBuffer( IndexBuffer config )
			{
				m_indexBuffer = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The indirect buffer.
			*/
			auto & indirectBuffer( IndirectBuffer config )
			{
				m_indirectBuffer = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The primitive count retrieval callback.
			*/
			auto & getPrimitiveCount( GetPrimitiveCountCallback config )
			{
				m_getPrimitiveCount = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The vertex count retrieval callback.
			*/
			auto & getVertexCount( GetVertexCountCallback config )
			{
				m_getVertexCount = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The index type retrieval callback.
			*/
			auto & getIndexType( GetIndexTypeCallback config )
			{
				m_getIndexType = std::move( config );
				return *this;
			}
			/**
			*\param[in] config
			*	The rasterizer cull mode.
			*/
			auto & getCullMode( GetCullModeCallback config )
			{
				m_getCullMode = std::move( config );
				return *this;
			}

			pp::ConfigT< WrapperT > m_baseConfig{};
			WrapperT< Offset2D > m_renderPosition{};
			WrapperT< VkPipelineDepthStencilStateCreateInfo > m_depthStencilState{};
			WrapperT< RunnablePass::GetPassIndexCallback > m_getPassIndex{};
			WrapperT< RunnablePass::IsEnabledCallback > m_isEnabled{};
			WrapperT< RunnablePass::RecordCallback > m_recordInto{};
			WrapperT< RunnablePass::RecordCallback > m_end{};
			WrapperT< GetPrimitiveCountCallback > m_getPrimitiveCount{};
			WrapperT< GetVertexCountCallback > m_getVertexCount{};
			WrapperT< GetIndexTypeCallback > m_getIndexType{};
			WrapperT< GetCullModeCallback > m_getCullMode{};
			WrapperT< Extent2D > m_renderSize{};
			WrapperT< VertexBuffer > m_vertexBuffer{};
			WrapperT< IndexBuffer > m_indexBuffer{};
			WrapperT< IndirectBuffer > m_indirectBuffer{};
		};

		template<>
		struct ConfigT< RawTypeT >
		{
			RawTypeT< Offset2D > renderPosition{};
			RawTypeT< VkPipelineDepthStencilStateCreateInfo > depthStencilState{};
			RawTypeT< RunnablePass::GetPassIndexCallback > getPassIndex{};
			RawTypeT< RunnablePass::IsEnabledCallback > isEnabled{};
			RawTypeT< RunnablePass::RecordCallback > recordInto{};
			RawTypeT< RunnablePass::RecordCallback > end{};
			RawTypeT< GetPrimitiveCountCallback > getPrimitiveCount{};
			RawTypeT< GetVertexCountCallback > getVertexCount{};
			RawTypeT< GetIndexTypeCallback > getIndexType{};
			RawTypeT< GetCullModeCallback > getCullMode{};
			RawTypeT< VertexBuffer > vertexBuffer{};
			RawTypeT< IndexBuffer > indexBuffer{};
			RawTypeT< IndirectBuffer > indirectBuffer{ defaultV< IndirectBuffer > };
		};

		using Config = ConfigT< std::optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	template<>
	struct DefaultValueGetterT< GetPrimitiveCountCallback >
	{
		static GetPrimitiveCountCallback get()
		{
			GetPrimitiveCountCallback const result{ [](){ return 1u; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< GetVertexCountCallback >
	{
		static GetVertexCountCallback get()
		{
			GetVertexCountCallback const result{ [](){ return 1u; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< GetIndexTypeCallback >
	{
		static GetIndexTypeCallback get()
		{
			GetIndexTypeCallback const result{ [](){ return VK_INDEX_TYPE_UINT32; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< GetCullModeCallback >
	{
		static GetCullModeCallback get()
		{
			GetCullModeCallback const result{ [](){ return VK_CULL_MODE_NONE; } };
			return result;
		}
	};
}
