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
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addSampledView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Spl" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		check( attachment.image.samplerDesc == defaultSamplerDesc );
		testEnd();
	}
	
	void testStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testStorageAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStorageView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Str" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		testEnd();
	}

	void testColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addColourView( "Colour"
			, view
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Colour" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addDepthStencilView("DepthStencil"
			, view
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "DepthStencil" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Ic" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Oc" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutColourAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "IOc" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Id" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Od" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "IOd" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Ids" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Ods" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "IOds" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Is" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "Os" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStencilAttachment" );
		crg::FrameGraph graph{ "test" };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.image.name == pass.name + view.data->name + "IOs" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
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
