/*
See LICENSE file in root folder
*/
#ifndef ___CRG_RenderPassTimer_H___
#define ___CRG_RenderPassTimer_H___

#include "FrameGraphPrerequisites.hpp"

#include <chrono>

namespace crg
{
	using Clock = std::chrono::high_resolution_clock;
	using Nanoseconds = std::chrono::nanoseconds;

	class FramePassTimerBlock
	{
	public:
		explicit FramePassTimerBlock( FramePassTimer & timer );
		FramePassTimerBlock( FramePassTimerBlock && rhs )noexcept;
		FramePassTimerBlock & operator=( FramePassTimerBlock && rhs )noexcept;
		FramePassTimerBlock( FramePassTimerBlock const & ) = delete;
		FramePassTimerBlock & operator=( FramePassTimerBlock const & ) = delete;
		~FramePassTimerBlock();

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
		*\param[in] device
		*	The GPU device.
		*\param[in] category
		*	The render pass category.
		*\param[in] name
		*	The timer name.
		*\param[in] passesCount
		*	The number of render passes.
		*/
		FramePassTimer( GraphContext const & context
			, std::string const & name
			, uint32_t passesCount = 1u );
		~FramePassTimer();
		/**
		*\brief
		*	Starts the CPU timer, resets GPU time.
		*/
		FramePassTimerBlock start();
		/**
		*\brief
		*	Notifies the given pass render.
		*\param[in] passIndex
		*	The pass index.
		*\param[in] subtractGpuFromCpu
		*	Tells if the GPU time should be subtracted from CPU time (in case of fence wait).
		*/
		void notifyPassRender( uint32_t passIndex = 0u
			, bool subtractGpuFromCpu = false );
		/**
		*\brief
		*	Reset the timer's times.
		*/
		void reset();
		/**
		*\brief
		*	Writes the timestamp for the beginning of the pass.
		*\param[in] cmd
		*	The command buffer used to record the begin timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		void beginPass( VkCommandBuffer commandBuffer
			, uint32_t passIndex = 0u )const;
		/**
		*\brief
		*	Writes the timestamp for the end of the pass.
		*\param[in] cmd
		*	The command buffer used to record the end timestamp.
		*\param[in] passIndex
		*	The pass index.
		*/
		void endPass( VkCommandBuffer commandBuffer
			, uint32_t passIndex = 0u )const;
		/**
		*\brief
		*	Retrieves GPU time from the query.
		*/
		void retrieveGpuTime();
		/**
		*\brief
		*	Updates the passes count to the given value.
		*\param[in] count
		*	The number of render passes.
		*\~french
		*\brief
		*	Met à jour le nombre de passes à la valeur donnée.
		*\param[in] count
		*	Le nombre de passes de rendu.
		*/
		void updateCount( uint32_t count );
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

		uint32_t getCount()const
		{
			return m_passesCount;
		}

		std::string const & getName()const
		{
			return m_name;
		}
		/**@}*/

	private:
		void stop();

	private:
		GraphContext const & m_context;
		uint32_t m_passesCount;
		std::string m_name;
		Clock::time_point m_cpuSaveTime;
		Nanoseconds m_cpuTime;
		Nanoseconds m_gpuTime;
		Nanoseconds m_subtracteGpuTime;
		VkQueryPool m_timerQuery;
		std::vector< std::pair< bool, bool > > m_startedPasses;
	};
}

#endif
