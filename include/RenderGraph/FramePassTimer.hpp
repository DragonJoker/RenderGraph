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

	class FramePassTimerBlock
	{
	public:
		CRG_API explicit FramePassTimerBlock( FramePassTimer & timer );
		CRG_API FramePassTimerBlock( FramePassTimerBlock && rhs )noexcept;
		CRG_API FramePassTimerBlock & operator=( FramePassTimerBlock && rhs )noexcept;
		CRG_API FramePassTimerBlock( FramePassTimerBlock const & ) = delete;
		CRG_API FramePassTimerBlock & operator=( FramePassTimerBlock const & ) = delete;
		CRG_API ~FramePassTimerBlock();

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
			, std::string const & name );
		CRG_API ~FramePassTimer();
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
			, bool subtractGpuFromCpu = false );
		/**
		*\brief
		*	Reset the timer's times.
		*/
		CRG_API void reset();
		/**
		*\brief
		*	Writes the timestamp for the beginning of the pass.
		*\param[in] cmd
		*	The command buffer used to record the begin timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		CRG_API void beginPass( VkCommandBuffer commandBuffer );
		/**
		*\brief
		*	Writes the timestamp for the end of the pass.
		*\param[in] cmd
		*	The command buffer used to record the end timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		CRG_API void endPass( VkCommandBuffer commandBuffer );
		/**
		*\brief
		*	Retrieves GPU time from the query.
		*/
		CRG_API void retrieveGpuTime();
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		Nanoseconds getCpuTime()const
		{
			return m_cpuTime;
		}

		Nanoseconds getGpuTime()const
		{
			return m_gpuTime;
		}

		std::string const & getName()const
		{
			return m_name;
		}
		/**@}*/

		OnFramePassDestroy onDestroy;

	private:
		void stop();

	private:
		GraphContext & m_context;
		std::string m_name;
		Clock::time_point m_cpuSaveTime;
		Nanoseconds m_cpuTime;
		Nanoseconds m_gpuTime;
		Nanoseconds m_subtractedGpuTime;
		VkQueryPool m_timerQueries;
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
