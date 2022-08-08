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
	template< typename StrongT, typename ValueT >
	struct GetValueCallbackT
	{
		using CallbackT = std::function< ValueT() >;

		GetValueCallbackT() = default;
		GetValueCallbackT( GetValueCallbackT const & ) = default;
		GetValueCallbackT( GetValueCallbackT && ) = default;
		GetValueCallbackT & operator=( GetValueCallbackT const & ) = default;
		GetValueCallbackT & operator=( GetValueCallbackT && ) = default;

		/**
		*\notes
		*	Intentionnally non explicit
		*/
		explicit GetValueCallbackT( CallbackT callback )
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
		CRG_API Fence( Fence && rhs );
		CRG_API Fence & operator=( Fence && rhs );
		CRG_API ~Fence();

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
			*\param[in] config
			*	The callback to recording the pass.
			*/
			auto & implicitAction( ImageViewId view
				, RecordContext::ImplicitAction action )
			{
				actions.emplace( view, action );
				return *this;
			}

			uint32_t maxPassCount{ 1u };
			bool resettable{ false };
			std::map< ImageViewId, RecordContext::ImplicitAction > actions{};
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
		struct SemaphoreWaitFlagsT
		{
		};
		struct EnabledT
		{
		};
		struct ComputePassT
		{
		};

		using InitialiseCallback = std::function< void() >;
		using RecordCallback = std::function< void( RecordContext &, VkCommandBuffer, uint32_t ) >;
		using GetSemaphoreWaitFlagsCallback = GetValueCallbackT< SemaphoreWaitFlagsT, VkPipelineStageFlags >;
		using GetPassIndexCallback = GetValueCallbackT< PassIndexT, uint32_t >;
		using IsEnabledCallback = GetValueCallbackT< EnabledT, bool >;
		using IsComputePassCallback = GetValueCallbackT< ComputePassT, bool >;

		struct Callbacks
		{
			CRG_API Callbacks( InitialiseCallback initialise
				, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
				, RecordCallback record );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
				, RecordCallback record
				, GetPassIndexCallback getPassIndex );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
				, RecordCallback record
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled );
			CRG_API Callbacks( InitialiseCallback initialise
				, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
				, RecordCallback record
				, GetPassIndexCallback getPassIndex
				, IsEnabledCallback isEnabled
				, IsComputePassCallback isComputePass );

			InitialiseCallback initialise;
			GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags;
			RecordCallback record;
			GetPassIndexCallback getPassIndex;
			IsEnabledCallback isEnabled;
			IsComputePassCallback isComputePass;
		};

	public:
		CRG_API RunnablePass( FramePass const & pass
			, GraphContext & context
			, RunnableGraph & graph
			, Callbacks callbacks
			, ru::Config config = {} );
		CRG_API virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		CRG_API uint32_t initialise();
		/**
		*\brief
		*	Records the pass commands into its command buffer.
		*\param[in] index
		*	The pass index.
		*/
		CRG_API void recordAll( RecordContext & context );
		/**
		*\brief
		*	Records the pass commands into its command buffer.
		*\param[in] index
		*	The pass index.
		*/
		CRG_API void recordCurrent( RecordContext & context );
		/**
		*\brief
		*	Re-records the pass commands into its command buffer.
		*/
		CRG_API void reRecordCurrent();
		/**
		*\brief
		*	Submits this pass' command buffer to the given queue.
		*\param[in] toWait
		*	The semaphore to wait for.
		*\param[out] queue
		*	The queue to submit to.
		*\return
		*	This pass' semaphore.
		*/
		CRG_API SemaphoreWaitArray run( SemaphoreWait toWait
			, VkQueue queue );
		/**
		*\brief
		*	Submits this pass' command buffer to the given queue.
		*\param[in] toWait
		*	The semaphores to wait for.
		*\param[out] queue
		*	The queue to submit to.
		*\return
		*	This pass' semaphore.
		*/
		CRG_API SemaphoreWaitArray run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		/**
		*\brief
		*	Resets the command buffer to initial state.
		*/
		CRG_API void resetCommandBuffer();
		CRG_API LayoutTransition const & getTransition( uint32_t passIndex
			, ImageViewId const & view )const;
		CRG_API AccessTransition const & getTransition( uint32_t passIndex
			, Buffer const & buffer )const;

		bool isEnabled()const
		{
			return m_callbacks.isEnabled();
		}

		uint32_t getIndex()const
		{
			return isEnabled() ? m_callbacks.getPassIndex() : ~( 0u );
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
			return uint32_t( m_commandBuffers.size() );
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

		void doCreateCommandPool();
		VkCommandBuffer doCreateCommandBuffer( std::string const & suffix );
		void doCreateCommandBuffers();
		void doCreateSemaphore();
		void doRegisterTransition( uint32_t passIndex
			, ImageViewId view
			, LayoutTransition transition );
		void doRegisterTransition( uint32_t passIndex
			, Buffer buffer
			, AccessTransition transition );

	protected:
		CRG_API void doUpdateFinalLayout( uint32_t passIndex
			, ImageViewId const & view
			, VkImageLayout layout
			, VkAccessFlags accessMask
			, VkPipelineStageFlags pipelineStage );
		CRG_API void doUpdateFinalAccess( uint32_t passIndex
			, Buffer const & buffer
			, VkAccessFlags accessMask
			, VkPipelineStageFlags pipelineStage );

	protected:
		FramePass const & m_pass;
		GraphContext & m_context;
		RunnableGraph & m_graph;
		Callbacks m_callbacks;
		ru::Config m_ruConfig;
		VkCommandPool m_commandPool{ nullptr };
		std::vector< CommandBuffer > m_commandBuffers;
		VkSemaphore m_semaphore{ nullptr };
		Fence m_fence;
		FramePassTimer m_timer;
		using LayoutTransitionMap = std::map< ImageViewId, LayoutTransition >;
		using AccessTransitionMap = std::map< VkBuffer, AccessTransition >;
		std::vector< LayoutTransitionMap > m_layoutTransitions;
		std::vector< AccessTransitionMap > m_accessTransitions;
		std::map< uint32_t, RecordContext > m_passContexts;
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::InitialiseCallback >
	{
		static RunnablePass::InitialiseCallback get()
		{
			RunnablePass::InitialiseCallback const result{ [](){} };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< RunnablePass::GetSemaphoreWaitFlagsCallback >
	{
		static RunnablePass::GetSemaphoreWaitFlagsCallback get()
		{
			RunnablePass::GetSemaphoreWaitFlagsCallback const result{ [](){ return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; } };
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
