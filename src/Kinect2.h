/*
* 
* Copyright (c) 2013, Wieden+Kennedy
* Stephen Schieberl, Michael Latzoni
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#pragma once

#include "cinder/Exception.h"
#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"
#include "cinder/Surface.h"
#include <functional>
#include <map>
#include "ole2.h"

#if defined( _DEBUG )
#pragma comment( lib, "comsuppwd.lib" )
#else
#pragma comment( lib, "comsuppw.lib" )
#endif
#pragma comment( lib, "wbemuuid.lib" )

#include "Kinect.h"

namespace Kinect2 {

ci::Channel8u									channel16To8( const ci::Channel16u& channel );
ci::Surface8u									colorizeBodyIndex( const ci::Channel8u& bodyIndexChannel );

ci::Color8u										getBodyColor( uint64_t index );
size_t											getDeviceCount();
std::map<size_t, std::string>					getDeviceMap();

ci::Vec2i										mapBodyCoordToColor( const ci::Vec3f& v, ICoordinateMapper* mapper );
ci::Vec2i										mapBodyCoordToDepth( const ci::Vec3f& v, ICoordinateMapper* mapper );
//ci::Surface8u									mapColorFrameToDepth( const ci::Surface8u& color, ICoordinateMapper* mapper );
ci::Vec2i										mapDepthCoordToColor( const ci::Vec2i& v, uint16_t depth, ICoordinateMapper* mapper );
ci::Channel16u									mapDepthFrameToColor( const ci::Channel16u& depth, ICoordinateMapper* mapper );
ci::Surface32f                                  mapDepthFrameToCamera( const ci::Channel16u& depth, ICoordinateMapper* mapper );

ci::Quatf										toQuatf( const Vector4& v );
ci::Vec2f										toVec2f( const PointF& v );
ci::Vec2f										toVec2f( const ColorSpacePoint& v );
ci::Vec2f										toVec2f( const DepthSpacePoint& v );
ci::Vec3f										toVec3f( const CameraSpacePoint& v );
ci::Vec4f										toVec4f( const Vector4& v );

std::string										wcharToString( wchar_t* v );

//////////////////////////////////////////////////////////////////////////////////////////////

class DeviceOptions
{
public:
	DeviceOptions();
	
	DeviceOptions&								enableAudio( bool enable = true );
	DeviceOptions&								enableBody( bool enable = true );
	DeviceOptions&								enableBodyIndex( bool enable = true );
	DeviceOptions&								enableColor( bool enable = true );
	DeviceOptions&								enableDepth( bool enable = true );
	DeviceOptions&								enableInfrared( bool enable = true );
	DeviceOptions&								enableInfraredLongExposure( bool enable = true );
	DeviceOptions&								setDeviceId( const std::string& id = "" ); 
	DeviceOptions&								setDeviceIndex( int32_t index = 0 );

	const std::string&							getDeviceId() const;
	int32_t										getDeviceIndex() const;
	bool										isAudioEnabled() const;
	bool										isBodyEnabled() const;
	bool										isBodyIndexEnabled() const;
	bool										isColorEnabled() const;
	bool										isDepthEnabled() const;
	bool										isInfraredEnabled() const;
	bool										isInfraredLongExposureEnabled() const;
protected:
	std::string									mDeviceId;
	int32_t										mDeviceIndex;

	bool										mEnabledAudio;
	bool										mEnabledBody;
	bool										mEnabledBodyIndex;
	bool										mEnabledColor;
	bool										mEnabledDepth;
	bool										mEnabledInfrared;
	bool										mEnabledInfraredLongExposure;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class Device;

class Body
{
public:
	Body();

	//////////////////////////////////////////////////////////////////////////////////////////////

	class Joint
	{
	public:
		Joint();
		
		const ci::Quatf&						getOrientation() const;
		const ci::Vec3f&						getPosition() const;
		TrackingState							getTrackingState() const;
	protected:
		Joint( const ci::Vec3f& position, const ci::Quatf& orientation, TrackingState trackingState );
		
		ci::Quatf								mOrientation;
		ci::Vec3f								mPosition;
		TrackingState							mTrackingState;

		friend class							Device;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////

	float										calcConfidence( bool weighted = false ) const;

	uint64_t									getId() const;
	uint8_t										getIndex() const;
	const std::map<JointType, Body::Joint>&		getJointMap() const;
	bool										isTracked() const;
	const HandState&                            getLeftHandState() const;
    const HandState&                            getRightHandState() const;

private:
	Body( uint64_t id, uint8_t index, const std::map<JointType, Body::Joint>& jointMap );
	Body( uint64_t id, uint8_t index, const std::map<JointType, Body::Joint>& jointMap, HandState leftHandState, HandState rightHandState );

	uint64_t									mId;
	uint8_t										mIndex;
	std::map<JointType, Body::Joint>			mJointMap;
	bool										mTracked;
	HandState									mLeftHandState;
    HandState                                   mRightHandState;

	friend class								Device;
};

class Frame
{
public:
	Frame();

	const std::vector<Body>&					getBodies() const;
	const ci::Channel8u&						getBodyIndex() const;
	const ci::Surface8u&						getColor() const;
	const ci::Channel16u&						getDepth() const;
	const std::string&							getDeviceId() const;
	const ci::Channel16u&						getInfrared() const;
	const ci::Channel16u&						getInfraredLongExposure() const;
	long long									getTimeStamp() const;
protected:
	Frame( long long frameId, const std::string& deviceId, const ci::Surface8u& color, 
		const ci::Channel16u& depth, const ci::Channel16u& infrared, 
		const ci::Channel16u& infraredLongExposure );

	std::vector<Body>							mBodies;
	std::string									mDeviceId;
	ci::Channel8u								mChannelBodyIndex;
	ci::Channel16u								mChannelDepth;
	ci::Channel16u								mChannelInfrared;
	ci::Channel16u								mChannelInfraredLongExposure;
	ci::Surface8u								mSurfaceColor;
	long long									mTimeStamp;

	friend class								Device;
};

//////////////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<Device>	DeviceRef;

class Device
{
public:
	static DeviceRef							create();
	~Device();
	
	void										start( const DeviceOptions& deviceOptions = DeviceOptions() );
	void										stop();

	ICoordinateMapper*							getCoordinateMapper() const;
	const DeviceOptions&						getDeviceOptions() const;
	const Frame&								getFrame() const;
	const ci::Vec4f&                            getFloorPlane() const;

protected:
	Device();

	virtual void								update();

	std::function<void ( Frame frame )>			mEventHandler;
	
	ICoordinateMapper*							mCoordinateMapper;
	IMultiSourceFrameReader*					mFrameReader;
	IKinectSensor*								mSensor;
	//WAITABLE_HANDLE							onSensorCollectionChanged();

	DeviceOptions								mDeviceOptions;
	Frame										mFrame;
	ci::Vec4f                                   mFloorPlane;

public:

	//////////////////////////////////////////////////////////////////////////////////////////////

	class Exception : public ci::Exception
	{
	public:
		const char* what() const throw();
	protected:
		char									mMessage[ 2048 ];
		friend class							Device;
	};
	
	class ExcDeviceEnumerationFailed : public Exception 
	{
	public:
		ExcDeviceEnumerationFailed( long hr ) throw();
	};

	class ExcDeviceInitFailed : public Exception 
	{
	public:
		ExcDeviceInitFailed( long hr, const std::string& id ) throw();
	};
	
	class ExcDeviceNotAvailable : public Exception 
	{
	public:
		ExcDeviceNotAvailable( long hr ) throw();
	};

	class ExcDeviceOpenFailed : public Exception 
	{
	public:
		ExcDeviceOpenFailed( long hr, const std::string& id ) throw();
	};

	class ExcGetCoordinateMapperFailed : public Exception 
	{
	public:
		ExcGetCoordinateMapperFailed( long hr, const std::string& id ) throw();
	};

	class ExcOpenFrameReaderFailed : public Exception 
	{
	public:
		ExcOpenFrameReaderFailed( long hr, const std::string& id ) throw();
	};
};

}
