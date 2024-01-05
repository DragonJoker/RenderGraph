/*
See LICENSE file in root folder
*/
#ifndef ___CRG_RenderPassTimer_H___
#define ___CRG_RenderPassTimer_H___

#include "FrameGraphPrerequisites.hpp"
#include "Signal.hpp"

#include <array>
#include <chrono>

namespace crg
{
	using Clock = std::chrono::high_resolution_clock;
	using Nanoseconds = std::chrono::nanoseconds;
	using FramePassDestroyFunc = std::function< void( FramePassTimer & ) >;
	using OnFramePassDestroy = Signal< FramePassDestroyFunc >;
	using OnFramePassDestroyConnection = SignalConnection< OnFramePassDestroy >;

	enum class TimerScope
	{
		eGraph,
		ePass,
		eUpdate,
	};

	class FramePassTimerBlock
	{
	public:
		CRG_API explicit FramePassTimerBlock( FramePassTimer & timer );
		CRG_API FramePassTimerBlock( FramePassTimerBlock && rhs )noexcept;
		CRG_API FramePassTimerBlock & operator=( FramePassTimerBlock && rhs )noexcept;
		CRG_API FramePassTimerBlock( FramePassTimerBlock const & ) = delete;
		CRG_API FramePassTimerBlock & operator=( FramePassTimerBlock const & ) = delete;
		CRG_API ~FramePassTimerBlock()noexcept;

		FramePassTimer * operator->()const
		{
			return m_timer;
		}

	private:
		FramePassTimer * m_timer;
	};

	class FramePassTimer
	{
		friend class FramePassTimerBlock;

	public:
		FramePassTimer( FramePassTimer const & rhs ) = delete;
		FramePassTimer( FramePassTimer && rhs )noexcept = delete;
		FramePassTimer & operator=( FramePassTimer const & rhs ) = delete;
		FramePassTimer & operator=( FramePassTimer && rhs )noexcept = delete;
		/**
		*\brief
		*	Reserves queries from given pool.
		*\param[in] device
		*	The GPU device.
		*\param[in] category
		*	The render pass category.
		*\param[in] name
		*	The timer name.
		*/
		CRG_API FramePassTimer( GraphContext & context
			, std::string const & name
			, TimerScope scope
			, VkQueryPool timerQueries
			, uint32_t & baseQueryOffset );
		/**
		*\brief
		*	Owns its query pool.
		*\param[in] device
		*	The GPU device.
		*\param[in] category
		*	The render pass category.
		*\param[in] name
		*	The timer name.
		*/
		CRG_API FramePassTimer( GraphContext & context
			, std::string const & name
			, TimerScope scope );
		CRG_API ~FramePassTimer()noexcept;
		/**
		*\brief
		*	Starts the CPU timer, resets GPU time.
		*/
		CRG_API FramePassTimerBlock start();
		/**
		*\brief
		*	Notifies the given pass render.
		*\param[in] passIndex
		*	The pass index.
		*\param[in] subtractGpuFromCpu
		*	Tells if the GPU time should be subtracted from CPU time (in case of fence wait).
		*/
		CRG_API void notifyPassRender( uint32_t passIndex = 0u
			, bool subtractGpuFromCpu = false )noexcept;
		/**
		*\brief
		*	Reset the timer's times.
		*/
		CRG_API void reset()noexcept;
		/**
		*\brief
		*	Writes the timestamp for the beginning of the pass.
		*\param[in] cmd
		*	The command buffer used to record the begin timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		CRG_API void beginPass( VkCommandBuffer commandBuffer )noexcept;
		/**
		*\brief
		*	Writes the timestamp for the end of the pass.
		*\param[in] cmd
		*	The command buffer used to record the end timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		CRG_API void endPass( VkCommandBuffer commandBuffer )noexcept;
		/**
		*\brief
		*	Retrieves GPU time from the query.
		*/
		CRG_API void retrieveGpuTime()noexcept;
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		Nanoseconds getCpuTime()const noexcept
		{
			return m_cpuTime;
		}

		Nanoseconds getGpuTime()const noexcept
		{
			return m_gpuTime;
		}

		std::string const & getName()const noexcept
		{
			return m_name;
		}

		TimerScope getScope()const noexcept
		{
			return m_scope;
		}
		/**@}*/

		OnFramePassDestroy onDestroy;

	private:
		void stop()noexcept;

	private:
		GraphContext & m_context;
		TimerScope m_scope{};
		std::string m_name{};
		Clock::time_point m_cpuSaveTime{};
		Nanoseconds m_cpuTime{};
		Nanoseconds m_gpuTime{};
		Nanoseconds m_subtractedGpuTime{};
		VkQueryPool m_timerQueries{};
		bool m_ownPool{};
		struct Query
		{
			uint32_t offset{};
			bool written{};
			bool started{};
			bool subtractGpuFromCpu{};
		};
		std::array< Query, 2u > m_queries;
	};
}

#endif
