//
//  HeadCam.h
//  Matter
//
//  Created by Robert Hodgin on 3/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#define SPRING_STRENGTH 0.02
#define SPRING_DAMPING 0.25
#define REST_LENGTH 0.0

#include "cinder/Camera.h"
#include "cinder/Vector.h"
#include "cinder/app/AppBasic.h"

using namespace ci;

class HeadCam {
  public:
	
	
	HeadCam();
	HeadCam( float camDist, float aspectRatio );
	void update( Vec3f projectionPosition, Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft);
	void dragCam( const ci::Vec2f &posOffset, float distFromCenter );
	void draw();
	ci::CameraPersp getCam(){ return mCam; }
	ci::Vec3f getEye(){ return mCam.getEyePoint(); }
	void resetEye();
	void setEye(ci::Vec3f coordinates);
	void setCenter(ci::Vec3f coordinates);
	void setPreset( int i );
	void setFov(float fov);
	void adjustProjection(Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft, Vec3f eyePos, float n, float f);
	Matrix44f getAdjustmentMatrix(Vec3f bottomLeft, Vec3f bottomRight, Vec3f topLeft, Vec3f eyePos, float n, float f);
	
	ci::CameraPersp		mCam;
	float				mCamDist;
	float				mAspectRatio;
	ci::Vec3f			mEye, mCenter, mUp;
	float				mFov;
	
	ci::Matrix44f		mMvpMatrix;
	ci::Vec3f			mBillboardUp, mBillboardRight;
};