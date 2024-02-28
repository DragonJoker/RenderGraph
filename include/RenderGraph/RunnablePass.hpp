/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/FramePassTimer.hpp"
#include "RenderGraph/RecordContext.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <optional>

namespace crg
{
	CRG_API void checkUndefinedInput( std::string const & stepName
		, Attachment const & attach
		, ImageViewId const & view
		, VkImageLayout currentLayout );

	template< typename StrongT, typename ValueT >
	struct GetValueCallbackT
	{
		using CallbackT = std::function< ValueT() >;

		explicit GetValueCallbackT( CallbackT callback = {} )
			: m_callback{ std::move( callback ) }
		{
		}

		ValueT operator()()const
		{
			return m_callback();
		}

	private:
		CallbackT m_callback;
	};

	template< typename StrongT, typename ValueT >
	GetValueCallbackT< StrongT, ValueT > makeValueCallback( std::function< ValueT() > callback )
	{
		return GetValueCallbackT< StrongT, ValueT >{ callback };
	}

	class Fence
	{
	public:
		CRG_API Fence( GraphContext & context
			, std::string const & name
			, VkFenceCreateInfo create );

		Fence( Fence const & ) = delete;
		Fence & operator=( Fence const & ) = delete;
		CRG_API Fence & operator=( Fence && rhs )noexcept = delete;
		CRG_API Fence( Fence && rhs )noexcept;
		CRG_API ~Fence()noexcept;

		CRG_API void reset();
		CRG_API VkResult wait( uint64_t timeout );

		operator VkFence()const
		{
			return m_fence;
		}

	private:
		GraphContext * m_context;
		VkFence m_fence{};
		bool m_fenceWaited{};
	};

	namespace ru
	{
		struct Config
		{
			/**
			*\param[in] view
			*	The action's target viex.
			*\param[in] action
			*	The implicit action.
			*/
			auto & implicitAction( ImageViewId view
				, RecordContext::ImplicitAction action )
			{
				implicitActions.try_emplace( view, action );
				return *this;
			}

			/**
			*\param[in] action
			*	The action to run before the pass recording.
			*/
			auto & prePassAction( RecordContext::ImplicitAction action )
			{
				prePassActions.emplace_back( action );
				return *this;
			}

			/**
			*\param[in] action
			*	The action to run after the pass recording.
			*/
			auto & postPassAction( RecordContext::ImplicitAction action )
			{
				postPassActions.emplace_back( action );
				return *this;
			}

			uint32_t maxPassCount{ 1u };
			bool resettable{ false };
			std::vector< RecordContext::ImplicitAction > prePassActions{};
			std::vector< RecordContext::ImplicitAction > postPassActions{};
			std::map< ImageViewId, RecordContext::ImplicitAction > implicitActions{};
		};
	}

	template<>
	struct DefaultValueGetterT< ru::Config >
	{
		static ru::Config get()
		{
			ru::Config const result{ []()
				{
						return ru::Config{};
				}() };
			return result;
		}
	};

	class RunnablePass
	{
	public:
		struct LayoutTransition
		{
			// The layout the view is in, when coming to this pass
			LayoutState from;
			// The layout this pass needs the view to be in
			LayoutState needed;
			// The layout the view needs to be, out from this pass.
			LayoutState to;
		};

		struct AccessTransition
		{
			// The layout the buffer is in, when coming to this pass
			AccessState from;
			// The layout this pass needs the buffer to be in
			AccessState needed;
			// The layout the buffer needs to be, out from this pass.
			AccessState to;
		};

		struct PassIndexT
		{
		};
		struct PipelineStateT
		{
		};
		struct EnabledT
		{
		};
		struct ComputePassT
		{
		};

		using InitialiseCallback = std::function< void( uint32_t passIndex ) >;
		using RecordCallback = std::function< void( RecordContext &, VkCommandBuffer, uint32_t ) >;
		using GetPipelineStateCallback = GetValueCallbackT< PipelineStateT, PipelineState >;
		using GetPassIndexCallback = GetValueCallbackT< PassIndexT, uint32_t >;
		using IsEnabledCallback = GetValueCallbackT< EnabledT, bool >;
		using IsComputePassCallback = GetValueCallbackT< ComputePassT, bool >;

		struct Callbacks
		{
			CRG_API Callbacks( InitialiseCallback initialise
				, GetPipelineStateCallback getPipelineState );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetPipelineStateCallback getPipelineState
				, RecordCallback record );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetPipelineStateCallback getPipelineState
				, RecordCallback record
				, GetPassIndexCallback getPassIndex );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetPipelineStateCallback getPipelineState
				, RecordCallback record
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetPipelineStateCallback getPipelineState
				, RecordCallback record
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled
				, IsComputePassCallback isComputePass );

			InitialiseCallback initialise;
			GetPipelineStateCallback getPipelineState;
			RecordCallback record;
			GetPassIndexCallback getPassIndex;
			IsEnabledCallback isEnabled;
			IsComputePassCallback isComputePass;
		};

		static constexpr uint32_t InvalidIndex = ~0u;

	public:
		CRG_API RunnablePass( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Callbacks callbacks
			, ru::Config config = {} );
		CRG_API virtual ~RunnablePass()noexcept;
		/**
		*\brief
		*	Initialises the pass GPU data for given index.
		*\param[in] passIndex
		*	The index of the pass to initialise.
		*/
		CRG_API void initialise( uint32_t passIndex );
		/**
		*\brief
		*	Records the pass commands into the given command buffer.
		*\param[in,out] context
		*	Stores the states.
		*\param[in] commandBuffer
		*	Receives the commands.
		*/
		CRG_API uint32_t recordCurrentInto( RecordContext & context
			, VkCommandBuffer commandBuffer );
		/**
		*\brief
		*	Re-records the pass commands into its command buffer.
		*/
		CRG_API uint32_t reRecordCurrent();
		/**
		*\brief
		*	Resets the command buffer to initial state.
		*/
		CRG_API void resetCommandBuffer( uint32_t passIndex );
		CRG_API void notifyPassRender();

		LayoutState getLayoutState( crg::ImageViewId view )const
		{
			return m_imageLayouts.getLayoutState( view );
		}

		bool isEnabled()const
		{
			return m_callbacks.isEnabled();
		}

		uint32_t getIndex()const
		{
			return isEnabled() ? m_callbacks.getPassIndex() : InvalidIndex;
		}

		FramePass const & getPass()const
		{
			return m_pass;
		}

		FramePassTimer const & getTimer()const
		{
			return m_timer;
		}

		FramePassTimer & getTimer()
		{
			return m_timer;
		}

		uint32_t getMaxPassCount()const
		{
			return uint32_t( m_passes.size() );
		}

		PipelineState const & getPipelineState()const
		{
			return m_pipelineState;
		}

		LayerLayoutStatesMap const & getImageLayouts()const
		{
			return m_imageLayouts.images;
		}

	protected:
		struct CommandBuffer
		{
			VkCommandBuffer commandBuffer{};
			bool recorded{};
		};

	private:
		void recordOne( CommandBuffer & enabled
			, uint32_t index
			, RecordContext & context );
		void recordInto( VkCommandBuffer commandBuffer
			, uint32_t index
			, RecordContext & context );

		VkCommandBuffer doCreateCommandBuffer( std::string const & suffix );

	private:
		using LayoutTransitionMap = std::map< ImageViewId, LayoutTransition >;
		using AccessTransitionMap = std::map< VkBuffer, AccessTransition >;

		struct PassData
		{
			PassData( PassData const & ) = delete;
			PassData & operator=( PassData const & ) = delete;
			PassData & operator=( PassData && )noexcept = delete;
			PassData( PassData && rhs )noexcept
				: graph{ rhs.graph }
				, context{ rhs.context }
				, commandBuffer{ std::move( rhs.commandBuffer ) }
				, semaphore{ std::move( rhs.semaphore ) }
				, fence{ std::move( rhs.fence ) }
				, layoutTransitions{ std::move( rhs.layoutTransitions ) }
				, accessTransitions{ std::move( rhs.accessTransitions ) }
				, initialised{ rhs.initialised }
			{
				rhs.commandBuffer.commandBuffer = {};
				rhs.commandBuffer.recorded = {};
				rhs.semaphore = {};
				rhs.initialised = {};
			}

			PassData( RunnableGraph & graph
				, GraphContext & context
				, std::string const & baseName );
			~PassData()noexcept;

			RunnableGraph & graph;
			GraphContext & context;
			CommandBuffer commandBuffer;
			VkSemaphore semaphore{};
			Fence fence;
			LayoutTransitionMap layoutTransitions;
			AccessTransitionMap accessTransitions;
			bool initialised{};
		};

	protected:
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		Callbacks m_callbacks;
		ru::Config m_ruConfig;
		PipelineState m_pipelineState;
		std::vector< PassData > m_passes;
		FramePassTimer m_timer;
		std::map< uint32_t, RecordContext > m_passContexts;
		LayerLayoutStatesHandler m_imageLayouts;
		AccessStateMap m_bufferAccesses;
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::InitialiseCallback >
	{
		static RunnablePass::InitialiseCallback get()
		{
			RunnablePass::InitialiseCallback const result{ []( uint32_t ){} };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::GetPipelineStateCallback >
	{
		static RunnablePass::GetPipelineStateCallback get()
		{
			RunnablePass::GetPipelineStateCallback const result{ [](){ return PipelineState{ VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::RecordCallback >
	{
		static RunnablePass::RecordCallback get()
		{
			RunnablePass::RecordCallback const result{ []( RecordContext &, VkCommandBuffer, uint32_t ){} };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::GetPassIndexCallback >
	{
		static RunnablePass::GetPassIndexCallback get()
		{
			RunnablePass::GetPassIndexCallback const result{ [](){ return 0u; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::IsEnabledCallback >
	{
		static RunnablePass::IsEnabledCallback get()
		{
			RunnablePass::IsEnabledCallback const result{ [](){ return true; } };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::IsComputePassCallback >
	{
		static RunnablePass::IsComputePassCallback get()
		{
			RunnablePass::IsComputePassCallback const result{ [](){ return false; } };
			return result;
		}
	};
}
