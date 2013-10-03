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
		
	mBillboardRight	= Vec3f::xAxis();
	mBillboardUp	= Vec3f::yAxis();
}

void HeadCam::setFov(float fov){
	mFov = fov;
	mCam.setPerspective( mFov, mAspectRatio, 1.0f, 3000.0f );
}

void HeadCam::update( Vec3f projectionPosition, Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft)
{	mCam.getBillboardVectors(&mBillboardRight, &mBillboardUp);
	
//	app::console() << "This camera thinks that it's looking at..." << mEye << std::endl;

	mCam.lookAt( projectionPosition, mCenter, mUp );
//	mCam.lookAt( mEye, mCenter, mUp );

	// TODO - Refactor to support more distorted projections with the .z factor
	//Vec3f bottomLeft = Vec3f(lowerLeftRear.x, lowerLeftRear.y, -zOffset);
	//Vec3f bottomLeft = Vec3f(lowerLeftRear.x, lowerLeftRear.y, lowerLeftRear.z);
	//Vec3f bottomRight = Vec3f(upperRightFront.x, lowerLeftRear.y, lowerLeftRear.z);
	//Vec3f topLeft = Vec3f(lowerLeftRear.x, upperRightFront.y, lowerLeftRear.z);
//	app::console() << "Camera's projection matrix" << std::endl;
//	app::console() << mCam.getProjectionMatrix() << std::endl;

	gl::pushMatrices();
	gl::setMatrices(mCam);
//	app::console() << "Projection Matrix popped to stack" << std:: endl;
//	app::console() << gl::getProjection() << std::endl;

	adjustProjection(bottomLeft, bottomRight, topLeft, mEye, 1, 10000);

//	app::console() << "Projection adjusted to stack" << std:: endl;
//	app::console() << gl::getProjection() << std::endl;
//	mMvpMatrix = mCam.getProjectionMatrix() * mCam.getModelViewMatrix();
	mMvpMatrix = gl::getProjection() * mCam.getModelViewMatrix();
	gl::popMatrices();
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

void HeadCam::adjustProjection(Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft, Vec3f eyePos, float n, float f)
{
	Vec3f va, vb, vc;
	Vec3f vr, vu, vn;
	float l, r, b, t, d;
	Matrix44f M;
	// Compute an orthonormal basis for the screen.
	vr = bottomRight - bottomLeft;
	vu = topLeft - bottomLeft;
	vr.normalize();
	vu.normalize();
	vn = vr.cross(vu);
	vn.normalize();
	// Compute the screen corner vectors.
	va = bottomLeft - eyePos;
	vb = bottomRight - eyePos;
	vc = topLeft - eyePos;
	// Find the distance from the eye to screen plane.
	d = -va.dot(vn);
	// Find the extent of the perpendicular projection.
	l = vr.dot(va) * n / d;
	r = vr.dot(vb) * n / d;
	b = vu.dot(va) * n / d;
	t = vu.dot(vc) * n / d;
	// Load the perpendicular projection.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(l, r, b, t, n, f);
	// Rotate the projection to be non-perpendicular.
	memset(M, 0, 16 * sizeof (float));
	M[0] = vr[0]; M[4] = vr[1]; M[ 8] = vr[2];
	M[1] = vu[0]; M[5] = vu[1]; M[ 9] = vu[2];
	M[2] = vn[0]; M[6] = vn[1]; M[10] = vn[2];
	M[15] = 1.0f;
	glMultMatrixf(M);
	// Move the apex of the frustum to the origin.
	glTranslatef(-eyePos[0], -eyePos[1], -eyePos[2]);
	glMatrixMode(GL_MODELVIEW);
}

Matrix44f HeadCam::getAdjustmentMatrix(Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft, Vec3f eyePos, float n, float f)
{
	Vec3f va, vb, vc;
	Vec3f vr, vu, vn;
	float l, r, b, t, d;
	Matrix44f M;
	// Compute an orthonormal basis for the screen.
	vr = bottomRight - bottomLeft;
	vu = topLeft - bottomLeft;
	vr.normalize();
	vu.normalize();
	vn = vr.cross(vu);
	vn.normalize();
	// Compute the screen corner vectors.
	va = bottomLeft - eyePos;
	vb = bottomRight - eyePos;
	vc = topLeft - eyePos;
	// Find the distance from the eye to screen plane.
	d = -va.dot(vn);
	// Find the extent of the perpendicular projection.
	l = vr.dot(va) * n / d;
	r = vr.dot(vb) * n / d;
	b = vu.dot(va) * n / d;
	t = vu.dot(vc) * n / d;
	// Load the perpendicular projection.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(l, r, b, t, n, f);
	// Rotate the projection to be non-perpendicular.
	memset(M, 0, 16 * sizeof (float));
	M[0] = vr[0]; M[4] = vr[1]; M[ 8] = vr[2];
	M[1] = vu[0]; M[5] = vu[1]; M[ 9] = vu[2];
	M[2] = vn[0]; M[6] = vn[1]; M[10] = vn[2];
	M[15] = 1.0f;
	glMultMatrixf(M);
	// Move the apex of the frustum to the origin.
	glTranslatef(-eyePos[0], -eyePos[1], -eyePos[2]);
	glMatrixMode(GL_MODELVIEW);
	return M;
}