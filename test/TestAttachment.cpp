#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/ImageData.hpp>

namespace
{
	static constexpr crg::SamplerDesc defaultSamplerDesc{};

	void testSampledAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testSampledAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addSampledView( view, 1u );
		require( pass.sampled.size() == 1u );
		auto & attachment = pass.sampled[0];
		check( attachment.name == pass.name + view.data->name + "Sampled" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		check( attachment.binding == 1u );
		check( attachment.samplerDesc == defaultSamplerDesc );
		testEnd();
	}
	
	void testStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testStorageAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addStorageView( view, 1u );
		require( pass.sampled.size() == 1u );
		auto & attachment = pass.sampled[0];
		check( attachment.name == pass.name + view.data->name + "Storage" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		check( attachment.binding == 1u );
		testEnd();
	}

	void testColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addColourView( "Colour"
			, view
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE );
		require( pass.colourInOuts.size() == 1u );
		auto & attachment = pass.colourInOuts[0];
		check( attachment.name == pass.name + view.data->name + "Colour" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addDepthStencilView("DepthStencil"
			, view
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "DepthStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view == view );
		testEnd();
	}

	void testInColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInputColourView( view );
		require( pass.colourInOuts.size() == 1u );
		auto & attachment = pass.colourInOuts[0];
		check( attachment.name == pass.name + view.data->name + "InColour" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addOutputColourView( view );
		require( pass.colourInOuts.size() == 1u );
		auto & attachment = pass.colourInOuts[0];
		check( attachment.name == pass.name + view.data->name + "OutColour" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testInOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInOutColourView( view );
		require( pass.colourInOuts.size() == 1u );
		auto & attachment = pass.colourInOuts[0];
		check( attachment.name == pass.name + view.data->name + "InOutColour" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testInDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInputDepthView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InDepth" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addOutputDepthView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "OutDepth" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testInOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInOutDepthView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InOutDepth" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testInDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInputDepthStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InDepthStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addOutputDepthStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "OutDepthStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view == view );
		testEnd();
	}

	void testInOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInOutDepthStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InOutDepthStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view == view );
		testEnd();
	}

	void testInStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInputStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view == view );
		testEnd();
	}

	void testOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addOutputStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "OutStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view == view );
		testEnd();
	}

	void testInOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass pass{ "test", crg::RunnablePassCreator{} };
		pass.addInOutStencilView( view );
		require( pass.depthStencilInOut.has_value() );
		auto & attachment = *pass.depthStencilInOut;
		check( attachment.name == pass.name + view.data->name + "InOutStencil" );
		check( attachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view == view );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestAttachment" );
	testSampledAttachment( testCounts );
	testStorageAttachment( testCounts );
	testColourAttachment( testCounts );
	testDepthStencilAttachment( testCounts );
	testInColourAttachment( testCounts );
	testOutColourAttachment( testCounts );
	testInOutColourAttachment( testCounts );
	testInDepthAttachment( testCounts );
	testOutDepthAttachment( testCounts );
	testInOutDepthAttachment( testCounts );
	testInDepthStencilAttachment( testCounts );
	testOutDepthStencilAttachment( testCounts );
	testInOutDepthStencilAttachment( testCounts );
	testInStencilAttachment( testCounts );
	testOutStencilAttachment( testCounts );
	testInOutStencilAttachment( testCounts );
	testSuiteEnd();
}
