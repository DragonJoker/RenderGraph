#include "Common.hpp"

#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>
#include <RenderGraph/RunnableGraph.hpp>
#include <RenderGraph/RunnablePass.hpp>
#include <RenderGraph/RunnablePasses/GenerateMipmaps.hpp>

#include <sstream>

namespace
{
	using CheckViews = std::function< void( test::TestCounts &
		, crg::FramePass const &
		, crg::RunnableGraph const & ) >;

	class DummyRunnable
		: public crg::RunnablePass
	{
	public:
		DummyRunnable( crg::FramePass const & pass
			, crg::GraphContext & context
			, crg::RunnableGraph & graph
			, test::TestCounts & testCounts
			, VkPipelineStageFlags pipelineStageFlags
			, CheckViews checkViews )
			: crg::RunnablePass{ pass
				, context
				, graph
				, { crg::RunnablePass::InitialiseCallback( [this](){ doInitialise(); } )
					, crg::RunnablePass::GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } ) } }
			, m_testCounts{ testCounts }
			, m_pipelineStageFlags{ pipelineStageFlags }
			, m_checkViews{ checkViews }
		{
		}

	private:
		void doInitialise()
		{
			m_checkViews( m_testCounts
				, m_pass
				, m_graph );
		}

		VkPipelineStageFlags doGetSemaphoreWaitFlags()const
		{
			return m_pipelineStageFlags;
		}

	private:
		test::TestCounts & m_testCounts;
		VkPipelineStageFlags m_pipelineStageFlags;
		CheckViews m_checkViews;
	};

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & pass
		, crg::GraphContext & context
		, crg::RunnableGraph & graph
		, VkPipelineStageFlags pipelineStageFlags
		, CheckViews checkViews )
	{
		return std::make_unique< DummyRunnable >( pass
			, context
			, graph
			, testCounts
			, pipelineStageFlags
			, checkViews );
	}

	void checkDummy( test::TestCounts & testCounts
		, crg::FramePass const & pass
		, crg::RunnableGraph const & graph )
	{
	}

	crg::GraphContext & getContext()
	{
		static crg::GraphContext context{ nullptr
			, nullptr
			, nullptr
			, VkPhysicalDeviceMemoryProperties{}
			, VkPhysicalDeviceProperties{}
			, false
			, nullptr };
		return context;
	}

	void checkOutputColourIsShaderReadOnly( test::TestCounts & testCounts
		, crg::FramePass const & pass
		, crg::RunnableGraph const & graph )
	{
		for ( auto & view : pass.images )
		{
			if ( view.isColourOutputAttach() )
			{
				check( graph.getOutputLayout( pass, view.view( 0u ), false ).layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			}
		}
	}

	void checkSampledIsShaderReadOnly( test::TestCounts & testCounts
		, crg::FramePass const & pass
		, crg::RunnableGraph const & graph )
	{
		for ( auto & view : pass.images )
		{
			if ( view.isSampledView() )
			{
				check( graph.getCurrentLayout( pass, 0u, view.view( 0u ) ).layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			}
		}
	}

	void testNoPass( test::TestCounts & testCounts )
	{
		testBegin( "testNoPass" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		checkThrow( graph.compile( getContext() ) );

		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass.addOutputColourView( rtv );

		testEnd();
	}

	void testOnePass( test::TestCounts & testCounts )
	{
		testBegin( "testOnePass" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass.addOutputColourView(  rtv );
		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testDuplicateName( test::TestCounts & testCounts )
	{
		testBegin( "testDuplicateName" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		checkNoThrow( graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} ) );
		checkThrow( graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} ) );
		testEnd();
	}

	void testOneDependency( test::TestCounts & testCounts )
	{
		testBegin( "testOneDependency" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass1 = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addOutputColourView( rtv );

		auto out = graph.createImage( test::createImage( "out", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( "outv", out ) );
		auto & pass2 = graph.createPass( "pass2C"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( rtv, 0u );
		pass2.addOutputColourView( outv );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\npass2CrtvSpl" -> "pass2C" [ label="rtv" ];
    "Transition to\npass2CrtvSpl" [ shape=box ];
    "pass1C" -> "Transition to\npass2CrtvSpl" [ label="rtv" ];
    "pass1C" [ shape=ellipse ];
    "pass2C" [ shape=ellipse ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testChainedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testChainedDependencies" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d0 = graph.createImage( test::createImage( "d0", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass0.addOutputColourView( d0v );

		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0u );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0u );
		pass2.addOutputColourView( d2v );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\npass1d0vSpl" [ shape=box ];
    "pass0" [ shape=ellipse ];
    "pass1" [ shape=ellipse ];
    "pass0" -> "Transition to\npass1d0vSpl" [ label="d0v" ];
    "Transition to\npass1d0vSpl" -> "pass1" [ label="d0v" ];
    "Transition to\npass2d1vSpl" [ shape=box ];
    "pass2" [ shape=ellipse ];
    "pass1" -> "Transition to\npass2d1vSpl" [ label="d1v" ];
    "Transition to\npass2d1vSpl" -> "pass2" [ label="d1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testSharedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testSharedDependencies" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dstv1 = graph.createView( test::createView( "dstv1", d ) );
		auto d0 = graph.createImage( test::createImage( "d0", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass0.addOutputDepthView( dstv1 );
		pass0.addOutputColourView( d0v );

		auto dstv2 = graph.createView( test::createView( "dstv2", d ) );
		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0 );
		pass1.addOutputDepthView( dstv2 );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0 );
		pass2.addOutputColourView( d2v );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\npass1d0vSpl" [ shape=box ];
    "pass0" [ shape=ellipse ];
    "pass1" [ shape=ellipse ];
    "pass0" -> "Transition to\npass1d0vSpl" [ label="d0v" ];
    "Transition to\npass1d0vSpl" -> "pass1" [ label="d0v" ];
    "Transition to\npass2d1vSpl" [ shape=box ];
    "pass2" [ shape=ellipse ];
    "pass1" -> "Transition to\npass2d1vSpl" [ label="d1v" ];
    "Transition to\npass2d1vSpl" -> "pass2" [ label="d1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void test2MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test2MipDependencies" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void test3MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test3MipDependencies" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", VK_FORMAT_R32G32B32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testLoopDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependencies" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

		checkNoThrow( graph.compile( getContext() ) );
		testEnd();
	}

	void testLoopDependenciesWithRoot( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRoot" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass0.addOutputColourView( bv );

		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

        checkNoThrow( graph.compile( getContext() ) );
		testEnd();
	}

	void testLoopDependenciesWithRootAndLeaf( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRootAndLeaf" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto c = graph.createImage( test::createImage( "c", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( "cv", c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass0.addOutputColourView( cv );

		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addSampledView( cv, 1 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addSampledView( cv, 1 );
		pass2.addOutputColourView( bv );

		auto & pass3 = graph.createPass( "pass3"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		pass3.addDependency( pass2 );
		pass3.addSampledView( cv, 0 );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\npass1cvSpl" [ shape=box ];
    "pass0" [ shape=ellipse ];
    "pass1" [ shape=ellipse ];
    "pass0" -> "Transition to\npass1cvSpl" [ label="cv" ];
    "Transition to\npass1cvSpl" -> "pass1" [ label="cv" ];
    "Transition to\npass2avSpl" [ shape=box ];
    "pass2" [ shape=ellipse ];
    "pass1" -> "Transition to\npass2avSpl" [ label="av" ];
    "Transition to\npass2avSpl" -> "pass2" [ label="av" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	std::pair< crg::ImageViewId, crg::FramePass * > buildSsaoPass( test::TestCounts & testCounts
		, crg::FramePass const & previous
		, crg::ImageViewId const & dsv
		, crg::ImageViewId const & d2sv
		, crg::FrameGraph & graph )
	{
		auto lp = graph.createImage( test::createImage( "lp", VK_FORMAT_R32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, VK_FORMAT_R32_SFLOAT, 0u ) );
		auto & ssaoLinearisePass = graph.createPass( "ssaoLinearisePass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoLinearisePass.addDependency( previous );
		ssaoLinearisePass.addSampledView( dsv, 0 );
		ssaoLinearisePass.addOutputColourView( m0v );

		auto m1v = graph.createView( test::createView( "m1v", lp, VK_FORMAT_R32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass1.addDependency( ssaoLinearisePass );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		auto mv = graph.createView( test::createView("mv",  lp, VK_FORMAT_R32_SFLOAT, 0u, 4u ) );
		auto rs = graph.createImage( test::createImage( "rs", VK_FORMAT_R32_SFLOAT ) );
		auto rsv = graph.createView( test::createView( "rsv", rs, VK_FORMAT_R32_SFLOAT ) );
		auto & ssaoRawPass = graph.createPass( "ssaoRawPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoRawPass.addDependency( ssaoMinifyPass3 );
		ssaoRawPass.addSampledView( mv, 0 );
		ssaoRawPass.addSampledView( m3v, 1 );
		ssaoRawPass.addOutputColourView( rsv );

		auto bl = graph.createImage( test::createImage( "b1", VK_FORMAT_R32_SFLOAT ) );
		auto blv = graph.createView( test::createView( "b1v", bl, VK_FORMAT_R32_SFLOAT ) );
		auto & ssaoBlurPass = graph.createPass( "ssaoBlurPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ssaoBlurPass.addDependency( ssaoRawPass );
		ssaoBlurPass.addSampledView( rsv, 0 );
		ssaoBlurPass.addSampledView( m3v, 1 );
		ssaoBlurPass.addOutputColourView( blv );

		return { blv, &ssaoBlurPass };
	}

	void testSsaoPass( test::TestCounts & testCounts )
	{
		testBegin( "testSsaoPass" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( "dtv", d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto v = graph.createImage( test::createImage( "v", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto vv = graph.createView( test::createView( "vv", v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3 = graph.createImage( test::createImage( "d3", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( "d3v", d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4 = graph.createImage( test::createImage( "d4", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( "d4v", d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto & geometryPass = graph.createPass( "geometryPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		geometryPass.addOutputColourView( d1v );
		geometryPass.addOutputColourView( d2v );
		geometryPass.addOutputColourView( d3v );
		geometryPass.addOutputColourView( d4v );
		geometryPass.addOutputColourView( vv );
		geometryPass.addOutputDepthStencilView( dtv );

		auto dsv = graph.createView( test::createView( "dsv", d, VK_FORMAT_R32_SFLOAT ) );
		auto ssaoResult = buildSsaoPass( testCounts
			, geometryPass
			, dsv
			, d2v
			, graph );

		auto of = graph.createImage( test::createImage( "of", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( "ofv", of, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & ambientPass = graph.createPass( "ambientPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		ambientPass.addDependency( *ssaoResult.second );
		ambientPass.addSampledView( dsv, 0 );
		ambientPass.addSampledView( d1v, 1 );
		ambientPass.addSampledView( d2v, 2 );
		ambientPass.addSampledView( d3v, 3 );
		ambientPass.addSampledView( d4v, 4 );
		ambientPass.addSampledView( ssaoResult.first, 5 );
		ambientPass.addOutputColourView( ofv );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nssaoLinearisePassdsvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ssaoLinearisePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nssaoLinearisePassdsvSpl" [ label="dtv" ];
    "Transition to\nssaoLinearisePassdsvSpl" -> "ssaoLinearisePass" [ label="dtv" ];
    "Transition to\nssaoMinifyPass1m0vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1m0vSpl" [ label="m0v" ];
    "Transition to\nssaoMinifyPass1m0vSpl" -> "ssaoMinifyPass1" [ label="m0v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
    "Transition to\nssaoRawPassm3vSpl" [ shape=box ];
    "ssaoRawPass" [ shape=ellipse ];
    "ssaoMinifyPass3" -> "Transition to\nssaoRawPassm3vSpl" [ label="m3v" ];
    "Transition to\nssaoRawPassm3vSpl" -> "ssaoRawPass" [ label="m3v" ];
    "Transition to\nssaoBlurPassrsvSpl" [ shape=box ];
    "ssaoBlurPass" [ shape=ellipse ];
    "ssaoRawPass" -> "Transition to\nssaoBlurPassrsvSpl" [ label="rsv" ];
    "Transition to\nssaoBlurPassrsvSpl" -> "ssaoBlurPass" [ label="rsv" ];
    "Transition to\nambientPassb1vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "ssaoBlurPass" -> "Transition to\nambientPassb1vSpl" [ label="b1v" ];
    "Transition to\nambientPassb1vSpl" -> "ambientPass" [ label="b1v" ];
    "Transition to\nambientPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd1vSpl" [ label="d1v" ];
    "Transition to\nambientPassd1vSpl" -> "ambientPass" [ label="d1v" ];
    "Transition to\nambientPassd2vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd2vSpl" [ label="d2v" ];
    "Transition to\nambientPassd2vSpl" -> "ambientPass" [ label="d2v" ];
    "Transition to\nambientPassd3vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd3vSpl" [ label="d3v" ];
    "Transition to\nambientPassd3vSpl" -> "ambientPass" [ label="d3v" ];
    "Transition to\nambientPassd4vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd4vSpl" [ label="d4v" ];
    "Transition to\nambientPassd4vSpl" -> "ambientPass" [ label="d4v" ];
    "Transition to\nambientPassdsvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassdsvSpl" [ label="dtv" ];
    "Transition to\nambientPassdsvSpl" -> "ambientPass" [ label="dtv" ];
}
)";
		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testBloomPostEffect( test::TestCounts & testCounts )
	{
		testBegin( "testBloomPostEffect" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto scene = graph.createImage( test::createImage( "scene", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto scenev = graph.createView( test::createView( "scenev", scene, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto output = graph.createImage( test::createImage( "output", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outputv = graph.createView( test::createView( "outputv", output, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto hi = graph.createImage( test::createImage( "hi", VK_FORMAT_R32G32B32A32_SFLOAT, 4u ) );
		auto hi0v = graph.createView( test::createView( "hi0v", hi, VK_FORMAT_R32G32B32A32_SFLOAT, 0u ) );
		auto hi1v = graph.createView( test::createView( "hi1v", hi, VK_FORMAT_R32G32B32A32_SFLOAT, 1u ) );
		auto hi2v = graph.createView( test::createView( "hi2v", hi, VK_FORMAT_R32G32B32A32_SFLOAT, 2u ) );
		auto hi3v = graph.createView( test::createView( "hi3v", hi, VK_FORMAT_R32G32B32A32_SFLOAT, 3u ) );
		auto bl = graph.createImage( test::createImage( "bl", VK_FORMAT_R32G32B32A32_SFLOAT, 4u ) );
		auto bl0v = graph.createView( test::createView( "bl0v", bl, VK_FORMAT_R32G32B32A32_SFLOAT, 0u ) );
		auto bl1v = graph.createView( test::createView( "bl1v", bl, VK_FORMAT_R32G32B32A32_SFLOAT, 1u ) );
		auto bl2v = graph.createView( test::createView( "bl2v", bl, VK_FORMAT_R32G32B32A32_SFLOAT, 2u ) );
		auto bl3v = graph.createView( test::createView( "bl3v", bl, VK_FORMAT_R32G32B32A32_SFLOAT, 3u ) );

		auto & hiPass = graph.createPass( "hiPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		hiPass.addSampledView( scenev, 0u );
		hiPass.addOutputColourView( hiPass.mergeViews( { hi0v, hi1v, hi2v, hi3v } ) );

		auto & blurPass0X = graph.createPass( "blurPass0X"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass0X.addDependency( hiPass );
		blurPass0X.addSampledView( hi0v, 0u );
		blurPass0X.addOutputColourView( bl0v );
		auto & blurPass0Y = graph.createPass( "blurPass0Y"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass0Y.addDependency( blurPass0X );
		blurPass0Y.addSampledView( bl0v, 0u );
		blurPass0Y.addOutputColourView( hi0v );

		auto & blurPass1X = graph.createPass( "blurPass1X"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass1X.addDependency( hiPass );
		blurPass1X.addSampledView( hi1v, 0u );
		blurPass1X.addOutputColourView( bl1v );
		auto & blurPass1Y = graph.createPass( "blurPass1Y"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass1Y.addDependency( blurPass1X );
		blurPass1Y.addSampledView( bl1v, 0u );
		blurPass1Y.addOutputColourView( hi1v );

		auto & blurPass2X = graph.createPass( "blurPass2X"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass2X.addDependency( hiPass );
		blurPass2X.addSampledView( hi2v, 0u );
		blurPass2X.addOutputColourView( bl2v );
		auto & blurPass2Y = graph.createPass( "blurPass2Y"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass2Y.addDependency( blurPass2X );
		blurPass2Y.addSampledView( bl2v, 0u );
		blurPass2Y.addOutputColourView( hi2v );

		auto & blurPass3X = graph.createPass( "blurPass3X"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass3X.addDependency( hiPass );
		blurPass3X.addSampledView( hi3v, 0u );
		blurPass3X.addOutputColourView( bl3v );
		auto & blurPass3Y = graph.createPass( "blurPass3Y"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		blurPass3Y.addDependency( blurPass3X );
		blurPass3Y.addSampledView( bl3v, 0u );
		blurPass3Y.addOutputColourView( hi3v );

		auto & combinePass = graph.createPass( "combinePass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		combinePass.addDependency( blurPass0Y );
		combinePass.addDependency( blurPass1Y );
		combinePass.addDependency( blurPass2Y );
		combinePass.addDependency( blurPass3Y );
		combinePass.addSampledView( scenev, 0u );
		combinePass.addSampledView( combinePass.mergeViews( { hi0v, hi1v, hi2v, hi3v } ), 1u );
		combinePass.addOutputColourView( outputv );

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		testEnd();
	}

	template< bool EnableSsao >
	std::pair< crg::ImageViewId, crg::FramePass * > buildDeferred( test::TestCounts & testCounts
		, crg::FramePass const * previous
		, crg::ImageViewId const & dsv
		, crg::ImageViewId const & dtv
		, crg::ImageViewId const & vtv
		, crg::FrameGraph & graph )
	{
		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3 = graph.createImage( test::createImage( "d3", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( "d3v", d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4 = graph.createImage( test::createImage( "d4", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( "d4v", d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto & geometryPass = graph.createPass( "geometryPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );

		if ( previous )
		{
			geometryPass.addDependency( *previous );
		}

		geometryPass.addOutputColourView( d1v );
		geometryPass.addOutputColourView( d2v );
		geometryPass.addOutputColourView( d3v );
		geometryPass.addOutputColourView( d4v );
		geometryPass.addOutputColourView( vtv );
		geometryPass.addOutputDepthStencilView( dtv );

		auto df = graph.createImage( test::createImage( "df", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto dfv = graph.createView( test::createView( "dfv", df, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto sp = graph.createImage( test::createImage( "sp", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto spv = graph.createView( test::createView( "spv", sp, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & lightingPass = graph.createPass( "lightingPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		lightingPass.addDependency( geometryPass );
		lightingPass.addSampledView( dtv, 0 );
		lightingPass.addSampledView( d1v, 1 );
		lightingPass.addSampledView( d2v, 2 );
		lightingPass.addSampledView( d3v, 3 );
		lightingPass.addSampledView( d4v, 4 );
		lightingPass.addOutputColourView( dfv );
		lightingPass.addOutputColourView( spv );

		auto of = graph.createImage( test::createImage( "of", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( "ofv", of, VK_FORMAT_R32G32B32_SFLOAT ) );

		if constexpr ( EnableSsao )
		{
			auto ssaoResult = buildSsaoPass( testCounts
				, geometryPass
				, dsv
				, d2v
				, graph );
			auto & ambientPass = graph.createPass( "ambientPass"
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			ambientPass.addDependency( *ssaoResult.second );
			ambientPass.addDependency( lightingPass );
			ambientPass.addSampledView( dsv, 0 );
			ambientPass.addSampledView( d1v, 1 );
			ambientPass.addSampledView( d2v, 2 );
			ambientPass.addSampledView( d3v, 3 );
			ambientPass.addSampledView( d4v, 4 );
			ambientPass.addSampledView( dfv, 5 );
			ambientPass.addSampledView( spv, 6 );
			ambientPass.addSampledView( ssaoResult.first, 7 );
			ambientPass.addOutputColourView( ofv );
			return { ofv, &ambientPass };
		}
		else
		{
			auto & ambientPass = graph.createPass( "ambientPass"
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			ambientPass.addDependency( lightingPass );
			ambientPass.addSampledView( dsv, 0 );
			ambientPass.addSampledView( d1v, 1 );
			ambientPass.addSampledView( d2v, 2 );
			ambientPass.addSampledView( d3v, 3 );
			ambientPass.addSampledView( d4v, 4 );
			ambientPass.addSampledView( dfv, 5 );
			ambientPass.addSampledView( spv, 6 );
			ambientPass.addOutputColourView( ofv );
			return { ofv, &ambientPass };
		}
	}
    
	std::pair< crg::ImageViewId, crg::FramePass * > buildWeightedBlended( test::TestCounts & testCounts
		, crg::FramePass const * previous
		, crg::ImageViewId const & dsv
		, crg::ImageViewId const & dtv
		, crg::ImageViewId const & vtv
		, crg::FrameGraph & graph )
	{
		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto r = graph.createImage( test::createImage( "r", VK_FORMAT_R16_SFLOAT ) );
		auto rv = graph.createView( test::createView( "rv", r, VK_FORMAT_R16_SFLOAT ) );
		auto & accumulationPass = graph.createPass( "accumulationPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );

		if ( previous )
		{
			accumulationPass.addDependency( *previous );
		}

		accumulationPass.addInOutColourView( av );
		accumulationPass.addInOutColourView( rv );
		accumulationPass.addInOutColourView( vtv );
		accumulationPass.addOutputDepthStencilView( dtv );

		auto c = graph.createImage( test::createImage( "c", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( "cv", c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & combinePass = graph.createPass( "combinePass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		combinePass.addDependency( accumulationPass );
		combinePass.addSampledView( dsv, 0u );
		combinePass.addSampledView( av, 1u );
		combinePass.addSampledView( rv, 2u );
		combinePass.addOutputColourView( cv );

		return { cv, &combinePass };
	}

	template< bool EnableDepthPrepass
		, bool EnableOpaque
		, bool EnableSsao
		, bool EnableTransparent >
	void testRender( test::TestCounts & testCounts )
	{
		testBegin( "testRender"
			+ ( EnableDepthPrepass ? std::string{ "Prepass" } : std::string{} )
			+ ( EnableOpaque ? std::string{ "Opaque" } : std::string{} )
			+ ( EnableSsao ? std::string{ "Ssao" } : std::string{} )
			+ ( EnableTransparent ? std::string{ "Transparent" } : std::string{} ) );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( "dtv", d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		crg::FramePass * previous{};

		if constexpr ( EnableDepthPrepass )
		{
			auto & depthPrepass = graph.createPass( "depthPrepass"
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			depthPrepass.addOutputDepthView( dtv );
			previous = &depthPrepass;
		}

		auto dsv = graph.createView( test::createView( "dsv", d, VK_FORMAT_R32_SFLOAT ) );
		auto o = graph.createImage( test::createImage( "o", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto otv = graph.createView( test::createView( "otv", o, VK_FORMAT_R16G16B16A16_SFLOAT ) );

		if constexpr ( EnableOpaque )
		{
			auto v = graph.createImage( test::createImage( "v", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( "vv", v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto dcs = buildDeferred< EnableSsao >( testCounts
				, previous
				, dsv
				, dtv
				, vv
				, graph );

			if constexpr ( EnableTransparent )
			{
				auto wbcsv = buildWeightedBlended( testCounts
					, dcs.second
					, dsv
					, dtv
					, vv
					, graph );
				auto & finalCombinePass = graph.createPass( "finalCombinePass"
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkDummy );
					} );
				finalCombinePass.addDependency( *wbcsv.second );
				finalCombinePass.addSampledView( dcs.first, 0 );
				finalCombinePass.addSampledView( wbcsv.first, 1 );
				finalCombinePass.addSampledView( vv, 2 );
				finalCombinePass.addOutputColourView( otv );
			}
			else
			{
				auto & finalCombinePass = graph.createPass( "finalCombinePass"
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkDummy );
					} );
				finalCombinePass.addDependency( *dcs.second );
				finalCombinePass.addSampledView( dcs.first, 0 );
				finalCombinePass.addSampledView( vv, 1 );
				finalCombinePass.addOutputColourView( otv );
			}
			
		}
		else if constexpr ( EnableTransparent )
		{
			auto v = graph.createImage( test::createImage( "v", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( "vv", v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto wbcsv = buildWeightedBlended( testCounts
				, previous
				, dsv
				, dtv
				, vv
				, graph );
			auto & finalCombinePass = graph.createPass( "finalCombinePass"
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			finalCombinePass.addDependency( *wbcsv.second );
			finalCombinePass.addSampledView( wbcsv.first, 0 );
			finalCombinePass.addSampledView( vv, 1 );
			finalCombinePass.addOutputColourView( otv );
		}
		else
		{
			auto & finalCombinePass = graph.createPass( "finalCombinePass"
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );

			if ( previous )
			{
				finalCombinePass.addDependency( *previous );
			}

			finalCombinePass.addSampledView( dsv, 0 );
			finalCombinePass.addOutputColourView( otv );
		}
		

		auto runnable = graph.compile( getContext() );
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		std::string ref;

		if constexpr ( EnableDepthPrepass )
		{
			if constexpr ( EnableOpaque )
			{
				if constexpr ( EnableSsao )
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nssaoLinearisePassdsvSpl" [ shape=box ];
    "ssaoLinearisePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nssaoLinearisePassdsvSpl" [ label="dtv" ];
    "Transition to\nssaoLinearisePassdsvSpl" -> "ssaoLinearisePass" [ label="dtv" ];
    "Transition to\nssaoMinifyPass1m0vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1m0vSpl" [ label="m0v" ];
    "Transition to\nssaoMinifyPass1m0vSpl" -> "ssaoMinifyPass1" [ label="m0v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
    "Transition to\nssaoRawPassm3vSpl" [ shape=box ];
    "ssaoRawPass" [ shape=ellipse ];
    "ssaoMinifyPass3" -> "Transition to\nssaoRawPassm3vSpl" [ label="m3v" ];
    "Transition to\nssaoRawPassm3vSpl" -> "ssaoRawPass" [ label="m3v" ];
    "Transition to\nssaoBlurPassrsvSpl" [ shape=box ];
    "ssaoBlurPass" [ shape=ellipse ];
    "ssaoRawPass" -> "Transition to\nssaoBlurPassrsvSpl" [ label="rsv" ];
    "Transition to\nssaoBlurPassrsvSpl" -> "ssaoBlurPass" [ label="rsv" ];
    "Transition to\nambientPassb1vSpl" [ shape=box ];
    "ssaoBlurPass" -> "Transition to\nambientPassb1vSpl" [ label="b1v" ];
    "Transition to\nambientPassb1vSpl" -> "ambientPass" [ label="b1v" ];
    "Transition to\nambientPassd1vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd1vSpl" [ label="d1v" ];
    "Transition to\nambientPassd1vSpl" -> "ambientPass" [ label="d1v" ];
    "Transition to\nambientPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd2vSpl" [ label="d2v" ];
    "Transition to\nambientPassd2vSpl" -> "ambientPass" [ label="d2v" ];
    "Transition to\nambientPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd3vSpl" [ label="d3v" ];
    "Transition to\nambientPassd3vSpl" -> "ambientPass" [ label="d3v" ];
    "Transition to\nambientPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd4vSpl" [ label="d4v" ];
    "Transition to\nambientPassd4vSpl" -> "ambientPass" [ label="d4v" ];
    "Transition to\nambientPassdsvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassdsvSpl" [ label="dtv" ];
    "Transition to\nambientPassdsvSpl" -> "ambientPass" [ label="dtv" ];
    "Transition to\naccumulationPassvvIOc" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\naccumulationPassvvIOc" [ label="vv" ];
    "Transition to\naccumulationPassvvIOc" -> "accumulationPass" [ label="vv" ];
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nssaoLinearisePassdsvSpl" [ shape=box ];
    "ssaoLinearisePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nssaoLinearisePassdsvSpl" [ label="dtv" ];
    "Transition to\nssaoLinearisePassdsvSpl" -> "ssaoLinearisePass" [ label="dtv" ];
    "Transition to\nssaoMinifyPass1m0vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1m0vSpl" [ label="m0v" ];
    "Transition to\nssaoMinifyPass1m0vSpl" -> "ssaoMinifyPass1" [ label="m0v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
    "Transition to\nssaoRawPassm3vSpl" [ shape=box ];
    "ssaoRawPass" [ shape=ellipse ];
    "ssaoMinifyPass3" -> "Transition to\nssaoRawPassm3vSpl" [ label="m3v" ];
    "Transition to\nssaoRawPassm3vSpl" -> "ssaoRawPass" [ label="m3v" ];
    "Transition to\nssaoBlurPassrsvSpl" [ shape=box ];
    "ssaoBlurPass" [ shape=ellipse ];
    "ssaoRawPass" -> "Transition to\nssaoBlurPassrsvSpl" [ label="rsv" ];
    "Transition to\nssaoBlurPassrsvSpl" -> "ssaoBlurPass" [ label="rsv" ];
    "Transition to\nambientPassb1vSpl" [ shape=box ];
    "ssaoBlurPass" -> "Transition to\nambientPassb1vSpl" [ label="b1v" ];
    "Transition to\nambientPassb1vSpl" -> "ambientPass" [ label="b1v" ];
    "Transition to\nambientPassd1vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd1vSpl" [ label="d1v" ];
    "Transition to\nambientPassd1vSpl" -> "ambientPass" [ label="d1v" ];
    "Transition to\nambientPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd2vSpl" [ label="d2v" ];
    "Transition to\nambientPassd2vSpl" -> "ambientPass" [ label="d2v" ];
    "Transition to\nambientPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd3vSpl" [ label="d3v" ];
    "Transition to\nambientPassd3vSpl" -> "ambientPass" [ label="d3v" ];
    "Transition to\nambientPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassd4vSpl" [ label="d4v" ];
    "Transition to\nambientPassd4vSpl" -> "ambientPass" [ label="d4v" ];
    "Transition to\nambientPassdsvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nambientPassdsvSpl" [ label="dtv" ];
    "Transition to\nambientPassdsvSpl" -> "ambientPass" [ label="dtv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\naccumulationPassvvIOc" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\naccumulationPassvvIOc" [ label="vv" ];
    "Transition to\naccumulationPassvvIOc" -> "accumulationPass" [ label="vv" ];
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
			}
			else
			{
				if constexpr ( EnableTransparent )
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
				else
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nfinalCombinePassdsvSpl" [ shape=box ];
    "depthPrepass" [ shape=ellipse ];
    "finalCombinePass" [ shape=ellipse ];
    "depthPrepass" -> "Transition to\nfinalCombinePassdsvSpl" [ label="dtv" ];
    "Transition to\nfinalCombinePassdsvSpl" -> "finalCombinePass" [ label="dtv" ];
}
)";
				}
			}
		}
		else
		{
			if constexpr ( EnableOpaque )
			{
				if constexpr ( EnableSsao )
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nssaoLinearisePassdsvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ssaoLinearisePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nssaoLinearisePassdsvSpl" [ label="dtv" ];
    "Transition to\nssaoLinearisePassdsvSpl" -> "ssaoLinearisePass" [ label="dtv" ];
    "Transition to\nssaoMinifyPass1m0vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1m0vSpl" [ label="m0v" ];
    "Transition to\nssaoMinifyPass1m0vSpl" -> "ssaoMinifyPass1" [ label="m0v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
    "Transition to\nssaoRawPassm3vSpl" [ shape=box ];
    "ssaoRawPass" [ shape=ellipse ];
    "ssaoMinifyPass3" -> "Transition to\nssaoRawPassm3vSpl" [ label="m3v" ];
    "Transition to\nssaoRawPassm3vSpl" -> "ssaoRawPass" [ label="m3v" ];
    "Transition to\nssaoBlurPassrsvSpl" [ shape=box ];
    "ssaoBlurPass" [ shape=ellipse ];
    "ssaoRawPass" -> "Transition to\nssaoBlurPassrsvSpl" [ label="rsv" ];
    "Transition to\nssaoBlurPassrsvSpl" -> "ssaoBlurPass" [ label="rsv" ];
    "Transition to\nambientPassb1vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "ssaoBlurPass" -> "Transition to\nambientPassb1vSpl" [ label="b1v" ];
    "Transition to\nambientPassb1vSpl" -> "ambientPass" [ label="b1v" ];
    "Transition to\nambientPassd1vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd1vSpl" [ label="d1v" ];
    "Transition to\nambientPassd1vSpl" -> "ambientPass" [ label="d1v" ];
    "Transition to\nambientPassd2vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd2vSpl" [ label="d2v" ];
    "Transition to\nambientPassd2vSpl" -> "ambientPass" [ label="d2v" ];
    "Transition to\nambientPassd3vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd3vSpl" [ label="d3v" ];
    "Transition to\nambientPassd3vSpl" -> "ambientPass" [ label="d3v" ];
    "Transition to\nambientPassd4vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd4vSpl" [ label="d4v" ];
    "Transition to\nambientPassd4vSpl" -> "ambientPass" [ label="d4v" ];
    "Transition to\nambientPassdsvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassdsvSpl" [ label="dtv" ];
    "Transition to\nambientPassdsvSpl" -> "ambientPass" [ label="dtv" ];
    "Transition to\naccumulationPassvvIOc" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\naccumulationPassvvIOc" [ label="vv" ];
    "Transition to\naccumulationPassvvIOc" -> "accumulationPass" [ label="vv" ];
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nssaoLinearisePassdsvSpl" [ shape=box ];
    "ssaoLinearisePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nssaoLinearisePassdsvSpl" [ label="dtv" ];
    "Transition to\nssaoLinearisePassdsvSpl" -> "ssaoLinearisePass" [ label="dtv" ];
    "Transition to\nssaoMinifyPass1m0vSpl" [ shape=box ];
    "ssaoMinifyPass1" [ shape=ellipse ];
    "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1m0vSpl" [ label="m0v" ];
    "Transition to\nssaoMinifyPass1m0vSpl" -> "ssaoMinifyPass1" [ label="m0v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" [ shape=box ];
    "ssaoMinifyPass2" [ shape=ellipse ];
    "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2m1vSpl" [ label="m1v" ];
    "Transition to\nssaoMinifyPass2m1vSpl" -> "ssaoMinifyPass2" [ label="m1v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" [ shape=box ];
    "ssaoMinifyPass3" [ shape=ellipse ];
    "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3m2vSpl" [ label="m2v" ];
    "Transition to\nssaoMinifyPass3m2vSpl" -> "ssaoMinifyPass3" [ label="m2v" ];
    "Transition to\nssaoRawPassm3vSpl" [ shape=box ];
    "ssaoRawPass" [ shape=ellipse ];
    "ssaoMinifyPass3" -> "Transition to\nssaoRawPassm3vSpl" [ label="m3v" ];
    "Transition to\nssaoRawPassm3vSpl" -> "ssaoRawPass" [ label="m3v" ];
    "Transition to\nssaoBlurPassrsvSpl" [ shape=box ];
    "ssaoBlurPass" [ shape=ellipse ];
    "ssaoRawPass" -> "Transition to\nssaoBlurPassrsvSpl" [ label="rsv" ];
    "Transition to\nssaoBlurPassrsvSpl" -> "ssaoBlurPass" [ label="rsv" ];
    "Transition to\nambientPassb1vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "ssaoBlurPass" -> "Transition to\nambientPassb1vSpl" [ label="b1v" ];
    "Transition to\nambientPassb1vSpl" -> "ambientPass" [ label="b1v" ];
    "Transition to\nambientPassd1vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd1vSpl" [ label="d1v" ];
    "Transition to\nambientPassd1vSpl" -> "ambientPass" [ label="d1v" ];
    "Transition to\nambientPassd2vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd2vSpl" [ label="d2v" ];
    "Transition to\nambientPassd2vSpl" -> "ambientPass" [ label="d2v" ];
    "Transition to\nambientPassd3vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd3vSpl" [ label="d3v" ];
    "Transition to\nambientPassd3vSpl" -> "ambientPass" [ label="d3v" ];
    "Transition to\nambientPassd4vSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassd4vSpl" [ label="d4v" ];
    "Transition to\nambientPassd4vSpl" -> "ambientPass" [ label="d4v" ];
    "Transition to\nambientPassdsvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nambientPassdsvSpl" [ label="dtv" ];
    "Transition to\nambientPassdsvSpl" -> "ambientPass" [ label="dtv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\naccumulationPassvvIOc" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\naccumulationPassvvIOc" [ label="vv" ];
    "Transition to\naccumulationPassvvIOc" -> "accumulationPass" [ label="vv" ];
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\nlightingPassd1vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "lightingPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd1vSpl" [ label="d1v" ];
    "Transition to\nlightingPassd1vSpl" -> "lightingPass" [ label="d1v" ];
    "Transition to\nlightingPassd2vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd2vSpl" [ label="d2v" ];
    "Transition to\nlightingPassd2vSpl" -> "lightingPass" [ label="d2v" ];
    "Transition to\nlightingPassd3vSpl" [ shape=box ];
    "geometryPass" -> "Transition to\nlightingPassd3vSpl" [ label="d3v" ];
    "Transition to\nlightingPassd3vSpl" -> "lightingPass" [ label="d3v" ];
    "Transition to\nlightingPassd4vSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassd4vSpl" [ label="d4v" ];
    "Transition to\nlightingPassd4vSpl" -> "lightingPass" [ label="d4v" ];
    "Transition to\nlightingPassdtvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nlightingPassdtvSpl" [ label="dtv" ];
    "Transition to\nlightingPassdtvSpl" -> "lightingPass" [ label="dtv" ];
    "Transition to\nambientPassdfvSpl" [ shape=box ];
    "ambientPass" [ shape=ellipse ];
    "lightingPass" -> "Transition to\nambientPassdfvSpl" [ label="dfv" ];
    "Transition to\nambientPassdfvSpl" -> "ambientPass" [ label="dfv" ];
    "Transition to\nambientPassspvSpl" [ shape=box ];
    "lightingPass" -> "Transition to\nambientPassspvSpl" [ label="spv" ];
    "Transition to\nambientPassspvSpl" -> "ambientPass" [ label="spv" ];
    "Transition to\nfinalCombinePassofvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "ambientPass" -> "Transition to\nfinalCombinePassofvSpl" [ label="ofv" ];
    "Transition to\nfinalCombinePassofvSpl" -> "finalCombinePass" [ label="ofv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "geometryPass" [ shape=ellipse ];
    "geometryPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
			}
			else
			{
				if constexpr ( EnableTransparent )
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Transition to\ncombinePassavSpl" [ shape=box ];
    "accumulationPass" [ shape=ellipse ];
    "combinePass" [ shape=ellipse ];
    "accumulationPass" -> "Transition to\ncombinePassavSpl" [ label="av" ];
    "Transition to\ncombinePassavSpl" -> "combinePass" [ label="av" ];
    "Transition to\ncombinePassdsvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassdsvSpl" [ label="dtv" ];
    "Transition to\ncombinePassdsvSpl" -> "combinePass" [ label="dtv" ];
    "Transition to\ncombinePassrvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\ncombinePassrvSpl" [ label="rv" ];
    "Transition to\ncombinePassrvSpl" -> "combinePass" [ label="rv" ];
    "Transition to\nfinalCombinePasscvSpl" [ shape=box ];
    "finalCombinePass" [ shape=ellipse ];
    "combinePass" -> "Transition to\nfinalCombinePasscvSpl" [ label="cv" ];
    "Transition to\nfinalCombinePasscvSpl" -> "finalCombinePass" [ label="cv" ];
    "Transition to\nfinalCombinePassvvSpl" [ shape=box ];
    "accumulationPass" -> "Transition to\nfinalCombinePassvvSpl" [ label="vv" ];
    "Transition to\nfinalCombinePassvvSpl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
				else
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
}
)";
				}
			}
		}

		checkEqualSortedLines( stream.str(), ref );
		testEnd();
	}

	void testVarianceShadowMap( test::TestCounts & testCounts )
	{
		testBegin( "testVarianceShadowMap" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto & depthPrepass = graph.createPass( "depthPrepass"
			, [&testCounts]( crg::FramePass const & pass
			, crg::GraphContext & context
			, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		depthPrepass.addOutputDepthView( depthv );
		auto previous = &depthPrepass;

		auto colour = graph.createImage( test::createImage( "colour", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto colourv = graph.createView( test::createView( "colourv", colour, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto & backgroundPass = graph.createPass( "backgroundPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		backgroundPass.addDependency( *previous );
		previous = &backgroundPass;
		backgroundPass.addInOutDepthView( depthv );
		backgroundPass.addOutputColourView( colourv );

		auto dirShadowMap = graph.createImage( test::createImage( "dirShadowMap", VK_FORMAT_X8_D24_UNORM_PACK32, 1u, 4u ) );
		auto dirVarianceMap = graph.createImage( test::createImage( "dirVarianceMap", VK_FORMAT_R32G32_SFLOAT, 1u, 4u ) );
		auto dirShadowMapv = graph.createView( test::createView( "dirShadowMapv", dirShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, 0u, 4u ) );
		auto dirVarianceMapv = graph.createView( test::createView( "dirVarianceMapv", dirVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, 0u, 4u ) );
		crg::ImageViewIdArray dirShadows;
		crg::ImageViewIdArray dirVariances;
		{
			auto dirIntermediate = graph.createImage( test::createImage( "dirIntermediate", VK_FORMAT_R32G32_SFLOAT ) );
			auto dirIntermediatev = graph.createView( test::createView( "dirIntermediatev", dirIntermediate, VK_FORMAT_R32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 4u; ++index )
			{
				auto shadowMapv = graph.createView( test::createView( "dirShadowMapv" + std::to_string( index ), dirShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, index ) );
				dirShadows.push_back( shadowMapv );
				auto varianceMapv = graph.createView( test::createView( "dirVarianceMapv" + std::to_string( index ), dirVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, index ) );
				dirVariances.push_back( varianceMapv );
				auto & shadowPass = graph.createPass( "dirShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				shadowPass.addDependency( *previous );
				previous = &shadowPass;
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = graph.createPass( "dirBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addSampledView( varianceMapv, 0u, {} );
				blurPassX.addOutputColourView( dirIntermediatev );

				auto & blurPassY = graph.createPass( "dirBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addSampledView( dirIntermediatev, 0u, {} );
				blurPassY.addOutputColourView( varianceMapv );
			}
		}
		auto pntShadowMap = graph.createImage( test::createImage( "pntShadowMap", VK_FORMAT_X8_D24_UNORM_PACK32, 1u, 36u ) );
		auto pntVarianceMap = graph.createImage( test::createImage( "pntVarianceMap", VK_FORMAT_R32G32_SFLOAT, 1u, 36u ) );
		auto pntShadowMapv = graph.createView( test::createView( "pntShadowMapv", pntShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, 0u, 36u ) );
		auto pntVarianceMapv = graph.createView( test::createView( "pntVarianceMapv", pntVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, 0u, 36u ) );
		crg::ImageViewIdArray pntShadows;
		crg::ImageViewIdArray pntVariances;
		{
			auto pntIntermediate = graph.createImage( test::createImage( "pntIntermediate", VK_FORMAT_R32G32_SFLOAT ) );
			auto pntIntermediatev = graph.createView( test::createView( "pntIntermediatev", pntIntermediate, VK_FORMAT_R32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 36u; ++index )
			{
				auto shadowMapv = graph.createView( test::createView( "pntShadowMapv" + std::to_string( index ), pntShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, index ) );
				pntShadows.push_back( shadowMapv );
				auto varianceMapv = graph.createView( test::createView( "pntVarianceMapv" + std::to_string( index ), pntVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, index ) );
				pntVariances.push_back( varianceMapv );
				auto & shadowPass = graph.createPass( "pntShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				shadowPass.addDependency( *previous );
				previous = &shadowPass;
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = graph.createPass( "pntBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addSampledView( varianceMapv, 0u, {} );
				blurPassX.addOutputColourView( pntIntermediatev );

				auto & blurPassY = graph.createPass( "pntBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addSampledView( pntIntermediatev, 0u, {} );
				blurPassY.addOutputColourView( varianceMapv );
			}
		}
		auto sptShadowMap = graph.createImage( test::createImage( "sptShadowMap", VK_FORMAT_X8_D24_UNORM_PACK32, 1u, 10u ) );
		auto sptVarianceMap = graph.createImage( test::createImage( "pntVarianceMap", VK_FORMAT_R32G32_SFLOAT, 1u, 10u ) );
		auto sptShadowMapv = graph.createView( test::createView( "sptShadowMapv", sptShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, 0u, 10u ) );
		auto sptVarianceMapv = graph.createView( test::createView( "sptVarianceMapv", sptVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, 0u, 10u ) );
		crg::ImageViewIdArray sptShadows;
		crg::ImageViewIdArray sptVariances;
		{
			auto sptIntermediate = graph.createImage( test::createImage( "sptIntermediate", VK_FORMAT_R32G32_SFLOAT ) );
			auto sptIntermediatev = graph.createView( test::createView( "sptIntermediatev", sptIntermediate, VK_FORMAT_R32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 10u; ++index )
			{
				auto shadowMapv = graph.createView( test::createView( "sptShadowMapv" + std::to_string( index ), sptShadowMap, VK_FORMAT_X8_D24_UNORM_PACK32, 0u, 1u, index ) );
				sptShadows.push_back( shadowMapv );
				auto varianceMapv = graph.createView( test::createView( "sptVarianceMapv" + std::to_string( index ), sptVarianceMap, VK_FORMAT_R32G32_SFLOAT, 0u, 1u, index ) );
				sptVariances.push_back( varianceMapv );
				auto & shadowPass = graph.createPass( "sptShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				shadowPass.addDependency( *previous );
				previous = &shadowPass;
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = graph.createPass( "sptBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addSampledView( varianceMapv, 0u, {} );
				blurPassX.addOutputColourView( sptIntermediatev );

				auto & blurPassY = graph.createPass( "sptBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & pass
						, crg::GraphContext & context
						, crg::RunnableGraph & graph )
					{
						return createDummy( testCounts
							, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							, checkOutputColourIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addSampledView( sptIntermediatev, 0u, {} );
				blurPassY.addOutputColourView( varianceMapv );
			}
		}

		auto & opaquePass = graph.createPass( "opaquePass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkSampledIsShaderReadOnly );
			} );
		opaquePass.addDependency( *previous );
		previous = &opaquePass;
		opaquePass.addSampledView( dirShadowMapv, 0u );
		opaquePass.addSampledView( dirVarianceMapv, 1u );
		opaquePass.addSampledView( opaquePass.mergeViews( dirShadows ), 0u );
		opaquePass.addSampledView( opaquePass.mergeViews( dirVariances ), 1u );
		opaquePass.addSampledView( pntShadowMapv, 2u );
		opaquePass.addSampledView( pntVarianceMapv, 3u );
		opaquePass.addSampledView( opaquePass.mergeViews( pntShadows ), 2u );
		opaquePass.addSampledView( opaquePass.mergeViews( pntVariances ), 3u );
		opaquePass.addSampledView( sptShadowMapv, 4u );
		opaquePass.addSampledView( sptVarianceMapv, 5u );
		opaquePass.addSampledView( opaquePass.mergeViews( sptShadows ), 4u );
		opaquePass.addSampledView( opaquePass.mergeViews( sptVariances ), 5u );
		opaquePass.addInOutDepthView( depthv );
		opaquePass.addInOutColourView( colourv );

		auto & transparentPass = graph.createPass( "transparentPass"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
					, checkDummy );
			} );
		transparentPass.addDependency( *previous );
		previous = &transparentPass;
		transparentPass.addInOutDepthView( depthv );
		transparentPass.addInOutColourView( colourv );

		auto runnable = graph.compile( getContext() );
		testEnd();
	}

	void testEnvironmentMap( test::TestCounts & testCounts )
	{
		testBegin( "testEnvironmentMap" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT_S8_UINT, 1u, 6u ) );
		auto colour = graph.createImage( test::createImage( "colour", VK_FORMAT_R16G16B16A16_SFLOAT, 8u, 6u ) );
		auto colourv = graph.createView( test::createView( "colourv", colour, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 8u, 0u, 6u ) );
		crg::ImageViewIdArray colourViews;
		std::vector< crg::FramePass const * > previouses{};
		crg::FramePass const * previous{};

		for ( auto index = 0u; index < 6u; ++index )
		{
			auto strIndex = std::to_string( index );
			auto colourvn = graph.createView( test::createView( "colourv" + strIndex, colour, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, index, 1u ) );
			auto depthvn = graph.createView( test::createView( "depthv" + strIndex, depth, VK_FORMAT_D32_SFLOAT_S8_UINT, 0u, 1u, index, 1u ) );
			auto & opaquePass = graph.createPass( "EnvOpaquePass" + strIndex
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			colourViews.push_back( colourvn );
			previous = &opaquePass;
			opaquePass.addOutputDepthView( depthvn );
			opaquePass.addOutputColourView( colourvn );

			auto & backgroundPass = graph.createPass( "EnvBackgroundPass" + strIndex
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			backgroundPass.addDependency( *previous );
			previous = &backgroundPass;
			backgroundPass.addInOutDepthView( depthvn );
			backgroundPass.addInOutColourView( colourvn );

			auto & transparentPass = graph.createPass( "EnvTransparentPass" + strIndex
				, [&testCounts]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & graph )
				{
					return createDummy( testCounts
						, pass, context, graph, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
						, checkDummy );
				} );
			transparentPass.addDependency( *previous );
			previouses.push_back( &transparentPass );
			transparentPass.addInputDepthView( depthvn );
			transparentPass.addInOutColourView( colourvn );
		}

		auto & mipsGen = graph.createPass( "EnvMips"
			, [&testCounts]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				return createDummy( testCounts
					, pass, context, graph, VK_PIPELINE_STAGE_TRANSFER_BIT
					, checkDummy );
			} );
		mipsGen.addDependencies( previouses );
		mipsGen.addTransferInputView( mipsGen.mergeViews( colourViews ) );
		mipsGen.addTransferOutputView( colourv );

		auto runnable = graph.compile( getContext() );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRenderGraph" );
	testNoPass( testCounts );
	testOnePass( testCounts );
	testDuplicateName( testCounts );
	testOneDependency( testCounts );
	testChainedDependencies( testCounts );
	testSharedDependencies( testCounts );
	test2MipDependencies( testCounts );
	test3MipDependencies( testCounts );
	testLoopDependencies( testCounts );
	testLoopDependenciesWithRoot( testCounts );
	testLoopDependenciesWithRootAndLeaf( testCounts );
	testSsaoPass( testCounts );
	testBloomPostEffect( testCounts );
	testRender< false, false, false, false >( testCounts );
	testRender< false, true, false, false >( testCounts );
	testRender< false, true, true, false >( testCounts );
	testRender< false, false, false, true >( testCounts );
	testRender< false, true, false, true >( testCounts );
	testRender< false, true, true, true >( testCounts );
	testRender< true, false, false, false >( testCounts );
	testRender< true, true, false, false >( testCounts );
	testRender< true, true, true, false >( testCounts );
	testRender< true, false, false, true >( testCounts );
	testRender< true, true, false, true >( testCounts );
	testRender< true, true, true, true >( testCounts );
	testVarianceShadowMap( testCounts );
	testEnvironmentMap( testCounts );
	testSuiteEnd();
}
