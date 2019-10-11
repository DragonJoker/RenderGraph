#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/ImageData.hpp>

namespace
{
	void testSampledAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testSampledAttachment" );
		auto colImage = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createSampled( "Sampled"
			, colView );

		check( colAttachment.name == "Sampled" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testColourAttachment" );
		auto colImage = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createColour( "Colour"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, colView );

		check( colAttachment.name == "Colour" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testDepthStencilAttachment" );
		auto dsImage = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsView = test::makeId( test::createView( dsImage ) );
		auto dsAttachment = crg::Attachment::createDepthStencil( "DepthStencil"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsView );

		check( dsAttachment.name == "DepthStencil" );
		check( dsAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( dsAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( dsAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( dsAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( dsAttachment.view == dsView );
		testEnd();
	}

	void testInColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInColourAttachment" );
		auto colImage = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInputColour( "Colour"
			, colView );

		check( colAttachment.name == "Colour" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutColourAttachment" );
		auto colImage = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createOutputColour( "Colour"
			, colView );

		check( colAttachment.name == "Colour" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutColourAttachment" );
		auto colImage = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInOutColour( "Colour"
			, colView );

		check( colAttachment.name == "Colour" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInputDepthStencil( "Depth"
			, colView );

		check( colAttachment.name == "Depth" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createOutputDepthStencil( "Depth"
			, colView );

		check( colAttachment.name == "Depth" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInOutDepthStencil( "Depth"
			, colView );

		check( colAttachment.name == "Depth" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInputDepthStencil( "DepthStencil"
			, colView );

		check( colAttachment.name == "DepthStencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createOutputDepthStencil( "DepthStencil"
			, colView );

		check( colAttachment.name == "DepthStencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInOutDepthStencil( "DepthStencil"
			, colView );

		check( colAttachment.name == "DepthStencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInputDepthStencil( "Stencil"
			, colView );

		check( colAttachment.name == "Stencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createOutputDepthStencil( "Stencil"
			, colView );

		check( colAttachment.name == "Stencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}

	void testInOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStencilAttachment" );
		auto colImage = test::createImage( VK_FORMAT_S8_UINT );
		auto colView = test::makeId( test::createView( colImage ) );
		auto colAttachment = crg::Attachment::createInOutDepthStencil( "Stencil"
			, colView );

		check( colAttachment.name == "Stencil" );
		check( colAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( colAttachment.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( colAttachment.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( colAttachment.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( colAttachment.view == colView );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestAttachment" );
	testSampledAttachment( testCounts );
	testColourAttachment( testCounts );
	testDepthStencilAttachment( testCounts );
	testInColourAttachment( testCounts );
	testOutColourAttachment( testCounts );
	testInOutColourAttachment( testCounts );
	testInDepthStencilAttachment( testCounts );
	testOutDepthStencilAttachment( testCounts );
	testInOutDepthStencilAttachment( testCounts );
	testSuiteEnd();
}
