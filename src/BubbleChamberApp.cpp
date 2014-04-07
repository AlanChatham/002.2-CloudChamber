#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Text.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"
#include "Resources.h"
#include "Room.h"
#include "HeadCam.h"
#include "Controller.h"
#include "OscListener.h"
#include "OscMessage.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define APP_WIDTH		1280
#define APP_HEIGHT		720
#define ROOM_WIDTH		600
#define ROOM_HEIGHT		400
#define ROOM_DEPTH		600
#define ROOM_FBO_RES	2

class BubbleChamberApp : public AppBasic {
  public:
	virtual void	prepareSettings( Settings *settings );
	virtual void	setup();
	virtual void	mouseDown( MouseEvent event );
	virtual void	mouseUp( MouseEvent event );
	virtual void	mouseMove( MouseEvent event );
	virtual void	mouseDrag( MouseEvent event );
	virtual void	mouseWheel( MouseEvent event );
	virtual void	keyDown( KeyEvent event );
	virtual void	update();
	void			drawIntoRoomFbo();
	virtual void	draw();
	void			drawGuts(Area area);
	void			drawInfoPanel();
	void			checkOSCMessage(const osc::Message * message);
	void			setCameras(Vec3f headPosition, bool fromKeyboard);
	
	// CAMERA
	HeadCam			mActiveCam;
	HeadCam			mHeadCam0;
	HeadCam			mHeadCam1;
	
	// SHADERS
	gl::GlslProg		mRoomShader;
	gl::GlslProg		mGlowCubeShader;
	
	// TEXTURES
	gl::Texture			mSmokeTex;
	gl::Texture			mDecalTex;
	gl::Texture			mIconTex;
	
	// ROOM
	Room				mRoom;
	gl::Fbo				mRoomFbo;

	// MOUSE
	Vec2f				mMousePos, mMouseDownPos, mMouseOffset;
	bool				mMousePressed;
	
	// CONTROLLER
	Controller			mController;

	// SAVE IMAGES
	int					mNumSaveFrames;
	bool				mSaveFrames;
};

void BubbleChamberApp::setCameras(Vec3f headPosition, bool fromKeyboard = false){
	// Separate out the components	
	float headX = headPosition.x;
	float headY = headPosition.y;
	float headZ = headPosition.z;
	
	// Adjust these away from the wonky coordinates we get from the Kinect,
	//  then normalize them so that they're in units of 100px per foot

	if (!fromKeyboard){
	//3.28 feet in a meter
		headX = headX * 3.28f * 100;
		headY = headY * 3.28f * 100 - ( ROOM_HEIGHT / 3 ); // Offset to bring up the vertical position of the eye
		headZ = headZ * 3.28f * 100;
	}
	
	// Make sure that the cameras are located somewhere in front of the screens
	//  Orientation is   X
	//                   |        ------> Screen 2 x axis
	//                   |   ------ Screen 2
	//                   |   |
	//                   |   |
	//         Z----<----|---o < Screen 1, o is the origin
	//                   |   | 
	//                   v   |

	//console() << "headX: " << headX << std::endl;
	//console() << "headZ: " << headZ << std::endl;

	if (headZ > ROOM_DEPTH / 2){
		mHeadCam0.setEye(Vec3f(headX, headY, headZ));
	}
	else{
		mHeadCam0.mEye = Vec3f(0,0,1200);
	}
	if (headX < -ROOM_WIDTH / 2){
		// headZ is negative here since that axis is backwards on Screen 2
		mHeadCam1.setEye(Vec3f(headX, headY, headZ));
	}
	else{
		mHeadCam1.mEye = Vec3f(-1200,0,0);
	}
}

void BubbleChamberApp::checkOSCMessage(const osc::Message * message){
	// Sanity check that we have our legit head message
	if (message->getAddress() == "/head" && message->getNumArgs() == 3){
		float headX = message->getArgAsFloat(0);
		float headY = message->getArgAsFloat(1);
		float headZ = message->getArgAsFloat(2);

		setCameras(Vec3f(headX, headY, headZ), false);
	}
}

void BubbleChamberApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
//	settings->setBorderless( true );
}

void BubbleChamberApp::setup()
{
	// CAMERA	
	//mActiveCam		= HeadCam( -450.0f, getWindowAspectRatio() );

	// Setup the camera for the main window
	mHeadCam0 = HeadCam( 1210.0f, getWindowAspectRatio() );
	mHeadCam0.mEye = Vec3f(-1200,0,1200);
	mHeadCam0.mEye.y = 0;
	mHeadCam0.mCenter = Vec3f(0,0, 0 );

	mHeadCam1 = HeadCam( 1200.0f, getWindowAspectRatio() );
	mHeadCam1.mEye = Vec3f(-1210,0,0);
	mHeadCam1.mCenter = Vec3f(0, 0, 0 );

	// LOAD SHADERS
	try {
		mRoomShader		= gl::GlslProg( loadResource( RES_ROOM_VERT ), loadResource( RES_ROOM_FRAG ) );
		mGlowCubeShader	= gl::GlslProg( loadResource( RES_GLOWCUBE_VERT ), loadResource( RES_GLOWCUBE_FRAG ) );
	} catch( gl::GlslProgCompileExc e ) {
		std::cout << e.what() << std::endl;
		quit();
	}
	
	// TEXTURE FORMAT
	gl::Texture::Format mipFmt;
    mipFmt.enableMipmapping( true );
    mipFmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );    
    mipFmt.setMagFilter( GL_LINEAR );
	
	// LOAD TEXTURES
	mSmokeTex		= gl::Texture( loadImage( loadResource( RES_SMOKE_PNG ) ) );
	mDecalTex		= gl::Texture( loadImage( loadResource( RES_DECAL_PNG) ) );
	mIconTex		= gl::Texture( loadImage( loadResource( RES_ICON_BUBBLE_CHAMBER ) ), mipFmt );
	
	// ROOM
	gl::Fbo::Format roomFormat;
	roomFormat.setColorInternalFormat( GL_RGB );
	mRoomFbo			= gl::Fbo( APP_WIDTH/ROOM_FBO_RES, APP_HEIGHT/ROOM_FBO_RES, roomFormat );
	bool isPowerOn		= true;
	bool isGravityOn	= true;
	mRoom				= Room( Vec3f( ROOM_WIDTH / 2, ROOM_HEIGHT / 2, ROOM_DEPTH / 2 ), isPowerOn, isGravityOn );		
	mRoom.init();
	
	// MOUSE
	mMousePos		= Vec2f::zero();
	mMouseDownPos	= Vec2f::zero();
	mMouseOffset	= Vec2f::zero();
	mMousePressed	= false;

	// CONTROLLER
	mController.init( &mRoom );
	
	// SAVE IMAGES
	mSaveFrames		= false;
	mNumSaveFrames	= 0;
}

void BubbleChamberApp::mouseDown( MouseEvent event )
{
	mMouseDownPos = event.getPos();
	mMousePressed = true;
	mMouseOffset = Vec2f::zero();
}

void BubbleChamberApp::mouseUp( MouseEvent event )
{
	mMousePressed = false;
	mMouseOffset = Vec2f::zero();
}

void BubbleChamberApp::mouseMove( MouseEvent event )
{
	mMousePos = event.getPos();
}

void BubbleChamberApp::mouseDrag( MouseEvent event )
{
	mouseMove( event );
	mMouseOffset = ( mMousePos - mMouseDownPos ) * 0.4f;
}

void BubbleChamberApp::mouseWheel( MouseEvent event )
{
	float dWheel	= event.getWheelIncrement();
	mRoom.adjustTimeMulti( dWheel );
}

void BubbleChamberApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ){
		case ' ':	mRoom.togglePower();				break;
		case 'm':	mController.releaseMoths();			break;
		case '1':	mController.preset( 1 );			break;
		case '2':	mController.preset( 2 );			break;
		case '3':	mController.preset( 3 );			break;
		case '4':	mController.preset( 4 );			break;
		case '5':	mController.preset( 5 );			break;
		case '6':	mController.preset( 6 );			break;
		default:										break;
	}
	
	switch( event.getCode() ){
		case KeyEvent::KEY_UP:		mActiveCam.setEye( mRoom.getCornerCeilingPos() );	break;
		case KeyEvent::KEY_DOWN:	mActiveCam.setEye( mRoom.getCornerFloorPos() );		break;
		case KeyEvent::KEY_RIGHT:	mActiveCam.resetEye();								break;
		default: break;
	}

	switch( event.getCode() ){
		//case KeyEvent::KEY_UP:		mMouseRightPos = Vec2f( 222.0f, 205.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_UP:		setCameras(Vec3f(mHeadCam0.mEye.x, mHeadCam0.mEye.y, mHeadCam0.mEye.z - 100), true);
									break;
		//case KeyEvent::KEY_LEFT:	mMouseRightPos = Vec2f(-128.0f,-178.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_LEFT:	setCameras(Vec3f(mHeadCam0.mEye.x - 100, mHeadCam0.mEye.y, mHeadCam0.mEye.z), true);
									break;
			//case KeyEvent::KEY_RIGHT:	mMouseRightPos = Vec2f(-256.0f, 122.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_RIGHT:	setCameras(Vec3f(mHeadCam0.mEye.x + 100, mHeadCam0.mEye.y, mHeadCam0.mEye.z), true);	break;
		//case KeyEvent::KEY_DOWN:	mMouseRightPos = Vec2f(   0.0f,   0.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_DOWN:	setCameras(Vec3f(mHeadCam0.mEye.x, mHeadCam0.mEye.y, mHeadCam0.mEye.z + 100), true);
									break;
		default: break;
	}
}

void BubbleChamberApp::update()
{
	// ROOM
	mRoom.update();
	
	// CAMERA
	Vec3f topLeft = Vec3f(-ROOM_WIDTH/2, ROOM_HEIGHT/2, ROOM_DEPTH/2);
	Vec3f bottomLeft = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);
	Vec3f bottomRight = Vec3f(ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);

	mHeadCam0.update(topLeft, bottomLeft, bottomRight, 10000);

	console() << "cam0 position" << mHeadCam0.mEye << std::endl;
	// Now update Camera 1

	topLeft = Vec3f(-ROOM_WIDTH/2, ROOM_HEIGHT/2, -ROOM_DEPTH/2);
	bottomLeft = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, -ROOM_DEPTH/2);
	bottomRight = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);

	mHeadCam1.update(topLeft, bottomLeft, bottomRight, 10000);
	
	console() << "cam1 position" << mHeadCam1.mEye << std::endl;


	// CONTROLLER
	mController.update();
}

void BubbleChamberApp::drawIntoRoomFbo()
{
	mRoomFbo.bindFramebuffer();
	gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ), true );
	
	gl::setMatricesWindow( mRoomFbo.getSize(), false );
	gl::setViewport( mRoomFbo.getBounds() );
	gl::disableAlphaBlending();
	gl::enable( GL_TEXTURE_2D );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	Matrix44f m;
	m.setToIdentity();
	m.scale( mRoom.getDims() );

	mRoomShader.bind();
	mRoomShader.uniform( "mvpMatrix", mActiveCam.mMvpMatrix );
	mRoomShader.uniform( "mMatrix", m );
	mRoomShader.uniform( "eyePos", mActiveCam.mCam.getEyePoint() );
	mRoomShader.uniform( "roomDims", mRoom.getDims() );
	mRoomShader.uniform( "power", mRoom.getPower() );
	mRoomShader.uniform( "lightPower", mRoom.getLightPower() );
	mRoomShader.uniform( "timePer", mRoom.getTimePer() * 1.5f + 0.5f );
	mRoom.draw();
	mRoomShader.unbind();
	
	mRoomFbo.unbindFramebuffer();
	glDisable( GL_CULL_FACE );
}

void BubbleChamberApp::draw()
{
	Area mViewArea0 = Area(0, 0, getWindowSize().x / 2,getWindowSize().y);
	Area mViewArea1 = Area(getWindowSize().x / 2, 0, getWindowSize().x, getWindowSize().y);

	gl::clear( ColorA( 0.1f, 0.1f, 0.1f, 0.0f ), true );

	

	mActiveCam = mHeadCam1;
	drawGuts(mViewArea0);

	mActiveCam = mHeadCam0;
	drawGuts(mViewArea1);
}

void BubbleChamberApp::drawGuts(Area area)
{
	float power = mRoom.getPower();
	Color powerColor = Color( power, power, power );
	
	drawIntoRoomFbo();
	
	gl::setMatricesWindow( getWindowSize(), false );
	//gl::setViewport( getWindowBounds() );
	gl::setViewport( area );

	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::enableAlphaBlending();
	gl::enable( GL_TEXTURE_2D );
	gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	// ROOM
	mRoomFbo.bindTexture();
	gl::drawSolidRect( getWindowBounds() );
	
	
	//mActiveCam.setFixedPipeline();
	// PANEL
	drawInfoPanel();

	gl::pushMatrices();
	gl::setMatrices( mActiveCam.getCam() );

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // zero it out
	glMultMatrixf(mActiveCam.mProjectionMatrix); // Apply the matrix to the loaded projection matrix
	glMatrixMode(GL_MODELVIEW); // Return to our modelview matrix
	
	gl::disable( GL_TEXTURE_2D );
	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::enableAlphaBlending();
	gl::color( ColorA( powerColor, 1.0f ) );
	
	// PARTICLES
	mController.drawParticles( power );
	
	// DRAW GIBS
	mController.drawGibs();
	
	// BUBBLES
	mController.drawBubbles( power );
	
	gl::enable( GL_TEXTURE_2D );
	gl::disableDepthWrite();
	
	// DECALS
	gl::color( powerColor );
	mDecalTex.bind( 0 );
	mController.drawDecals();
	
	// SMOKES
	Vec3f right, up;
	mActiveCam.getCam().getBillboardVectors( &right, &up );
	mSmokeTex.bind( 0 );
	mController.drawSmokes( right, up );

	gl::disable( GL_TEXTURE_2D );
	gl::enableDepthWrite();
	gl::color( powerColor );
	
	// MOTHS
	mController.drawMoths();
	
	// GLOWCUBES
	mGlowCubeShader.bind();
	mGlowCubeShader.uniform( "eyePos", mActiveCam.getEye() );
	mGlowCubeShader.uniform( "power", mRoom.getPower() );
	mGlowCubeShader.uniform( "mvpMatrix", mActiveCam.mMvpMatrix );
	mController.drawGlowCubes( &mGlowCubeShader );
	mGlowCubeShader.unbind();

	if( mSaveFrames ){
//		writeImage( getHomeDirectory() + "BubbleChamber/" + toString( mNumSaveFrames ) + ".png", copyWindowSurface() );
		mNumSaveFrames ++;
	}
	
	gl::popMatrices();
}

void BubbleChamberApp::drawInfoPanel()
{
	gl::pushMatrices();
	gl::translate( mRoom.getDims() );
	gl::scale( Vec3f( -1.0f, -1.0f, 1.0f ) );
	gl::color( Color( 1.0f, 1.0f, 1.0f ) * ( 1.0 - mRoom.getPower() ) );
	gl::enableAlphaBlending();
	
	float iconWidth		= 50.0f;
	
	float X0			= 15.0f;
	float X1			= X0 + iconWidth;
	float Y0			= 300.0f;
	float Y1			= Y0 + iconWidth;
	
	// DRAW ICON AND MOTH
	float c = mRoom.getPower() * 0.5f + 0.5f;
	gl::color( ColorA( c, c, c, 0.5f ) );
	gl::draw( mIconTex, Rectf( X0, Y0, X1, Y1 ) );

	c = mRoom.getPower();
	gl::color( ColorA( c, c, c, 0.5f ) );
	gl::disable( GL_TEXTURE_2D );
	
	// DRAW TIME BAR
	float timePer		= mRoom.getTimePer();
	gl::drawSolidRect( Rectf( Vec2f( X0, Y1 + 2.0f ), Vec2f( X0 + timePer * ( iconWidth ), Y1 + 2.0f + 4.0f ) ) );
	
	// DRAW FPS BAR
	float fpsPer		= getAverageFps()/60.0f;
	gl::drawSolidRect( Rectf( Vec2f( X0, Y1 + 4.0f + 4.0f ), Vec2f( X0 + fpsPer * ( iconWidth ), Y1 + 4.0f + 6.0f ) ) );
	
	
	gl::popMatrices();
}


CINDER_APP_BASIC( BubbleChamberApp, RendererGl )
