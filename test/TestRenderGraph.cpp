#include "Common.hpp"

#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/RunnablePass.hpp>

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
		crg::FrameGraph graph{ testCounts.testName };
		checkThrow( graph.compile() );

		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C", crg::RunnablePassCreator{} );
		pass.addOutputColourView( rtv );

		testEnd();
	}

	void testOnePass( test::TestCounts & testCounts )
	{
		testBegin( "testOnePass" );
		crg::FrameGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C", crg::RunnablePassCreator{} );
		pass.addOutputColourView(  rtv );
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
		crg::FrameGraph graph{ testCounts.testName };
		checkNoThrow( graph.createPass( "pass1C", crg::RunnablePassCreator{} ) );
		checkThrow( graph.createPass( "pass1C", crg::RunnablePassCreator{} ) );
		testEnd();
	}

	void testOneDependency( test::TestCounts & testCounts )
	{
		testBegin( "testOneDependency" );
		crg::FrameGraph graph{ testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass1 = graph.createPass( "pass1C", crg::RunnablePassCreator{} );
		pass1.addOutputColourView( rtv );

		auto out = graph.createImage( test::createImage( "out", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( "outv", out ) );
		auto & pass2 = graph.createPass( "pass2C", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( rtv, 0u );
		pass2.addOutputColourView( outv );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\npass2CrtvSampled" -> "pass2C" [ label="rtv" ];
    "\nTransition to\npass2CrtvSampled" [ shape=square ];
    "pass1C" -> "\nTransition to\npass2CrtvSampled" [ label="rtv" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testChainedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testChainedDependencies" );
		crg::FrameGraph graph{ testCounts.testName };
		auto d0 = graph.createImage( test::createImage( "d0", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0", crg::RunnablePassCreator{} );
		pass0.addOutputColourView( d0v );

		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1", crg::RunnablePassCreator{} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0u );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0u );
		pass2.addOutputColourView( d2v );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\npass1d0vSampled" -> "pass1" [ label="d0v" ];
    "\nTransition to\npass1d0vSampled" [ shape=square ];
    "\nTransition to\npass2d1vSampled" -> "pass2" [ label="d1v" ];
    "\nTransition to\npass2d1vSampled" [ shape=square ];
    "pass0" -> "\nTransition to\npass1d0vSampled" [ label="d0v" ];
    "pass1" -> "\nTransition to\npass2d1vSampled" [ label="d1v" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testSharedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testSharedDependencies" );
		crg::FrameGraph graph{ testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dstv1 = graph.createView( test::createView( "dstv1", d ) );
		auto d0 = graph.createImage( test::createImage( "d0", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0", crg::RunnablePassCreator{} );
		pass0.addOutputDepthView( dstv1 );
		pass0.addOutputColourView( d0v );

		auto dstv2 = graph.createView( test::createView( "dstv2", d ) );
		auto d1 = graph.createImage( test::createImage( "d1", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1", crg::RunnablePassCreator{} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0 );
		pass1.addOutputDepthView( dstv2 );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0 );
		pass2.addOutputColourView( d2v );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\npass1d0vSampled" -> "pass1" [ label="d0v" ];
    "\nTransition to\npass1d0vSampled" [ shape=square ];
    "\nTransition to\npass2d1vSampled" -> "pass2" [ label="d1v" ];
    "\nTransition to\npass2d1vSampled" [ shape=square ];
    "pass0" -> "\nTransition to\npass1d0vSampled" [ label="d0v" ];
    "pass1" -> "\nTransition to\npass2d1vSampled" [ label="d1v" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void test2MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test2MipDependencies" );
		crg::FrameGraph graph{ testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1", crg::RunnablePassCreator{} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2", crg::RunnablePassCreator{} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void test3MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test3MipDependencies" );
		crg::FrameGraph graph{ testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", VK_FORMAT_R32G32B32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1", crg::RunnablePassCreator{} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2", crg::RunnablePassCreator{} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3", crg::RunnablePassCreator{} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
		testEnd();
	}

	void testLoopDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependencies" );
		crg::FrameGraph graph{ testCounts.testName };
		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1", crg::RunnablePassCreator{} );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

		checkNoThrow( graph.compile() );
		testEnd();
	}

	void testLoopDependenciesWithRoot( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRoot" );
		crg::FrameGraph graph{ testCounts.testName };
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0", crg::RunnablePassCreator{} );
		pass0.addOutputColourView( bv );

		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1", crg::RunnablePassCreator{} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

        checkNoThrow( graph.compile() );
		testEnd();
	}

	void testLoopDependenciesWithRootAndLeaf( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRootAndLeaf" );
		crg::FrameGraph graph{ testCounts.testName };
		auto c = graph.createImage( test::createImage( "c", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( "cv", c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0", crg::RunnablePassCreator{} );
		pass0.addOutputColourView( cv );

		auto a = graph.createImage( test::createImage( "a", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", VK_FORMAT_R32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1", crg::RunnablePassCreator{} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addSampledView( cv, 1 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2", crg::RunnablePassCreator{} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addSampledView( cv, 1 );
		pass2.addOutputColourView( bv );

		auto & pass3 = graph.createPass( "pass3", crg::RunnablePassCreator{} );
		pass3.addDependency( pass2 );
		pass3.addSampledView( cv, 0 );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\npass1cvSampled" -> "pass1" [ label="cv" ];
    "\nTransition to\npass1cvSampled" [ shape=square ];
    "\nTransition to\npass2avSampled" -> "pass2" [ label="av" ];
    "\nTransition to\npass2avSampled" [ shape=square ];
    "pass0" -> "\nTransition to\npass1cvSampled" [ label="cv" ];
    "pass1" -> "\nTransition to\npass2avSampled" [ label="av" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
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
		auto & ssaoLinearisePass = graph.createPass( "ssaoLinearisePass", crg::RunnablePassCreator{} );
		ssaoLinearisePass.addDependency( previous );
		ssaoLinearisePass.addSampledView( dsv, 0 );
		ssaoLinearisePass.addOutputColourView( m0v );

		auto m1v = graph.createView( test::createView( "m1v", lp, VK_FORMAT_R32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1", crg::RunnablePassCreator{} );
		ssaoMinifyPass1.addDependency( ssaoLinearisePass );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2", crg::RunnablePassCreator{} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3", crg::RunnablePassCreator{} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		auto mv = graph.createView( test::createView("mv",  lp, VK_FORMAT_R32_SFLOAT, 0u, 4u ) );
		auto rs = graph.createImage( test::createImage( "rs", VK_FORMAT_R32_SFLOAT ) );
		auto rsv = graph.createView( test::createView( "rsv", rs, VK_FORMAT_R32_SFLOAT ) );
		auto & ssaoRawPass = graph.createPass( "ssaoRawPass", crg::RunnablePassCreator{} );
		ssaoRawPass.addDependency( ssaoMinifyPass3 );
		ssaoRawPass.addSampledView( mv, 0 );
		ssaoRawPass.addSampledView( m3v, 1 );
		ssaoRawPass.addOutputColourView( rsv );

		auto bl = graph.createImage( test::createImage( "b1", VK_FORMAT_R32_SFLOAT ) );
		auto blv = graph.createView( test::createView( "b1v", bl, VK_FORMAT_R32_SFLOAT ) );
		auto & ssaoBlurPass = graph.createPass( "ssaoBlurPass", crg::RunnablePassCreator{} );
		ssaoBlurPass.addDependency( ssaoRawPass );
		ssaoBlurPass.addSampledView( rsv, 0 );
		ssaoBlurPass.addSampledView( m3v, 1 );
		ssaoBlurPass.addOutputColourView( blv );

		return { blv, &ssaoBlurPass };
	}

	void testSsaoPass( test::TestCounts & testCounts )
	{
		testBegin( "testSsaoPass" );
		crg::FrameGraph graph{ testCounts.testName };
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
		auto & geometryPass = graph.createPass( "geometryPass", crg::RunnablePassCreator{} );
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
		auto & ambientPass = graph.createPass( "ambientPass", crg::RunnablePassCreator{} );
		ambientPass.addDependency( *ssaoResult.second );
		ambientPass.addSampledView( dsv, 0 );
		ambientPass.addSampledView( d1v, 1 );
		ambientPass.addSampledView( d2v, 2 );
		ambientPass.addSampledView( d3v, 3 );
		ambientPass.addSampledView( d4v, 4 );
		ambientPass.addSampledView( ssaoResult.first, 5 );
		ambientPass.addOutputColourView( ofv );

		checkNoThrow( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassb1vSampled" -> "ambientPass" [ label="b1v" ];
    "\nTransition to\nambientPassb1vSampled" [ shape=square ];
    "\nTransition to\nssaoBlurPassrsvSampled" -> "ssaoBlurPass" [ label="rsv" ];
    "\nTransition to\nssaoBlurPassrsvSampled" [ shape=square ];
    "\nTransition to\nssaoLinearisePassdsvSampled" -> "ssaoLinearisePass" [ label="dtv" ];
    "\nTransition to\nssaoLinearisePassdsvSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" -> "ssaoMinifyPass1" [ label="m0v" ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassm3vSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassm3vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassmvSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassmvSampled" [ shape=square ];
    "geometryPass" -> "\nTransition to\nssaoLinearisePassdsvSampled" [ label="dtv" ];
    "ssaoBlurPass" -> "\nTransition to\nambientPassb1vSampled" [ label="b1v" ];
    "ssaoLinearisePass" -> "\nTransition to\nssaoMinifyPass1m0vSampled" [ label="m0v" ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassm3vSampled" [ label="m3v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassmvSampled" [ label="m3v" ];
    "ssaoRawPass" -> "\nTransition to\nssaoBlurPassrsvSampled" [ label="rsv" ];
}
)";
		checkEqualLines( sort( stream.str() ), sort( ref ) );
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
		auto & geometryPass = graph.createPass( "geometryPass", crg::RunnablePassCreator{} );

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
		auto & lightingPass = graph.createPass( "lightingPass", crg::RunnablePassCreator{} );
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
			auto & ambientPass = graph.createPass( "ambientPass", crg::RunnablePassCreator{} );
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
			auto & ambientPass = graph.createPass( "ambientPass", crg::RunnablePassCreator{} );
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
		auto & accumulationPass = graph.createPass( "accumulationPass", crg::RunnablePassCreator{} );

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
		auto & combinePass = graph.createPass( "combinePass", crg::RunnablePassCreator{} );
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
		crg::FrameGraph graph{ testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( "dtv", d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		crg::FramePass * previous{};

		if constexpr ( EnableDepthPrepass )
		{
			auto & depthPrepass = graph.createPass( "depthPrepass", crg::RunnablePassCreator{} );
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
				auto & finalCombinePass = graph.createPass( "finalCombinePass", crg::RunnablePassCreator{} );
				finalCombinePass.addDependency( *wbcsv.second );
				finalCombinePass.addSampledView( dcs.first, 0 );
				finalCombinePass.addSampledView( wbcsv.first, 1 );
				finalCombinePass.addSampledView( vv, 2 );
				finalCombinePass.addOutputColourView( otv );
			}
			else
			{
				auto & finalCombinePass = graph.createPass( "finalCombinePass", crg::RunnablePassCreator{} );
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
			auto & finalCombinePass = graph.createPass( "finalCombinePass", crg::RunnablePassCreator{} );
			finalCombinePass.addDependency( *wbcsv.second );
			finalCombinePass.addSampledView( wbcsv.first, 0 );
			finalCombinePass.addSampledView( vv, 1 );
			finalCombinePass.addOutputColourView( otv );
		}
		else
		{
			auto & finalCombinePass = graph.createPass( "finalCombinePass", crg::RunnablePassCreator{} );

			if ( previous )
			{
				finalCombinePass.addDependency( *previous );
			}

			finalCombinePass.addSampledView( dsv, 0 );
			finalCombinePass.addOutputColourView( otv );
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
    "\nTransition to\nambientPassb1vSampled" -> "ambientPass" [ label="b1v" ];
    "\nTransition to\nambientPassb1vSampled" [ shape=square ];
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "\nTransition to\nssaoBlurPassrsvSampled" -> "ssaoBlurPass" [ label="rsv" ];
    "\nTransition to\nssaoBlurPassrsvSampled" [ shape=square ];
    "\nTransition to\nssaoLinearisePassdsvSampled" -> "ssaoLinearisePass" [ label="dtv" ];
    "\nTransition to\nssaoLinearisePassdsvSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" -> "ssaoMinifyPass1" [ label="m0v" ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassm3vSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassm3vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassmvSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassmvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "geometryPass" -> "\nTransition to\nssaoLinearisePassdsvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
    "ssaoBlurPass" -> "\nTransition to\nambientPassb1vSampled" [ label="b1v" ];
    "ssaoLinearisePass" -> "\nTransition to\nssaoMinifyPass1m0vSampled" [ label="m0v" ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassm3vSampled" [ label="m3v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassmvSampled" [ label="m3v" ];
    "ssaoRawPass" -> "\nTransition to\nssaoBlurPassrsvSampled" [ label="rsv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassb1vSampled" -> "ambientPass" [ label="b1v" ];
    "\nTransition to\nambientPassb1vSampled" [ shape=square ];
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePassofvSampled" -> "finalCombinePass" [ label="ofv" ];
    "\nTransition to\nfinalCombinePassofvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "\nTransition to\nssaoBlurPassrsvSampled" -> "ssaoBlurPass" [ label="rsv" ];
    "\nTransition to\nssaoBlurPassrsvSampled" [ shape=square ];
    "\nTransition to\nssaoLinearisePassdsvSampled" -> "ssaoLinearisePass" [ label="dtv" ];
    "\nTransition to\nssaoLinearisePassdsvSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" -> "ssaoMinifyPass1" [ label="m0v" ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassm3vSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassm3vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassmvSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassmvSampled" [ shape=square ];
    "ambientPass" -> "\nTransition to\nfinalCombinePassofvSampled" [ label="ofv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "geometryPass" -> "\nTransition to\nssaoLinearisePassdsvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
    "ssaoBlurPass" -> "\nTransition to\nambientPassb1vSampled" [ label="b1v" ];
    "ssaoLinearisePass" -> "\nTransition to\nssaoMinifyPass1m0vSampled" [ label="m0v" ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassm3vSampled" [ label="m3v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassmvSampled" [ label="m3v" ];
    "ssaoRawPass" -> "\nTransition to\nssaoBlurPassrsvSampled" [ label="rsv" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePassofvSampled" -> "finalCombinePass" [ label="ofv" ];
    "\nTransition to\nfinalCombinePassofvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "ambientPass" -> "\nTransition to\nfinalCombinePassofvSampled" [ label="ofv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
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
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
}
)";
				}
				else
				{
					ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nfinalCombinePassdsvSampled" -> "finalCombinePass" [ label="dtv" ];
    "\nTransition to\nfinalCombinePassdsvSampled" [ shape=square ];
    "depthPrepass" -> "\nTransition to\nfinalCombinePassdsvSampled" [ label="dtv" ];
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
    "\nTransition to\nambientPassb1vSampled" -> "ambientPass" [ label="b1v" ];
    "\nTransition to\nambientPassb1vSampled" [ shape=square ];
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "\nTransition to\nssaoBlurPassrsvSampled" -> "ssaoBlurPass" [ label="rsv" ];
    "\nTransition to\nssaoBlurPassrsvSampled" [ shape=square ];
    "\nTransition to\nssaoLinearisePassdsvSampled" -> "ssaoLinearisePass" [ label="dtv" ];
    "\nTransition to\nssaoLinearisePassdsvSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" -> "ssaoMinifyPass1" [ label="m0v" ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassm3vSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassm3vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassmvSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassmvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "geometryPass" -> "\nTransition to\nssaoLinearisePassdsvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
    "ssaoBlurPass" -> "\nTransition to\nambientPassb1vSampled" [ label="b1v" ];
    "ssaoLinearisePass" -> "\nTransition to\nssaoMinifyPass1m0vSampled" [ label="m0v" ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassm3vSampled" [ label="m3v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassmvSampled" [ label="m3v" ];
    "ssaoRawPass" -> "\nTransition to\nssaoBlurPassrsvSampled" [ label="rsv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassb1vSampled" -> "ambientPass" [ label="b1v" ];
    "\nTransition to\nambientPassb1vSampled" [ shape=square ];
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePassofvSampled" -> "finalCombinePass" [ label="ofv" ];
    "\nTransition to\nfinalCombinePassofvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "\nTransition to\nssaoBlurPassrsvSampled" -> "ssaoBlurPass" [ label="rsv" ];
    "\nTransition to\nssaoBlurPassrsvSampled" [ shape=square ];
    "\nTransition to\nssaoLinearisePassdsvSampled" -> "ssaoLinearisePass" [ label="dtv" ];
    "\nTransition to\nssaoLinearisePassdsvSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" -> "ssaoMinifyPass1" [ label="m0v" ];
    "\nTransition to\nssaoMinifyPass1m0vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" -> "ssaoMinifyPass2" [ label="m1v" ];
    "\nTransition to\nssaoMinifyPass2m1vSampled" [ shape=square ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" -> "ssaoMinifyPass3" [ label="m2v" ];
    "\nTransition to\nssaoMinifyPass3m2vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassm3vSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassm3vSampled" [ shape=square ];
    "\nTransition to\nssaoRawPassmvSampled" -> "ssaoRawPass" [ label="m3v" ];
    "\nTransition to\nssaoRawPassmvSampled" [ shape=square ];
    "ambientPass" -> "\nTransition to\nfinalCombinePassofvSampled" [ label="ofv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "geometryPass" -> "\nTransition to\nssaoLinearisePassdsvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
    "ssaoBlurPass" -> "\nTransition to\nambientPassb1vSampled" [ label="b1v" ];
    "ssaoLinearisePass" -> "\nTransition to\nssaoMinifyPass1m0vSampled" [ label="m0v" ];
    "ssaoMinifyPass1" -> "\nTransition to\nssaoMinifyPass2m1vSampled" [ label="m1v" ];
    "ssaoMinifyPass2" -> "\nTransition to\nssaoMinifyPass3m2vSampled" [ label="m2v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassm3vSampled" [ label="m3v" ];
    "ssaoMinifyPass3" -> "\nTransition to\nssaoRawPassmvSampled" [ label="m3v" ];
    "ssaoRawPass" -> "\nTransition to\nssaoBlurPassrsvSampled" [ label="rsv" ];
}
)";
					}
				}
				else
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
}
)";
					}
					else
					{
						ref = R"(digraph ")" + testCounts.testName + R"(" {
    "\nTransition to\nambientPassdfvSampled" -> "ambientPass" [ label="dfv" ];
    "\nTransition to\nambientPassdfvSampled" [ shape=square ];
    "\nTransition to\nambientPassspvSampled" -> "ambientPass" [ label="spv" ];
    "\nTransition to\nambientPassspvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePassofvSampled" -> "finalCombinePass" [ label="ofv" ];
    "\nTransition to\nfinalCombinePassofvSampled" [ shape=square ];
    "\nTransition to\nlightingPassd1vSampled" -> "lightingPass" [ label="d1v" ];
    "\nTransition to\nlightingPassd1vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd2vSampled" -> "lightingPass" [ label="d2v" ];
    "\nTransition to\nlightingPassd2vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd3vSampled" -> "lightingPass" [ label="d3v" ];
    "\nTransition to\nlightingPassd3vSampled" [ shape=square ];
    "\nTransition to\nlightingPassd4vSampled" -> "lightingPass" [ label="d4v" ];
    "\nTransition to\nlightingPassd4vSampled" [ shape=square ];
    "\nTransition to\nlightingPassdtvSampled" -> "lightingPass" [ label="dtv" ];
    "\nTransition to\nlightingPassdtvSampled" [ shape=square ];
    "ambientPass" -> "\nTransition to\nfinalCombinePassofvSampled" [ label="ofv" ];
    "geometryPass" -> "\nTransition to\nlightingPassd1vSampled" [ label="d1v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd2vSampled" [ label="d2v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd3vSampled" [ label="d3v" ];
    "geometryPass" -> "\nTransition to\nlightingPassd4vSampled" [ label="d4v" ];
    "geometryPass" -> "\nTransition to\nlightingPassdtvSampled" [ label="dtv" ];
    "lightingPass" -> "\nTransition to\nambientPassdfvSampled" [ label="dfv" ];
    "lightingPass" -> "\nTransition to\nambientPassspvSampled" [ label="spv" ];
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
    "\nTransition to\ncombinePassavSampled" -> "combinePass" [ label="av" ];
    "\nTransition to\ncombinePassavSampled" [ shape=square ];
    "\nTransition to\ncombinePassdsvSampled" -> "combinePass" [ label="dtv" ];
    "\nTransition to\ncombinePassdsvSampled" [ shape=square ];
    "\nTransition to\ncombinePassrvSampled" -> "combinePass" [ label="rv" ];
    "\nTransition to\ncombinePassrvSampled" [ shape=square ];
    "\nTransition to\nfinalCombinePasscvSampled" -> "finalCombinePass" [ label="cv" ];
    "\nTransition to\nfinalCombinePasscvSampled" [ shape=square ];
    "accumulationPass" -> "\nTransition to\ncombinePassavSampled" [ label="av" ];
    "accumulationPass" -> "\nTransition to\ncombinePassdsvSampled" [ label="dtv" ];
    "accumulationPass" -> "\nTransition to\ncombinePassrvSampled" [ label="rv" ];
    "combinePass" -> "\nTransition to\nfinalCombinePasscvSampled" [ label="cv" ];
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
