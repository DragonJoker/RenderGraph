#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/ImageData.hpp>

namespace
{
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
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestAttachment" );
	testColourAttachment( testCounts );
	testDepthStencilAttachment( testCounts );
	testSuiteEnd();
}
