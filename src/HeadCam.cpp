//
//  HeadCam.cpp
//  Matter
//
//  Created by Robert Hodgin on 3/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "HeadCam.h"

using namespace ci;

HeadCam::HeadCam()
{
}

HeadCam::HeadCam( float camDist, float aspectRatio )
{
	mCamDist		= camDist;
	
	mEye			= Vec3f( 0.0f, 0.0f, mCamDist );
	mCenter			= Vec3f( 0.0f, 0.0f, 0.0f );
	mUp				= Vec3f::yAxis();
	
	mAspectRatio	= aspectRatio;
	mFov			= 25.0f;//65.0f;
	
	mCam.setPerspective( mFov, aspectRatio, 1.0f, 3000.0f );
	
}

void HeadCam::setFov(float fov){
	mFov = fov;
	mCam.setPerspective( mFov, mAspectRatio, 1.0f, 3000.0f );
}

void HeadCam::update( Vec3f straightAhead, float l, float r, float t, float b, float n, float f)
{	
	mCam.lookAt(mEye, straightAhead, mUp);

	// Get our projection matrix
	calculateProjectionMatrix(l, r, t, b, n, f);
	// Set the MVPMatrix for shader usage
	mMvpMatrix = mProjectionMatrix * mCam.getModelViewMatrix();

	setFixedPipeline();
}

void HeadCam::setFixedPipeline(){
	gl::pushMatrices();
	// Set the ModelView matrix on the fixed pipeline
	gl::setMatrices(mCam);
	
    // Load the fixed pipeline projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // zero it out
	glMultMatrixf(mProjectionMatrix); // Apply the matrix to the loaded projection matrix
	glMatrixMode(GL_MODELVIEW); // Return to our modelview matrix

	gl::popMatrices();
}

void HeadCam::update( Vec3f topLeft, Vec3f bottomLeft, Vec3f bottomRight, float f)
{	
	Vec3f vUp = topLeft - bottomLeft;
	Vec3f vRight = bottomRight - bottomLeft;
	Vec3f vPlaneNormal = vRight.cross(vUp);
	vPlaneNormal.normalize();

	Vec3f straightAhead = mEye - vPlaneNormal;
	mCam.lookAt(mEye, straightAhead, mUp);

	// Get our projection matrix
	calculateProjectionMatrix(topLeft, bottomLeft, bottomRight, mEye, f);
	// Set the MVPMatrix for shader usage
	mMvpMatrix = mProjectionMatrix * mCam.getModelViewMatrix();

	// Also set up the matricies for the fixed pipeline...
	setFixedPipeline();

}


void HeadCam::dragCam( const Vec2f &posOffset, float distFromCenter )
{
	mEye += Vec3f( posOffset.xy(), distFromCenter );
}

void HeadCam::resetEye()
{
	mEye = Vec3f( 0.0f, 0.0f, mCamDist );
}

void HeadCam::setEye(ci::Vec3f coordinates){
	mEye = coordinates;
}

void HeadCam::setCenter(ci::Vec3f coordinates){
	mCenter = coordinates;
}

void HeadCam::setPreset( int i )
{
	if( i == 0 ){
		mEye =  Vec3f( 0.0f, 0.0f, mCamDist ) ;
		mCenter = Vec3f( 0.0f, -100.0f, 0.0f ) ;
	} else if( i == 1 ){
		mEye = Vec3f( mCamDist * 0.4f, -175.0f, -100.0f ) ;
		mCenter = Vec3f( 0.0f, -190.0f, 0.0f ) ;
	} else if( i == 2 ){
		mEye = Vec3f( -174.0f, -97.8f, -20.0f ) ;
		mCenter = Vec3f( 0.0f, -190.0f, 0.0f ) ;
	}
}

//Sets the GL_PROJECTION matrix and returns it
void HeadCam::calculateProjectionMatrix(float l, float r, float t, float b, float n, float f)
{
	Matrix44f M;

	// This second n gets us to not clip things in front of our projection screen,
	//  effectively extending our clipping frustum.
	//  We need a separate one, since the x and y still need to be clipped by the proper n values
	float nZAdjust = n/4;

    // Build our own standard projection matrix
	memset(M, 0, 16 * sizeof (float));
	M[0] = (2 * n) / (r - l); M[4] =         0        ; M[ 8] =  (r + l) / (r - l); M[12] =      0      ;
	M[1] =         0        ; M[5] = (2 * n) / (t - b); M[ 9] =  (t + b) / (t - b); M[13] =      0      ;
	M[2] =         0        ; M[6] =         0        ; M[10] = -(f + nZAdjust) / (f - nZAdjust); M[14] = -(2 * f * nZAdjust)/(f-nZAdjust);
	M[3] =         0        ; M[7] =         0        ; M[11] =         -1        ; M[15] =      0      ;


	// An orthographic projection matrix, should you want it
//	M[0] = 2 / (r - l); M[4] =     0      ; M[ 8] =     0       ; M[12] =  -(r + l) / (r - l);
//	M[1] =     0      ; M[5] = 2 / (t - b); M[ 9] =     0       ; M[13] =  -(t + b) / (t - b);
//	M[2] =     0      ; M[6] =     0      ; M[10] = -2 / (f - n); M[14] =  -(f + n) / (f - n);
//	M[3] =     0      ; M[7] =     0      ; M[11] =     0       ; M[15] =         1         ;

	mProjectionMatrix = M;
}


void HeadCam::calculateProjectionMatrix(Vec3f topLeft, Vec3f bottomLeft, Vec3f bottomRight, Vec3f camPosition, float f){
	float l, r, t, b, n;
	Vec3f vUp = topLeft - bottomLeft;
	Vec3f vRight = bottomRight - bottomLeft;
	Vec3f planeNormal = vRight.cross(vUp);
	// Normalize these to project onto them
	vUp.normalize();
	vRight.normalize();
	planeNormal.normalize();

	Vec3f vTopLeft = topLeft - camPosition;
	Vec3f vBottomLeft = bottomLeft - camPosition;
	Vec3f vBottomRight = bottomRight - camPosition;

	n = -vTopLeft.dot(planeNormal);

	app::console() << "n" << n << std::endl;
	app::console() << "planeNormal" << planeNormal << std::endl;

	l = vRight.dot(vBottomLeft);
	r = vRight.dot(vBottomRight);
	t = vUp.dot(vTopLeft);
	b = vUp.dot(vBottomLeft);

	calculateProjectionMatrix(l, r, t, b, n, f);
}