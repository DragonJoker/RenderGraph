#include "RenderGraph/FramePassTimer.hpp"
#include "RenderGraph/GraphContext.hpp"

#include <cassert>

namespace crg
{
	using namespace std::literals::chrono_literals;

	//*********************************************************************************************
	
	namespace
	{
		VkQueryPool createQueryPool( GraphContext const & context
			, std::string const & name
			, uint32_t passesCount )
		{
			VkQueryPool result{ VK_NULL_HANDLE };

			if ( context.device )
			{
				VkQueryPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
					, nullptr
					, 0u
					, VK_QUERY_TYPE_TIMESTAMP
					, 2u * passesCount
					, 0u };
				auto res = context.vkCreateQueryPool( context.device
					, &createInfo
					, context.allocator
					, &result );
				checkVkResult( res, ( name + " VkQueryPool creation" ).c_str() );
				crgRegisterObject( context, name + "QueryPools", result );
			}

			return result;
		}
	}

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

	FramePassTimerBlock::~FramePassTimerBlock()
	{
		if ( m_timer )
		{
			m_timer->stop();
		}
	}

	//*********************************************************************************************

	FramePassTimer::FramePassTimer( GraphContext const & context
		, std::string const & name
		, uint32_t passesCount )
		: m_context{ context }
		, m_passesCount{ passesCount }
		, m_name{ name }
		, m_timerQuery{ createQueryPool( context
			, name
			, passesCount ) }
		, m_cpuTime{ 0ns }
		, m_gpuTime{ 0ns }
		, m_startedPasses( size_t( m_passesCount ), { false, false } )
	{
	}

	FramePassTimer::~FramePassTimer()
	{
		onDestroy( *this );

		if ( m_timerQuery )
		{
			crgUnregisterObject( m_context, m_timerQuery );
			m_context.vkDestroyQueryPool( m_context.device
				, m_timerQuery
				, m_context.allocator );
		}
	}

	FramePassTimerBlock FramePassTimer::start()
	{
		m_cpuSaveTime = Clock::now();
		return FramePassTimerBlock{ *this };
	}

	void FramePassTimer::notifyPassRender( uint32_t passIndex
		, bool subtractGpuFromCpu )
	{
		auto & started = m_startedPasses[passIndex];
		started.first = true;
		started.second = subtractGpuFromCpu;
	}

	void FramePassTimer::stop()
	{
		auto current = Clock::now();
		m_cpuTime += ( current - m_cpuSaveTime );
		m_cpuTime -= m_subtracteGpuTime;
	}

	void FramePassTimer::reset()
	{
		m_cpuTime = 0ns;
		m_gpuTime = 0ns;
	}

	void FramePassTimer::beginPass( VkCommandBuffer commandBuffer
		, uint32_t passIndex )const
	{
		assert( passIndex < m_passesCount );
		m_context.vkCmdResetQueryPool( commandBuffer
			, m_timerQuery
			, passIndex * 2u
			, 2u );
		m_context.vkCmdWriteTimestamp( commandBuffer
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, m_timerQuery
			, passIndex * 2u + 0u );
	}

	void FramePassTimer::endPass( VkCommandBuffer commandBuffer
		, uint32_t passIndex )const
	{
		assert( passIndex < m_passesCount );
		m_context.vkCmdWriteTimestamp( commandBuffer
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, m_timerQuery
			, passIndex * 2u + 1u );
	}

	void FramePassTimer::retrieveGpuTime()
	{
		static float const period = float( m_context.properties.limits.timestampPeriod );
		m_gpuTime = 0ns;
		m_subtracteGpuTime = 0ns;

		for ( uint32_t i = 0; i < m_passesCount; ++i )
		{
			auto & started = m_startedPasses[i];

			if ( started.first )
			{
				std::vector< uint64_t > values{ 0u, 0u };
				m_context.vkGetQueryPoolResults( m_context.device
					, m_timerQuery
					, i * 2u
					, 2u
					, sizeof( uint64_t ) * values.size()
					, values.data()
					, sizeof( uint64_t )
					, VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT );

				auto gpuTime = Nanoseconds{ uint64_t( ( values[1] - values[0] ) / period ) };
				m_gpuTime += gpuTime;

				if ( started.second )
				{
					m_subtracteGpuTime += gpuTime;
				}

				started.first = false;
				started.second = false;
			}
		}
	}

	void FramePassTimer::updateCount( uint32_t count )
	{
		if ( m_passesCount != count )
		{
			m_passesCount = count;
			crgUnregisterObject( m_context, m_timerQuery );
			m_context.vkDestroyQueryPool( m_context.device
				, m_timerQuery
				, m_context.allocator );
			m_timerQuery = createQueryPool( m_context
				, m_name
				, m_passesCount );
			m_startedPasses.resize( m_passesCount );
		}
	}

	//*********************************************************************************************
}
