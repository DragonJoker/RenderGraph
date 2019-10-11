#include "Common.hpp"

#include <RenderGraph/RenderGraph.hpp>
#include <RenderGraph/ImageData.hpp>

#include <sstream>

namespace
{
	std::string sort( std::string const & value )
	{
		std::stringstream stream{ value };
		std::multiset< std::string > sorted;
		std::string line;

		while ( std::getline( stream, line ) )
		{
			sorted.insert( line );
		}

		std::stringstream result;

		for ( auto & v : sorted )
		{
			result << v << std::endl;
		}

		return result.str();
	}

	void testNoPass( test::TestCounts & testCounts )
	{
		testBegin( "testNoPass" );
		crg::RenderGraph graph{ testCounts.testName };
		checkThrow( graph.compile() );

		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createOutputColour( "RT"
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};

		checkNoThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		checkThrow( graph.compile() );
		testEnd();
	}

	void testOnePass( test::TestCounts & testCounts )
	{
		testBegin( "testOnePass" );
		crg::RenderGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createOutputColour( "RT"
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};

		checkNoThrow( graph.add( pass ) );
		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testDuplicateName( test::TestCounts & testCounts )
	{
		testBegin( "testDuplicateName" );
		crg::RenderGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createOutputColour( "RT"
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass ) );
		checkThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		testEnd();
	}

	void testWrongRemove( test::TestCounts & testCounts )
	{
		testBegin( "testWrongRemove" );
		crg::RenderGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createOutputColour( "RT"
			, rtv );
		crg::RenderPass pass1
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		crg::RenderPass pass2
		{
			"pass2C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass1 ) );
		checkThrow( graph.remove( pass2 ) );
		checkNoThrow( graph.remove( pass1 ) );
		testEnd();
	}

	void testOneDependency( test::TestCounts & testCounts )
	{
		testBegin( "testOneDependency" );
		crg::RenderGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createOutputColour( "RT"
			, rtv );
		crg::RenderPass pass1
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto inAttach = crg::Attachment::createSampled( "IN"
			, rtv );
		auto out = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( out, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outAttach = crg::Attachment::createOutputColour( "OUT"
			, outv );
		crg::RenderPass pass2
		{
			"pass2C",
			{ inAttach },
			{ outAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "RT\nto\nIN" [ shape=square ];
    "pass1C" -> "RT\nto\nIN" [ label="RT" ];
    "RT\nto\nIN" -> "pass2C" [ label="IN" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testChainedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testChainedDependencies" );
		crg::RenderGraph graph{ testCounts.testName };
		auto d0 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( d0, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d0tAttach = crg::Attachment::createOutputColour( "D0Tg"
			, d0v );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ d0tAttach },
		};
		checkNoThrow( graph.add( pass0 ) );

		auto d0sAttach = crg::Attachment::createSampled( "D0Sp"
			, d0v );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d1tAttach = crg::Attachment::createOutputColour( "D1Tg"
			, d1v );
		crg::RenderPass pass1
		{
			"pass1",
			{ d0sAttach },
			{ d1tAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto d1sAttach = crg::Attachment::createSampled( "D1Sp"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d2tAttach = crg::Attachment::createOutputColour( "D2Tg"
			, d2v );
		crg::RenderPass pass2
		{
			"pass2",
			{ d1sAttach },
			{ d2tAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "D0Tg\nto\nD0Sp" [ shape=square ];
    "pass0" -> "D0Tg\nto\nD0Sp" [ label="D0Tg" ];
    "D0Tg\nto\nD0Sp" -> "pass1" [ label="D0Sp" ];
    "D1Tg\nto\nD1Sp" [ shape=square ];
    "pass1" -> "D1Tg\nto\nD1Sp" [ label="D1Tg" ];
    "D1Tg\nto\nD1Sp" -> "pass2" [ label="D1Sp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testSharedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testSharedDependencies" );
		crg::RenderGraph graph{ testCounts.testName };
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dstv1 = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT, 1u ) );
		auto dstAttach1 = crg::Attachment::createOutputDepth( "DSTarget1"
			, dstv1 );
		auto dstv2 = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT, 1u ) );
		auto dstAttach2 = crg::Attachment::createOutputDepth( "DSTarget2"
			, dstv2 );

		auto d0 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( d0, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d0tAttach = crg::Attachment::createOutputColour( "D0Tg"
			, d0v );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ d0tAttach },
			{ dstAttach1 },
		};
		checkNoThrow( graph.add( pass0 ) );

		auto d0sAttach = crg::Attachment::createSampled( "D0Sp"
			, d0v );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d1tAttach = crg::Attachment::createOutputColour( "D1Tg"
			, d1v );
		crg::RenderPass pass1
		{
			"pass1",
			{ d0sAttach },
			{ d1tAttach },
			{ dstAttach2 },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto d1sAttach = crg::Attachment::createSampled( "D1Sp"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d2tAttach = crg::Attachment::createOutputColour( "D2Tg"
			, d2v );
		crg::RenderPass pass2
		{
			"pass2",
			{ d1sAttach },
			{ d2tAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "D0Tg\nto\nD0Sp" [ shape=square ];
    "pass0" -> "D0Tg\nto\nD0Sp" [ label="D0Tg" ];
    "D0Tg\nto\nD0Sp" -> "pass1" [ label="D0Sp" ];
    "D1Tg\nto\nD1Sp" [ shape=square ];
    "pass1" -> "D1Tg\nto\nD1Sp" [ label="D1Tg" ];
    "D1Tg\nto\nD1Sp" -> "pass2" [ label="D1Sp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void test2MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test2MipDependencies" );
		crg::RenderGraph graph{ testCounts.testName };
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );

		auto m0sAttach = crg::Attachment::createSampled( "SSAOLinSp"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createOutputColour( "SSAOMin1Tg"
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createSampled( "SSAOMin1Sp"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createOutputColour( "SSAOMin2Tg"
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void test3MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test3MipDependencies" );
		crg::RenderGraph graph{ testCounts.testName };
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );

		auto m0sAttach = crg::Attachment::createSampled( "SSAOLinSp"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createOutputColour( "SSAOMin1Tg"
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createSampled( "SSAOMin1Sp"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createOutputColour( "SSAOMin2Tg"
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		auto m2sAttach = crg::Attachment::createSampled( "SSAOMin2Sp"
			, m2v );
		auto m3v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m3tAttach = crg::Attachment::createOutputColour( "SSAOMin3Tg"
			, m3v );
		crg::RenderPass ssaoMinifyPass3
		{
			"ssaoMinifyPass3",
			{ m2sAttach },
			{ m3tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass3 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testLoopDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependencies" );
		crg::RenderGraph graph{ testCounts.testName };
		auto a = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto atAttach = crg::Attachment::createOutputColour( "Img1Tg"
			, av );

		auto b = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bsAttach = crg::Attachment::createSampled( "Img2Sp"
			, bv );
		crg::RenderPass pass1
		{
			"ssaoMinifyPass1",
			{ bsAttach },
			{ atAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto asAttach = crg::Attachment::createSampled( "Img1Sp"
			, av );
		auto btAttach = crg::Attachment::createOutputColour( "Img2Tg"
			, bv );
		crg::RenderPass pass2
		{
			"pass2",
			{ asAttach },
			{ btAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		checkThrow( graph.compile() );
		testEnd();
	}

	void testLoopDependenciesWithRoot( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRoot" );
		crg::RenderGraph graph{ testCounts.testName };
		auto b = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto btAttach = crg::Attachment::createOutputColour( "Img2Tg"
			, bv );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ btAttach },
		};

		auto a = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto atAttach = crg::Attachment::createOutputColour( "Img1Tg"
			, av );
		auto bsAttach = crg::Attachment::createSampled( "Img2Sp"
			, bv );
		crg::RenderPass pass1
		{
			"pass1",
			{ bsAttach },
			{ atAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto asAttach = crg::Attachment::createSampled( "Img1Sp"
			, av );
		crg::RenderPass pass2
		{
			"pass2",
			{ asAttach },
			{ btAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		checkThrow( graph.compile() );
		testEnd();
	}

	void testLoopDependenciesWithRootAndLeaf( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRootAndLeaf" );
		crg::RenderGraph graph{ testCounts.testName };
		auto c = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ctAttach = crg::Attachment::createOutputColour( "Img0Tg"
			, cv );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ ctAttach },
		};
		checkNoThrow( graph.add( pass0 ) );

		auto a = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto atAttach = crg::Attachment::createOutputColour( "Img1Tg"
			, av );
		auto b = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bsAttach = crg::Attachment::createSampled( "Img2Sp"
			, bv );
		auto csAttach = crg::Attachment::createSampled( "Img0Sp"
			, cv );
		crg::RenderPass pass1
		{
			"pass1",
			{ bsAttach, csAttach },
			{ atAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto asAttach = crg::Attachment::createSampled( "Img1Sp"
			, av );
		auto btAttach = crg::Attachment::createOutputColour( "Img2Tg"
			, bv );
		crg::RenderPass pass2
		{
			"pass2",
			{ asAttach, csAttach },
			{ btAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		crg::RenderPass pass3
		{
			"pass3",
			{ csAttach },
			{},
		};
		checkNoThrow( graph.add( pass3 ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Img0Tg\nto\nImg0Sp" [ shape=square ];
    "pass0" -> "Img0Tg\nto\nImg0Sp" [ label="Img0Tg" ];
    "Img0Tg\nto\nImg0Sp" -> "pass1" [ label="Img0Sp" ];
    "Img1Tg\nto\nImg1Sp" [ shape=square ];
    "pass1" -> "Img1Tg\nto\nImg1Sp" [ label="Img1Tg" ];
    "Img1Tg\nto\nImg1Sp" -> "pass2" [ label="Img1Sp" ];
    "Img2Tg\nto\nImg2Sp" [ shape=square ];
    "pass2" -> "Img2Tg\nto\nImg2Sp" [ label="Img2Tg" ];
    "Img2Tg\nto\nImg2Sp" -> "pass1" [ label="Img2Sp" ];
    "Img0Tg\nto\nImg0Sp" [ shape=square ];
    "pass0" -> "Img0Tg\nto\nImg0Sp" [ label="Img0Tg" ];
    "Img0Tg\nto\nImg0Sp" -> "pass2" [ label="Img0Sp" ];
    "Img0Tg\nto\nImg0Sp" [ shape=square ];
    "pass0" -> "Img0Tg\nto\nImg0Sp" [ label="Img0Tg" ];
    "Img0Tg\nto\nImg0Sp" -> "pass3" [ label="Img0Sp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	crg::Attachment buildSsaoPass( test::TestCounts & testCounts
		, crg::RenderPass const & previous
		, crg::Attachment const & dsAttach
		, crg::Attachment const & d2sAttach
		, crg::RenderGraph & graph )
	{
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 0u ) );
		auto m0tAttach = crg::Attachment::createOutputColour( "SSAOLinTg"
			, m0v );
		crg::RenderPass ssaoLinearisePass
		{
			"ssaoLinearisePass",
			{ dsAttach },
			{ m0tAttach },
		};
		checkNoThrow( graph.add( ssaoLinearisePass ) );

		auto m0sAttach = crg::Attachment::createSampled( "SSAOMin0Sp"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createOutputColour( "SSAOMin1Tg"
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createSampled( "SSAOMin1Sp"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createOutputColour( "SSAOMin2Tg"
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		auto m2sAttach = crg::Attachment::createSampled( "SSAOMin2Sp"
			, m2v );
		auto m3v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 3u ) );
		auto m3tAttach = crg::Attachment::createOutputColour( "SSAOMin3Tg"
			, m3v );
		crg::RenderPass ssaoMinifyPass3
		{
			"ssaoMinifyPass3",
			{ m2sAttach },
			{ m3tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass3 ) );

		auto mv = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 0u, 4u ) );
		auto msAttach = crg::Attachment::createSampled( "SSAOMinSp"
			, mv );
		auto rs = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT ) );
		auto rsv = graph.createView( test::createView( rs, VK_FORMAT_R32_SFLOAT ) );
		auto rstAttach = crg::Attachment::createOutputColour( "SSAORawTg"
			, rsv );
		crg::RenderPass ssaoRawPass
		{
			"ssaoRawPass",
			{ msAttach, d2sAttach },
			{ rstAttach },
		};
		checkNoThrow( graph.add( ssaoRawPass ) );

		auto rssAttach = crg::Attachment::createSampled( "SSAORawSp"
			, rsv );
		auto bl = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT ) );
		auto blv = graph.createView( test::createView( bl, VK_FORMAT_R32_SFLOAT ) );
		auto bltAttach = crg::Attachment::createOutputColour( "SSAOBlurTg"
			, blv );
		crg::RenderPass ssaoBlurPass
		{
			"ssaoBlurPass",
			{ rssAttach, d2sAttach },
			{ bltAttach },
		};
		checkNoThrow( graph.add( ssaoBlurPass ) );

		return crg::Attachment::createSampled( "SSAOBlurSp"
			, blv );
	}

	void testSsaoPass( test::TestCounts & testCounts )
	{
		testBegin( "testSsaoPass" );
		crg::RenderGraph graph{ testCounts.testName };
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtAttach = crg::Attachment::createOutputDepth( "DepthTg"
			, dtv );
		auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto vv = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto vtAttach = crg::Attachment::createOutputColour( "VelocityTg"
			, vv );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1tAttach = crg::Attachment::createOutputColour( "Data1Tg"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2tAttach = crg::Attachment::createOutputColour( "Data2Tg"
			, d2v );
		auto d3 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3tAttach = crg::Attachment::createOutputColour( "Data3Tg"
			, d3v );
		auto d4 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4tAttach = crg::Attachment::createOutputColour( "Data4Tg"
			, d4v );
		crg::RenderPass geometryPass
		{
			"geometryPass",
			{},
			{ d1tAttach, d2tAttach, d3tAttach, d4tAttach, vtAttach },
			{ dtAttach },
		};
		checkNoThrow( graph.add( geometryPass ) );

		auto dsv = graph.createView( test::createView( d, VK_FORMAT_R32_SFLOAT ) );
		auto dsAttach = crg::Attachment::createSampled( "DepthSp"
			, dsv );
		auto d2sAttach = crg::Attachment::createSampled( "Data2Sp"
			, d2v );
		auto ssaosAttach = buildSsaoPass( testCounts
			, geometryPass
			, dsAttach
			, d2sAttach
			, graph );

		auto d1sAttach = crg::Attachment::createSampled( "Data1Sp"
			, d1v );
		auto d3sAttach = crg::Attachment::createSampled( "Data3Sp"
			, d3v );
		auto d4sAttach = crg::Attachment::createSampled( "Data4Sp"
			, d4v );
		auto of = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( of, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto oftAttach = crg::Attachment::createOutputColour( "OutputTg"
			, ofv );
		crg::RenderPass ambientPass
		{
			"ambientPass",
			{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, ssaosAttach },
			{ oftAttach },
		};
		checkNoThrow( graph.add( ambientPass ) );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "DepthTg\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg\nto\nDepthSp" [ label="DepthTg" ];
    "DepthTg\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoRawPass" [ label="Data2Sp" ];
    "SSAORawTg\nto\nSSAORawSp" [ shape=square ];
    "ssaoRawPass" -> "SSAORawTg\nto\nSSAORawSp" [ label="SSAORawTg" ];
    "SSAORawTg\nto\nSSAORawSp" -> "ssaoBlurPass" [ label="SSAORawSp" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" [ shape=square ];
    "ssaoBlurPass" -> "SSAOBlurTg\nto\nSSAOBlurSp" [ label="SSAOBlurTg" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" -> "ambientPass" [ label="SSAOBlurSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoBlurPass" [ label="Data2Sp" ];
    "DepthTg\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg\nto\nDepthSp" [ label="DepthTg" ];
    "DepthTg\nto\nDepthSp" -> "ssaoLinearisePass" [ label="DepthSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoMinifyPass1" [ label="SSAOMin0Sp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass3" -> "SSAOMin3Tg\nto\nSSAOMinSp" [ label="SSAOMin3Tg" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoRawPass" [ label="SSAOMin2Sp" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMinSp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoRawPass" [ label="SSAOMin1Sp" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMinSp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoRawPass" [ label="SSAOMin0Sp" ];
    "SSAOLinTg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMinSp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	template< bool EnableSsao >
	crg::Attachment buildDeferred( test::TestCounts & testCounts
		, crg::Attachment const & dsAttach
		, crg::Attachment const & dtAttach
		, crg::Attachment const & vtAttach
		, crg::RenderGraph & graph )
	{
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1tAttach = crg::Attachment::createOutputColour( "Data1Tg"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2tAttach = crg::Attachment::createOutputColour( "Data2Tg"
			, d2v );
		auto d3 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3tAttach = crg::Attachment::createOutputColour( "Data3Tg"
			, d3v );
		auto d4 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4tAttach = crg::Attachment::createOutputColour( "Data4Tg"
			, d4v );
		crg::RenderPass geometryPass
		{
			"geometryPass",
			{},
			{ d1tAttach, d2tAttach, d3tAttach, d4tAttach, vtAttach },
			{ dtAttach },
		};
		checkNoThrow( graph.add( geometryPass ) );

		auto d1sAttach = crg::Attachment::createSampled( "Data1Sp"
			, d1v );
		auto d2sAttach = crg::Attachment::createSampled( "Data2Sp"
			, d2v );
		auto d3sAttach = crg::Attachment::createSampled( "Data3Sp"
			, d3v );
		auto d4sAttach = crg::Attachment::createSampled( "Data4Sp"
			, d4v );
		auto df = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto dfv = graph.createView( test::createView( df, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto dftAttach = crg::Attachment::createOutputColour( "DiffuseTg"
			, dfv );
		auto sp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto spv = graph.createView( test::createView( sp, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto sptAttach = crg::Attachment::createOutputColour( "SpecularTg"
			, spv );
		crg::RenderPass lightingPass
		{
			"lightingPass",
			{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach },
			{ dftAttach, sptAttach },
		};
		checkNoThrow( graph.add( lightingPass ) );

		auto dfsAttach = crg::Attachment::createSampled( "DiffuseSp"
			, dfv );
		auto spsAttach = crg::Attachment::createSampled( "SpecularSp"
			, spv );
		auto of = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( of, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto oftAttach = crg::Attachment::createOutputColour( "OutputTg"
			, ofv );

		if constexpr ( EnableSsao )
		{
			auto ssaosAttach = buildSsaoPass( testCounts
				, geometryPass
				, dsAttach
				, d2sAttach
				, graph );
			crg::RenderPass ambientPass
			{
				"ambientPass",
				{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, dfsAttach, spsAttach, ssaosAttach },
				{ oftAttach },
			};
			checkNoThrow( graph.add( ambientPass ) );
		}
		else
		{
			crg::RenderPass ambientPass
			{
				"ambientPass",
				{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, dfsAttach, spsAttach },
				{ oftAttach },
			};
			checkNoThrow( graph.add( ambientPass ) );
		}

		return crg::Attachment::createSampled( "OutputSp"
			, ofv );
	}

	crg::Attachment buildWeightedBlended( test::TestCounts & testCounts
		, crg::Attachment const & dsAttach
		, crg::Attachment const & dtAttach
		, crg::Attachment const & vtAttach
		, crg::RenderGraph & graph )
	{
		auto a = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto av = graph.createView( test::createView( a, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto atAttach = crg::Attachment::createInOutColour( "AccumulationTg"
			, av );
		auto r = graph.createImage( test::createImage( VK_FORMAT_R16_SFLOAT ) );
		auto rv = graph.createView( test::createView( r, VK_FORMAT_R16_SFLOAT ) );
		auto rtAttach = crg::Attachment::createInOutColour( "RevealageTg"
			, rv );
		crg::RenderPass accumulationPass
		{
			"accumulationPass",
			{},
			{ atAttach, rtAttach, vtAttach },
			{ dtAttach },
		};
		checkNoThrow( graph.add( accumulationPass ) );

		auto asAttach = crg::Attachment::createSampled( "AccumulationSp"
			, av );
		auto rsAttach = crg::Attachment::createSampled( "RevealageSp"
			, rv );
		auto c = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ctAttach = crg::Attachment::createOutputColour( "CombineTg"
			, cv );
		crg::RenderPass combinePass
		{
			"combinePass",
			{ dsAttach, asAttach, rsAttach },
			{ ctAttach },
		};
		checkNoThrow( graph.add( combinePass ) );

		return crg::Attachment::createSampled( "CombineSp"
			, cv );
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
		crg::RenderGraph graph{ testCounts.testName };
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dt1Attach = crg::Attachment::createOutputDepth( "DepthTg1"
			, dtv );
		auto dt2Attach = crg::Attachment::createInputDepth( "DepthTg2"
			, dtv );
		auto opaqueDTAttach = &dt1Attach;

		if constexpr ( EnableDepthPrepass )
		{
			opaqueDTAttach = &dt2Attach;
			crg::RenderPass depthPrepass
			{
				"depthPrepass",
				{},
				{},
				{ dt1Attach },
			};
			checkNoThrow( graph.add( depthPrepass ) );
		}

		auto dsv = graph.createView( test::createView( d, VK_FORMAT_R32_SFLOAT ) );
		auto dsAttach = crg::Attachment::createSampled( "DepthSp"
			, dsv );
		auto o = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto otv = graph.createView( test::createView( o, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto otAttach = crg::Attachment::createOutputColour( "FinalCombineTg"
			, otv );

		if constexpr ( EnableOpaque )
		{
			auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv1 = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vt1Attach = crg::Attachment::createOutputColour( "VelocityTg1"
				, vv1 );
			auto dcsAttach = buildDeferred< EnableSsao >( testCounts
				, dsAttach
				, *opaqueDTAttach
				, vt1Attach
				, graph );

			if constexpr ( EnableTransparent )
			{
				auto vv2 = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
				auto vt2Attach = crg::Attachment::createInOutColour( "VelocityTg2"
					, vv2 );
				auto wbcsAttach = buildWeightedBlended( testCounts
					, dsAttach
					, dt2Attach
					, vt2Attach
					, graph );

				auto vsAttach = crg::Attachment::createSampled( "VelocitySp"
					, vv2 );
				crg::RenderPass finalCombinePass
				{
					"finalCombinePass",
					{ dcsAttach, wbcsAttach, vsAttach },
					{ otAttach },
				};
				checkNoThrow( graph.add( finalCombinePass ) );
			}
			else
			{
				auto vsAttach = crg::Attachment::createSampled( "VelocitySp"
					, vv1 );
				crg::RenderPass finalCombinePass
				{
					"finalCombinePass",
					{ dcsAttach, vsAttach },
					{ otAttach },
				};
				checkNoThrow( graph.add( finalCombinePass ) );
			}
			
		}
		else if constexpr ( EnableTransparent )
		{
			auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vt1Attach = crg::Attachment::createOutputColour( "VelocityTg1"
				, vv );
			auto wbcsAttach = buildWeightedBlended( testCounts
				, dsAttach
				, dt2Attach
				, vt1Attach
				, graph );

			auto vsAttach = crg::Attachment::createSampled( "VelocitySp"
				, vv );
			crg::RenderPass finalCombinePass
			{
				"finalCombinePass",
				{ wbcsAttach, vsAttach },
				{ otAttach },
			};
			checkNoThrow( graph.add( finalCombinePass ) );
		}
		else
		{
			crg::RenderPass finalCombinePass
			{
				"finalCombinePass",
				{ dsAttach },
				{ otAttach },
			};
			checkNoThrow( graph.add( finalCombinePass ) );
		}
		

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
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
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "accumulationPass" [ label="DepthTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "VelocityTg2\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg2\nto\nVelocitySp" [ label="VelocityTg2" ];
    "VelocityTg2\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "geometryPass" [ label="DepthTg2" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoRawPass" [ label="Data2Sp" ];
    "SSAORawTg\nto\nSSAORawSp" [ shape=square ];
    "ssaoRawPass" -> "SSAORawTg\nto\nSSAORawSp" [ label="SSAORawTg" ];
    "SSAORawTg\nto\nSSAORawSp" -> "ssaoBlurPass" [ label="SSAORawSp" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" [ shape=square ];
    "ssaoBlurPass" -> "SSAOBlurTg\nto\nSSAOBlurSp" [ label="SSAOBlurTg" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" -> "ambientPass" [ label="SSAOBlurSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoBlurPass" [ label="Data2Sp" ];
    "VelocityTg1\nto\nVelocityTg2" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocityTg2" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocityTg2" -> "accumulationPass" [ label="VelocityTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "combinePass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ssaoLinearisePass" [ label="DepthSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoMinifyPass1" [ label="SSAOMin0Sp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass3" -> "SSAOMin3Tg\nto\nSSAOMinSp" [ label="SSAOMin3Tg" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoRawPass" [ label="SSAOMin2Sp" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMinSp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoRawPass" [ label="SSAOMin1Sp" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMinSp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoRawPass" [ label="SSAOMin0Sp" ];
    "SSAOLinTg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMinSp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "geometryPass" [ label="DepthTg2" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoRawPass" [ label="Data2Sp" ];
    "SSAORawTg\nto\nSSAORawSp" [ shape=square ];
    "ssaoRawPass" -> "SSAORawTg\nto\nSSAORawSp" [ label="SSAORawTg" ];
    "SSAORawTg\nto\nSSAORawSp" -> "ssaoBlurPass" [ label="SSAORawSp" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" [ shape=square ];
    "ssaoBlurPass" -> "SSAOBlurTg\nto\nSSAOBlurSp" [ label="SSAOBlurTg" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" -> "ambientPass" [ label="SSAOBlurSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoBlurPass" [ label="Data2Sp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ssaoLinearisePass" [ label="DepthSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoMinifyPass1" [ label="SSAOMin0Sp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass3" -> "SSAOMin3Tg\nto\nSSAOMinSp" [ label="SSAOMin3Tg" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoRawPass" [ label="SSAOMin2Sp" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMinSp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoRawPass" [ label="SSAOMin1Sp" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMinSp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoRawPass" [ label="SSAOMin0Sp" ];
    "SSAOLinTg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMinSp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "accumulationPass" [ label="DepthTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "VelocityTg2\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg2\nto\nVelocitySp" [ label="VelocityTg2" ];
    "VelocityTg2\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "geometryPass" [ label="DepthTg2" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "VelocityTg1\nto\nVelocityTg2" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocityTg2" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocityTg2" -> "accumulationPass" [ label="VelocityTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "combinePass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "geometryPass" [ label="DepthTg2" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
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
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "accumulationPass" [ label="DepthTg2" ];
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "combinePass" [ label="DepthSp" ];
}
)";
				}
				else
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "depthPrepass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "finalCombinePass" [ label="DepthSp" ];
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
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoRawPass" [ label="Data2Sp" ];
    "SSAORawTg\nto\nSSAORawSp" [ shape=square ];
    "ssaoRawPass" -> "SSAORawTg\nto\nSSAORawSp" [ label="SSAORawTg" ];
    "SSAORawTg\nto\nSSAORawSp" -> "ssaoBlurPass" [ label="SSAORawSp" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" [ shape=square ];
    "ssaoBlurPass" -> "SSAOBlurTg\nto\nSSAOBlurSp" [ label="SSAOBlurTg" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" -> "ambientPass" [ label="SSAOBlurSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoBlurPass" [ label="Data2Sp" ];
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "accumulationPass" [ label="DepthTg2" ];
    "VelocityTg1\nto\nVelocityTg2" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocityTg2" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocityTg2" -> "accumulationPass" [ label="VelocityTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "VelocityTg2\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg2\nto\nVelocitySp" [ label="VelocityTg2" ];
    "VelocityTg2\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "combinePass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ssaoLinearisePass" [ label="DepthSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoMinifyPass1" [ label="SSAOMin0Sp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass3" -> "SSAOMin3Tg\nto\nSSAOMinSp" [ label="SSAOMin3Tg" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoRawPass" [ label="SSAOMin2Sp" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMinSp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoRawPass" [ label="SSAOMin1Sp" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMinSp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoRawPass" [ label="SSAOMin0Sp" ];
    "SSAOLinTg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMinSp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoRawPass" [ label="Data2Sp" ];
    "SSAORawTg\nto\nSSAORawSp" [ shape=square ];
    "ssaoRawPass" -> "SSAORawTg\nto\nSSAORawSp" [ label="SSAORawTg" ];
    "SSAORawTg\nto\nSSAORawSp" -> "ssaoBlurPass" [ label="SSAORawSp" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" [ shape=square ];
    "ssaoBlurPass" -> "SSAOBlurTg\nto\nSSAOBlurSp" [ label="SSAOBlurTg" ];
    "SSAOBlurTg\nto\nSSAOBlurSp" -> "ambientPass" [ label="SSAOBlurSp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ssaoBlurPass" [ label="Data2Sp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ssaoLinearisePass" [ label="DepthSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoMinifyPass1" [ label="SSAOMin0Sp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoMinifyPass2" [ label="SSAOMin1Sp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoMinifyPass3" [ label="SSAOMin2Sp" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass3" -> "SSAOMin3Tg\nto\nSSAOMinSp" [ label="SSAOMin3Tg" ];
    "SSAOMin3Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMin2Sp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMin2Sp" -> "ssaoRawPass" [ label="SSAOMin2Sp" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass2" -> "SSAOMin2Tg\nto\nSSAOMinSp" [ label="SSAOMin2Tg" ];
    "SSAOMin2Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMin1Sp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMin1Sp" -> "ssaoRawPass" [ label="SSAOMin1Sp" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoMinifyPass1" -> "SSAOMin1Tg\nto\nSSAOMinSp" [ label="SSAOMin1Tg" ];
    "SSAOMin1Tg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMin0Sp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMin0Sp" -> "ssaoRawPass" [ label="SSAOMin0Sp" ];
    "SSAOLinTg\nto\nSSAOMinSp" [ shape=square ];
    "ssaoLinearisePass" -> "SSAOLinTg\nto\nSSAOMinSp" [ label="SSAOLinTg" ];
    "SSAOLinTg\nto\nSSAOMinSp" -> "ssaoRawPass" [ label="SSAOMinSp" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
    "DepthTg1\nto\nDepthTg2" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthTg2" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthTg2" -> "accumulationPass" [ label="DepthTg2" ];
    "VelocityTg1\nto\nVelocityTg2" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocityTg2" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocityTg2" -> "accumulationPass" [ label="VelocityTg2" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "VelocityTg2\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg2\nto\nVelocitySp" [ label="VelocityTg2" ];
    "VelocityTg2\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "combinePass" [ label="DepthSp" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "lightingPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "lightingPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "lightingPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "lightingPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "lightingPass" [ label="DepthSp" ];
    "DiffuseTg\nto\nDiffuseSp" [ shape=square ];
    "lightingPass" -> "DiffuseTg\nto\nDiffuseSp" [ label="DiffuseTg" ];
    "DiffuseTg\nto\nDiffuseSp" -> "ambientPass" [ label="DiffuseSp" ];
    "SpecularTg\nto\nSpecularSp" [ shape=square ];
    "lightingPass" -> "SpecularTg\nto\nSpecularSp" [ label="SpecularTg" ];
    "SpecularTg\nto\nSpecularSp" -> "ambientPass" [ label="SpecularSp" ];
    "OutputTg\nto\nOutputSp" [ shape=square ];
    "ambientPass" -> "OutputTg\nto\nOutputSp" [ label="OutputTg" ];
    "OutputTg\nto\nOutputSp" -> "finalCombinePass" [ label="OutputSp" ];
    "Data1Tg\nto\nData1Sp" [ shape=square ];
    "geometryPass" -> "Data1Tg\nto\nData1Sp" [ label="Data1Tg" ];
    "Data1Tg\nto\nData1Sp" -> "ambientPass" [ label="Data1Sp" ];
    "Data2Tg\nto\nData2Sp" [ shape=square ];
    "geometryPass" -> "Data2Tg\nto\nData2Sp" [ label="Data2Tg" ];
    "Data2Tg\nto\nData2Sp" -> "ambientPass" [ label="Data2Sp" ];
    "Data3Tg\nto\nData3Sp" [ shape=square ];
    "geometryPass" -> "Data3Tg\nto\nData3Sp" [ label="Data3Tg" ];
    "Data3Tg\nto\nData3Sp" -> "ambientPass" [ label="Data3Sp" ];
    "Data4Tg\nto\nData4Sp" [ shape=square ];
    "geometryPass" -> "Data4Tg\nto\nData4Sp" [ label="Data4Tg" ];
    "Data4Tg\nto\nData4Sp" -> "ambientPass" [ label="Data4Sp" ];
    "DepthTg1\nto\nDepthSp" [ shape=square ];
    "geometryPass" -> "DepthTg1\nto\nDepthSp" [ label="DepthTg1" ];
    "DepthTg1\nto\nDepthSp" -> "ambientPass" [ label="DepthSp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "geometryPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
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
    "AccumulationTg\nto\nAccumulationSp" [ shape=square ];
    "accumulationPass" -> "AccumulationTg\nto\nAccumulationSp" [ label="AccumulationTg" ];
    "AccumulationTg\nto\nAccumulationSp" -> "combinePass" [ label="AccumulationSp" ];
    "RevealageTg\nto\nRevealageSp" [ shape=square ];
    "accumulationPass" -> "RevealageTg\nto\nRevealageSp" [ label="RevealageTg" ];
    "RevealageTg\nto\nRevealageSp" -> "combinePass" [ label="RevealageSp" ];
    "CombineTg\nto\nCombineSp" [ shape=square ];
    "combinePass" -> "CombineTg\nto\nCombineSp" [ label="CombineTg" ];
    "CombineTg\nto\nCombineSp" -> "finalCombinePass" [ label="CombineSp" ];
    "VelocityTg1\nto\nVelocitySp" [ shape=square ];
    "accumulationPass" -> "VelocityTg1\nto\nVelocitySp" [ label="VelocityTg1" ];
    "VelocityTg1\nto\nVelocitySp" -> "finalCombinePass" [ label="VelocitySp" ];
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

		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRenderGraph" );
	testNoPass( testCounts );
	testOnePass( testCounts );
	testDuplicateName( testCounts );
	testWrongRemove( testCounts );
	testOneDependency( testCounts );
	testChainedDependencies( testCounts );
	testSharedDependencies( testCounts );
	test2MipDependencies( testCounts );
	test3MipDependencies( testCounts );
	testLoopDependencies( testCounts );
	testLoopDependenciesWithRoot( testCounts );
	testLoopDependenciesWithRootAndLeaf( testCounts );
	testSsaoPass( testCounts );
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
	testSuiteEnd();
}
