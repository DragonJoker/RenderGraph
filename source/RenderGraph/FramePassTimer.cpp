#include "RenderGraph/FramePassTimer.hpp"
#include "RenderGraph/GraphContext.hpp"

#include <cassert>

namespace crg
{
	using namespace std::literals::chrono_literals;

	//*********************************************************************************************

	FramePassTimerBlock::FramePassTimerBlock( FramePassTimer & timer )
		: m_timer{ &timer }
	{
	}

	FramePassTimerBlock::FramePassTimerBlock( FramePassTimerBlock && rhs )noexcept
		: m_timer{ rhs.m_timer }
	{
		rhs.m_timer = nullptr;
	}

	FramePassTimerBlock & FramePassTimerBlock::operator=( FramePassTimerBlock && rhs )noexcept
	{
		if ( this != &rhs )
		{
			m_timer = rhs.m_timer;
			rhs.m_timer = nullptr;
		}

		return *this;
	}

	FramePassTimerBlock::~FramePassTimerBlock()noexcept
	{
		if ( m_timer )
		{
			m_timer->stop();
		}
	}

	//*********************************************************************************************

	FramePassTimer::FramePassTimer( GraphContext & context
		, std::string const & name
		, TimerScope scope
		, VkQueryPool timerQueries
		, uint32_t & baseQueryOffset )
		: m_context{ context }
		, m_scope{ scope }
		, m_name{ name }
		, m_cpuTime{ 0ns }
		, m_gpuTime{ 0ns }
		, m_timerQueries{ timerQueries }
		, m_ownPool{ false }
		, m_queries{ { { baseQueryOffset, false, false }, { baseQueryOffset + 2u, false, false } } }
	{
		baseQueryOffset += 4u;
	}

	FramePassTimer::FramePassTimer( GraphContext & context
		, std::string const & name
		, TimerScope scope )
		: m_context{ context }
		, m_scope{ scope }
		, m_name{ name }
		, m_cpuTime{ 0ns }
		, m_gpuTime{ 0ns }
		, m_timerQueries{ createQueryPool( context, name, 4u ) }
		, m_ownPool{ true }
		, m_queries{ { { 0u, false, false }, { 2u, false, false } } }
	{
	}

	FramePassTimer::~FramePassTimer()noexcept
	{
		try
		{
			onDestroy( *this );

			if ( m_ownPool && m_timerQueries )
			{
				crgUnregisterObject( m_context, m_timerQueries );
				m_context.vkDestroyQueryPool( m_context.device
					, m_timerQueries
					, m_context.allocator );
			}
		}
		catch ( ... )
		{
			// Nothing to do here
		}
	}

	FramePassTimerBlock FramePassTimer::start()
	{
		m_cpuSaveTime = Clock::now();
		return FramePassTimerBlock{ *this };
	}

	void FramePassTimer::notifyPassRender( uint32_t passIndex
		, bool subtractGpuFromCpu )noexcept
	{
		auto & query = m_queries.front();
		query.started = true;
		query.subtractGpuFromCpu = subtractGpuFromCpu;
	}

	void FramePassTimer::stop()noexcept
	{
		auto current = Clock::now();
		m_cpuTime += ( current - m_cpuSaveTime );
		m_cpuTime -= m_subtractedGpuTime;
	}

	void FramePassTimer::reset()noexcept
	{
		m_cpuTime = 0ns;
		m_gpuTime = 0ns;
	}

	void FramePassTimer::beginPass( VkCommandBuffer commandBuffer )noexcept
	{
		std::swap( m_queries.front(), m_queries.back() );
		auto & query = m_queries.front();
		m_context.vkCmdResetQueryPool( commandBuffer
			, m_timerQueries
			, query.offset
			, 2u );
		m_context.vkCmdWriteTimestamp( commandBuffer
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, m_timerQueries
			, query.offset + 0u );
	}

	void FramePassTimer::endPass( VkCommandBuffer commandBuffer )noexcept
	{
		auto & query = m_queries.front();
		m_context.vkCmdWriteTimestamp( commandBuffer
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, m_timerQueries
			, query.offset + 1u );
		query.written = true;
	}

	void FramePassTimer::retrieveGpuTime()noexcept
	{
		static float const period = m_context.properties.limits.timestampPeriod;

		auto before = Clock::now();
		m_gpuTime = 0ns;
		m_subtractedGpuTime = 0ns;
		auto & query = m_queries.front();

		if ( query.started && query.written )
		{
			std::array< uint64_t, 2u > values{ 0u, 0u };
			m_context.vkGetQueryPoolResults( m_context.device
				, m_timerQueries
				, query.offset
				, 2u
				, sizeof( uint64_t ) * values.size()
				, values.data()
				, sizeof( uint64_t )
				, VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT );

			auto gpuTime = Nanoseconds{ uint64_t( float( values[1] - values[0] ) / period ) };
			m_gpuTime += gpuTime;

			if ( query.subtractGpuFromCpu )
			{
				m_subtractedGpuTime += gpuTime;
			}

			query.started = false;
			query.subtractGpuFromCpu = false;
			query.written = false;
		}

		auto after = Clock::now();
		m_cpuTime += ( after - before );
	}

	//*********************************************************************************************
}
