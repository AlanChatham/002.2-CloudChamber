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
	// Update based on positions of the viewing window bounds relative to the camera
	void update( Vec3f straightAhead, float l, float r, float t, float b, float n, float f);
	// Update based on a window bounded by 3 points in global coordinate space
	void update( Vec3f topLeft, Vec3f bottomLeft, Vec3f bottomRight, float f);
	void dragCam( const ci::Vec2f &posOffset, float distFromCenter );
	void draw();
	ci::CameraPersp getCam(){ return mCam; }
	ci::Vec3f getEye(){ return mCam.getEyePoint(); }
	void resetEye();
	void setEye(ci::Vec3f coordinates);
	void setCenter(ci::Vec3f coordinates);
	void setPreset( int i );
	void setFov(float fov);
	void calculateProjectionMatrix(float l, float r, float t, float b, float n, float f);
	// Takes in a plane defining a window defined by 3 global coordinates,
	//  maps projection onto what that window would see
	void calculateProjectionMatrix(Vec3f topLeft, Vec3f bottomLeft, Vec3f bottomRight, Vec3f camPosition, float f);

	// Sets the fixed OpenGL <v3 pipeline
	void setFixedPipeline();
	
	ci::CameraPersp		mCam;
	float				mCamDist;
	float				mAspectRatio;
	ci::Vec3f			mEye, mCenter, mUp;
	float				mFov;
	
	ci::Matrix44f		mMvpMatrix;
	ci::Matrix44f       mProjectionMatrix;

};