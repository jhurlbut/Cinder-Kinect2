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

#include "Kinect2.h"
#include "cinder/app/App.h"

#include <comutil.h>

namespace Kinect2
{
using namespace ci;
using namespace ci::app;
using namespace std;

Channel8u channel16To8( const Channel16u& channel )
{
	Channel8u channel8;
	if ( channel ) {
		channel8						= Channel8u( channel.getWidth(), channel.getHeight() );
		Channel16u::ConstIter iter16	= channel.getIter();
		Channel8u::Iter iter8			= channel8.getIter();
		while ( iter8.line() && iter16.line() ) {
			while ( iter8.pixel() && iter16.pixel() ) {
				iter8.v()				= iter16.v() >> 4;
			}
		}
	}
	return channel8;
}

Surface8u colorizeBodyIndex( const Channel8u& bodyIndexChannel )
{
	Surface8u surface;
	if ( bodyIndexChannel ) {
		surface = Surface8u( bodyIndexChannel.getWidth(), bodyIndexChannel.getHeight(), true, SurfaceChannelOrder::RGBA );
		Channel8u::ConstIter iterChannel	= bodyIndexChannel.getIter();
		Surface8u::Iter iterSurface			= surface.getIter();
		while ( iterChannel.line() && iterSurface.line() ) {
			while ( iterChannel.pixel() && iterSurface.pixel() ) {
				size_t index				= (size_t)iterChannel.v();
				ColorA8u color( getBodyColor( index ), 0xFF );
				if ( index == 0 || index > BODY_COUNT ) {
					color.a					= 0x00;
				}
				iterSurface.r()				= color.r;
				iterSurface.g()				= color.g;
				iterSurface.b()				= color.b;
				iterSurface.a()				= color.a;
			}
		}
	}
	return surface;
}

Color8u getBodyColor( uint64_t index )
{
	switch ( index ) {
	case 0:
		return Color8u::black();
	case 1:
		return Color8u( 0xFF, 0x00, 0x00 );
	case 2:
		return Color8u( 0x00, 0xFF, 0x00 );
	case 3:
		return Color8u( 0x00, 0x00, 0xFF );
	case 4:
		return Color8u( 0xFF, 0xFF, 0x00 );
	case 5:
		return Color8u( 0x00, 0xFF, 0xFF );
	case 6:
		return Color8u( 0xFF, 0x00, 0xFF );
	default:
		return Color8u::white();
	}
}

size_t getDeviceCount()
{
	size_t count								= 0;
	IKinectSensorCollection* sensorCollection	= 0;
	long hr = GetKinectSensorCollection( &sensorCollection );
	if ( SUCCEEDED( hr ) && sensorCollection != 0 ) {
		IEnumKinectSensor* sensorEnum = 0;
		hr = sensorCollection->get_Enumerator( &sensorEnum );
		if ( SUCCEEDED( hr ) || sensorEnum != 0 ) {
			size_t i = 0;
			while ( SUCCEEDED( hr ) && i < 8 ) {
				IKinectSensor* sensor = 0;
				hr = sensorEnum->GetNext( &sensor );
				if ( sensor != 0 ) {
					++count;
				}
				++i;
			}
		}
	}
	return count;
}

map<size_t, string> getDeviceMap()
{
	map<size_t, string> deviceMap;
	IKinectSensorCollection* sensorCollection	= 0;
	long hr = GetKinectSensorCollection( &sensorCollection );
	if ( SUCCEEDED( hr ) && sensorCollection != 0 ) {
		IEnumKinectSensor* sensorEnum = 0;
		hr = sensorCollection->get_Enumerator( &sensorEnum );
		if ( SUCCEEDED( hr ) || sensorEnum != 0 ) {
			size_t i = 0;
			while ( SUCCEEDED( hr ) && i < 8 ) {
				IKinectSensor* sensor = 0;
				hr = sensorEnum->GetNext( &sensor );
				if ( sensor != 0 ) {
					wchar_t wid[ 48 ];
					if ( SUCCEEDED( sensor->get_UniqueKinectId( 48, wid ) ) ) {
						string id = wcharToString( wid );
						if ( !id.empty() ) {
							deviceMap[ i ] = string( id );
						}
					}
				}
				++i;
			}
		}
	}
	return deviceMap;
}

Vec2i mapBodyCoordToColor( const Vec3f& v, ICoordinateMapper* mapper )
{
	CameraSpacePoint cameraSpacePoint;
	cameraSpacePoint.X = v.x;
	cameraSpacePoint.Y = v.y;
	cameraSpacePoint.Z = v.z;

	ColorSpacePoint colorSpacePoint;
	long hr = mapper->MapCameraPointToColorSpace( cameraSpacePoint, &colorSpacePoint );
	if ( SUCCEEDED( hr ) ) {
		return Vec2i( static_cast<int32_t>( colorSpacePoint.X ), static_cast<int32_t>( colorSpacePoint.Y ) );
	}
	return Vec2i();
}

Vec2i mapBodyCoordToDepth( const Vec3f& v, ICoordinateMapper* mapper )
{
	CameraSpacePoint cameraSpacePoint;
	cameraSpacePoint.X = v.x;
	cameraSpacePoint.Y = v.y;
	cameraSpacePoint.Z = v.z;

	DepthSpacePoint depthSpacePoint;
	long hr = mapper->MapCameraPointToDepthSpace( cameraSpacePoint, &depthSpacePoint );
	if ( SUCCEEDED( hr ) ) {
		return Vec2i( toVec2f( depthSpacePoint ) );
	}
	return Vec2i();
}

Vec2i mapDepthCoordToColor( const Vec2i& v, uint16_t depth, ICoordinateMapper* mapper )
{
	DepthSpacePoint depthSpacePoint;
	depthSpacePoint.X = (float)v.x;
	depthSpacePoint.Y = (float)v.y;

	ColorSpacePoint colorSpacePoint;
	long hr = mapper->MapDepthPointToColorSpace( depthSpacePoint, depth, &colorSpacePoint );
	if ( SUCCEEDED( hr ) ) {
		return Vec2i( toVec2f( colorSpacePoint ) );
	}
	return Vec2i();
}

ci::Surface32f mapDepthFrameToCamera( const Channel16u& depth, ICoordinateMapper* mapper )
{
    size_t numPoints = depth.getWidth() * depth.getHeight();
    ci::Surface32f channel ( depth.getWidth(), depth.getHeight(), false, SurfaceChannelOrder::RGB );
    vector<CameraSpacePoint> cameraSpacePoints( numPoints );
    long hr = mapper->MapDepthFrameToCameraSpace( (UINT)numPoints, depth.getData(), numPoints, &cameraSpacePoints[ 0 ] );
    if ( SUCCEEDED( hr ) ) {
        ci::Surface32f::Iter iter = channel.getIter();
        size_t i = 0;
        while ( iter.line() ) {
            while ( iter.pixel() ) {
                CameraSpacePoint pos = cameraSpacePoints[ i ];
                //console()<<"cameraSpacePoints[ i ] x " << cameraSpacePoints[ i ].X << " y " << cameraSpacePoints[ i ].Y << " z " << cameraSpacePoints[ i ].Z << endl;
                iter.r() = pos.X;
                iter.g() = pos.Y;
                iter.b() = pos.Z;
                i++;
            }
        }
    }
    return channel;
}


Channel16u mapDepthFrameToColor( const Channel16u& depth, ICoordinateMapper* mapper )
{
	size_t numPoints = depth.getWidth() * depth.getHeight();
	Channel16u channel( depth.getWidth(), depth.getHeight() );
	vector<ColorSpacePoint> colorSpacePoints( numPoints );
	long hr = mapper->MapDepthFrameToColorSpace( (UINT)numPoints, depth.getData(), numPoints, &colorSpacePoints[ 0 ] );
	if ( SUCCEEDED( hr ) ) {
		Channel16u::Iter iter = channel.getIter();
		size_t i = 0;
		while ( iter.line() ) {
			while ( iter.pixel() ) {
				Vec2i pos = Vec2i( toVec2f( colorSpacePoints[ i ] ) );
				uint16_t v = 0x0000;
				if ( pos.x >= 0 && pos.x < depth.getWidth() && pos.y >= 0 && pos.y < depth.getHeight() ) {
					v = depth.getValue( pos );
				}
				iter.v() = v;
			}
		}
	}
	return channel;
}

Quatf toQuatf( const Vector4& v )
{
	return Quatf( v.w, v.x, v.y, v.z );
}

Vec2f toVec2f( const PointF& v )
{
	return Vec2f( v.X, v.Y );
}

Vec2f toVec2f( const ColorSpacePoint& v )
{
	return Vec2f( v.X, v.Y );
}

Vec2f toVec2f( const DepthSpacePoint& v )
{
	return Vec2f( v.X, v.Y );
}

Vec3f toVec3f( const CameraSpacePoint& v )
{
	return Vec3f( v.X, v.Y, v.Z );
}

Vec4f toVec4f( const Vector4& v )
{
	return Vec4f( v.x, v.y, v.z, v.w );
}

string wcharToString( wchar_t* v )
{
	string str = "";
	wchar_t* id = ::SysAllocString( v );
	_bstr_t idStr( id );
	if ( idStr.length() > 0 ) {
		str = string( idStr );
	}
	::SysFreeString( id );
	return str;
}

//////////////////////////////////////////////////////////////////////////////////////////////

DeviceOptions::DeviceOptions()
: mDeviceIndex( 0 ), mDeviceId( "" ), mEnabledAudio( false ), mEnabledBody( false ), 
mEnabledBodyIndex( false ), mEnabledColor( true ), mEnabledDepth( true ), 
mEnabledInfrared( false ), mEnabledInfraredLongExposure( false )
{
}

DeviceOptions& DeviceOptions::enableAudio( bool enable )
{
	mEnabledAudio = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableBody( bool enable )
{
	mEnabledBody = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableBodyIndex( bool enable )
{
	mEnabledBodyIndex = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableColor( bool enable )
{
	mEnabledColor = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableDepth( bool enable )
{
	mEnabledDepth = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableInfrared( bool enable )
{
	mEnabledInfrared = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableInfraredLongExposure( bool enable )
{
	mEnabledInfraredLongExposure = enable;
	return *this;
}

DeviceOptions& DeviceOptions::setDeviceId( const string& id )
{
	mDeviceId = id;
	return *this;
}

DeviceOptions& DeviceOptions::setDeviceIndex( int32_t index )
{
	mDeviceIndex = index;
	return *this;
}

const string& DeviceOptions::getDeviceId() const
{
	return mDeviceId;
}

int32_t	 DeviceOptions::getDeviceIndex() const
{
	return mDeviceIndex;
}

bool DeviceOptions::isAudioEnabled() const
{
	return mEnabledAudio;
}

bool DeviceOptions::isBodyEnabled() const
{
	return mEnabledBody;
}

bool DeviceOptions::isBodyIndexEnabled() const
{
	return mEnabledBodyIndex;
}

bool DeviceOptions::isColorEnabled() const
{
	return mEnabledColor;
}

bool DeviceOptions::isDepthEnabled() const
{
	return mEnabledDepth;
}

bool DeviceOptions::isInfraredEnabled() const
{
	return mEnabledInfrared;
}

bool DeviceOptions::isInfraredLongExposureEnabled() const
{
	return mEnabledInfraredLongExposure;
}

//////////////////////////////////////////////////////////////////////////////////////////////

Body::Joint::Joint()
: mOrientation( Quatf() ), mPosition( Vec3f::zero() ), mTrackingState( TrackingState::TrackingState_NotTracked )
{
}

Body::Joint::Joint( const Vec3f& position, const Quatf& orientation, TrackingState trackingState )
: mOrientation( orientation ), mPosition( position ), mTrackingState( trackingState )
{
}

const Vec3f& Body::Joint::getPosition() const
{
	return mPosition;
}

const Quatf& Body::Joint::getOrientation() const
{
	return mOrientation;
}

TrackingState Body::Joint::getTrackingState() const
{
	return mTrackingState;
}

//////////////////////////////////////////////////////////////////////////////////////////////

Body::Body()
: mId( 0 ), mIndex( 0 ), mTracked( false )
{
}

Body::Body( uint64_t id, uint8_t index, const map<JointType, Body::Joint>& jointMap )
: mId( id ), mIndex( index ), mJointMap( jointMap ), mTracked( true )
{
}
Body::Body( uint64_t id, uint8_t index, const std::map<JointType, Body::Joint>& jointMap, HandState leftHandState, HandState rightHandState )
: mId( id ), mIndex( index ), mJointMap( jointMap ), mTracked( true ), mLeftHandState(leftHandState), mRightHandState(rightHandState)
{
}

float Body::calcConfidence( bool weighted ) const
{
	float c = 0.0f;
	if ( weighted ) {
		static map<JointType, float> weights;
		if ( weights.empty() ) {
			weights[ JointType::JointType_SpineBase ]		= 0.042553191f;
			weights[ JointType::JointType_SpineMid ]		= 0.042553191f;
			weights[ JointType::JointType_Neck ]			= 0.021276596f;
			weights[ JointType::JointType_Head ]			= 0.042553191f;
			weights[ JointType::JointType_ShoulderLeft ]	= 0.021276596f;
			weights[ JointType::JointType_ElbowLeft ]		= 0.010638298f;
			weights[ JointType::JointType_WristLeft ]		= 0.005319149f;
			weights[ JointType::JointType_HandLeft ]		= 0.042553191f;
			weights[ JointType::JointType_ShoulderRight ]	= 0.021276596f;
			weights[ JointType::JointType_ElbowRight ]		= 0.010638298f;
			weights[ JointType::JointType_WristRight ]		= 0.005319149f;
			weights[ JointType::JointType_HandRight ]		= 0.042553191f;
			weights[ JointType::JointType_HipLeft ]			= 0.021276596f;
			weights[ JointType::JointType_KneeLeft ]		= 0.010638298f;
			weights[ JointType::JointType_AnkleLeft ]		= 0.005319149f;
			weights[ JointType::JointType_FootLeft ]		= 0.042553191f;
			weights[ JointType::JointType_HipRight ]		= 0.021276596f;
			weights[ JointType::JointType_KneeRight ]		= 0.010638298f;
			weights[ JointType::JointType_AnkleRight ]		= 0.005319149f;
			weights[ JointType::JointType_FootRight ]		= 0.042553191f;
			weights[ JointType::JointType_SpineShoulder ]	= 0.002659574f;
			weights[ JointType::JointType_HandTipLeft ]		= 0.002659574f;
			weights[ JointType::JointType_ThumbLeft ]		= 0.002659574f;
			weights[ JointType::JointType_HandTipRight ]	= 0.002659574f;
			weights[ JointType::JointType_ThumbRight ]		= 0.521276596f;
		}
		for ( map<JointType, Body::Joint>::const_iterator iter = mJointMap.begin(); iter != mJointMap.end(); ++iter ) {
			if ( iter->second.getTrackingState() == TrackingState::TrackingState_Tracked ) {
				c += weights[ iter->first ];
			}
		}
	} else {
		for ( map<JointType, Body::Joint>::const_iterator iter = mJointMap.begin(); iter != mJointMap.end(); ++iter ) {
			if ( iter->second.getTrackingState() == TrackingState::TrackingState_Tracked ) {
				c += 1.0f;
			}
		}
		c /= (float)JointType::JointType_Count;
	}
	return c;
}

uint64_t Body::getId() const 
{ 
	return mId; 
}

uint8_t Body::getIndex() const 
{ 
	return mIndex; 
}

const map<JointType, Body::Joint>& Body::getJointMap() const 
{ 
	return mJointMap; 
}

bool Body::isTracked() const 
{ 
	return mTracked; 
}
const HandState& Body::getLeftHandState() const 
{ 
    return mLeftHandState; 
}

const HandState& Body::getRightHandState() const 
{ 
    return mRightHandState; 
}

//////////////////////////////////////////////////////////////////////////////////////////////

Frame::Frame()
: mDeviceId( "" ), mTimeStamp( 0L )
{
}

Frame::Frame( long long time, const string& deviceId, const Surface8u& color,
			  const Channel16u& depth, const Channel16u& infrared, 
			  const Channel16u& infraredLongExposure )
: mSurfaceColor( color ), mChannelDepth( depth ), mChannelInfrared( infrared ), 
mChannelInfraredLongExposure( infraredLongExposure ), mDeviceId( deviceId ), 
mTimeStamp( time )
{
}

const vector<Body>& Frame::getBodies() const
{
	return mBodies;
}

const Channel8u& Frame::getBodyIndex() const
{
	return mChannelBodyIndex;
}

const Surface8u& Frame::getColor() const
{
	return mSurfaceColor;
}

const Channel16u& Frame::getDepth() const
{
	return mChannelDepth;
}

const string& Frame::getDeviceId() const
{
	return mDeviceId;
}

const Channel16u& Frame::getInfrared() const
{
	return mChannelInfrared;
}

const Channel16u& Frame::getInfraredLongExposure() const
{
	return mChannelInfraredLongExposure;
}

int64_t Frame::getTimeStamp() const
{
	return mTimeStamp;
}

//////////////////////////////////////////////////////////////////////////////////////////////

DeviceRef Device::create()
{
	return DeviceRef( new Device() );
}

Device::Device()
: mFrameReader( 0 ), mSensor( 0 ), mCoordinateMapper( 0 )
{
	App::get()->getSignalUpdate().connect( bind( &Device::update, this ) );
}

Device::~Device()
{
	stop();
}

ICoordinateMapper* Device::getCoordinateMapper() const
{
	return mCoordinateMapper;
}

const DeviceOptions& Device::getDeviceOptions() const
{
	return mDeviceOptions;
}

const Frame& Device::getFrame() const
{
	return mFrame;
}

const ci::Vec4f&    Device::getFloorPlane() const{
    return mFloorPlane;
}
void Device::start( const DeviceOptions& deviceOptions )
{
	long hr = S_OK;
	mDeviceOptions = deviceOptions;
	
	IKinectSensorCollection* sensorCollection = 0;
	hr = GetKinectSensorCollection( &sensorCollection );
	if ( FAILED( hr ) || sensorCollection == 0 ) {
		throw ExcDeviceNotAvailable( hr );
	}

	//sensorCollection->SubscribeCollectionChanged( &Device::onSensorCollectionChanged );
	IEnumKinectSensor* sensorEnum = 0;
	hr = sensorCollection->get_Enumerator( &sensorEnum );
	if ( FAILED( hr ) || sensorEnum == 0 ) {
		throw ExcDeviceEnumerationFailed( hr );
	}

	hr			= S_OK;
	int32_t i	= 0;

	while ( SUCCEEDED( hr ) && i < 8 ) { // TODO find actual max device count
		hr = sensorEnum->GetNext( &mSensor );
		if ( mSensor != 0 ) {
			string id = "";
			wchar_t wid[ 48 ];
			if ( SUCCEEDED( mSensor->get_UniqueKinectId( 48, wid ) ) ) {
				id = wcharToString( wid );
			}
			if ( mDeviceOptions.getDeviceId().empty() ) {
				if ( mDeviceOptions.getDeviceIndex() == i ) {
					mDeviceOptions.setDeviceId( id );
					break;
				}
			} else {
				if ( mDeviceOptions.getDeviceId() == id ) {
					mDeviceOptions.setDeviceIndex( i );
					break;
				}
			}
		}
		++i;
	}

	if ( mSensor == 0 ) {
		throw ExcDeviceInitFailed( hr, mDeviceOptions.getDeviceId() );
	} else {
		hr = mSensor->Open();
		if ( SUCCEEDED( hr ) ) {
			hr = mSensor->get_CoordinateMapper( &mCoordinateMapper );
			if ( SUCCEEDED( hr ) ) {
				long flags = 0L;
				if ( mDeviceOptions.isAudioEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_Audio;
				}
				if ( mDeviceOptions.isBodyEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_Body;
				}
				if ( mDeviceOptions.isBodyIndexEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_BodyIndex;
				}
				if ( mDeviceOptions.isColorEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_Color;
				}
				if ( mDeviceOptions.isDepthEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_Depth;
				}
				if ( mDeviceOptions.isInfraredEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_Infrared;
				}
				if ( mDeviceOptions.isInfraredLongExposureEnabled() ) {
					flags |= FrameSourceTypes::FrameSourceTypes_LongExposureInfrared;
				}
				hr = mSensor->OpenMultiSourceFrameReader( flags, &mFrameReader );
				if ( FAILED( hr ) ) {
					if ( mFrameReader != 0 ) {
						mFrameReader->Release();
						mFrameReader = 0;
					}
					throw ExcOpenFrameReaderFailed( hr, mDeviceOptions.getDeviceId() );
				}
			} else {
				throw ExcDeviceOpenFailed( hr, mDeviceOptions.getDeviceId() );
			}
		} else {
			throw ExcGetCoordinateMapperFailed( hr, mDeviceOptions.getDeviceId() );
		}
	}
}

void Device::stop()
{
	if ( mCoordinateMapper != 0 ) {
		mCoordinateMapper->Release();
		mCoordinateMapper = 0;
	}
	if ( mFrameReader != 0 ) {
		mFrameReader->Release();
		mFrameReader = 0;
	}
	if ( mSensor != 0 ) {
		long hr = mSensor->Close();
		if ( SUCCEEDED( hr ) && mSensor != 0 ) {
			mSensor->Release();
			mSensor = 0;
		}
	}
}

void Device::update()
{

	if ( mFrameReader == 0 ) {
		return;
	}

	IAudioBeamFrame* audioFrame								= 0;
	IBodyFrame* bodyFrame									= 0;
	IBodyIndexFrame* bodyIndexFrame							= 0;
	IColorFrame* colorFrame									= 0;
	IDepthFrame* depthFrame									= 0;
	IMultiSourceFrame* frame								= 0;
	IInfraredFrame* infraredFrame							= 0;
	ILongExposureInfraredFrame* infraredLongExposureFrame	= 0;
	
	HRESULT hr = mFrameReader->AcquireLatestFrame( &frame );
	
	if ( SUCCEEDED( hr ) && mDeviceOptions.isAudioEnabled() ) {
		// TODO audio	
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isBodyEnabled() ) {
		IBodyFrameReference* frameRef = 0;
		hr = frame->get_BodyFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &bodyFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isBodyIndexEnabled() ) {
		IBodyIndexFrameReference* frameRef = 0;
		hr = frame->get_BodyIndexFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &bodyIndexFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isColorEnabled() ) {
		IColorFrameReference* frameRef = 0;
		hr = frame->get_ColorFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &colorFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isDepthEnabled() ) {
		IDepthFrameReference* frameRef = 0;
		hr = frame->get_DepthFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &depthFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isInfraredEnabled() ) {
		IInfraredFrameReference* frameRef = 0;
		hr = frame->get_InfraredFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &infraredFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) && mDeviceOptions.isInfraredLongExposureEnabled() ) {
		ILongExposureInfraredFrameReference* frameRef = 0;
		hr = frame->get_LongExposureInfraredFrameReference( &frameRef );
		if ( SUCCEEDED( hr ) ) {
			hr = frameRef->AcquireFrame( &infraredLongExposureFrame );
		}
		if ( frameRef != 0 ) {
			frameRef->Release();
			frameRef = 0;
		}
	}

	if ( SUCCEEDED( hr ) ) {
		long long timeStamp										= 0L;

		// TODO audio

		std::vector<Body> bodies;
		int64_t bodyTime										= 0L;
		IBody* kinectBodies[ BODY_COUNT ]						= { 0 };
		
		Channel8u bodyIndexChannel;
		IFrameDescription* bodyIndexFrameDescription			= 0;
		int32_t bodyIndexWidth									= 0;
		int32_t bodyIndexHeight									= 0;
		uint32_t bodyIndexBufferSize							= 0;
		uint8_t* bodyIndexBuffer								= 0;
		int64_t bodyIndexTime									= 0L;
		
		Surface8u colorSurface;
		IFrameDescription* colorFrameDescription				= 0;
		int32_t colorWidth										= 0;
		int32_t colorHeight										= 0;
		ColorImageFormat colorImageFormat						= ColorImageFormat_None;
		uint32_t colorBufferSize								= 0;
		uint8_t* colorBuffer									= 0;

		Channel16u depthChannel;
		IFrameDescription* depthFrameDescription				= 0;
		int32_t depthWidth										= 0;
		int32_t depthHeight										= 0;
		uint16_t depthMinReliableDistance						= 0;
		uint16_t depthMaxReliableDistance						= 0;
		uint32_t depthBufferSize								= 0;
		uint16_t* depthBuffer									= 0;

		Channel16u infraredChannel;
		IFrameDescription* infraredFrameDescription				= 0;
		int32_t infraredWidth									= 0;
		int32_t infraredHeight									= 0;
		uint32_t infraredBufferSize								= 0;
		uint16_t* infraredBuffer								= 0;

		Channel16u infraredLongExposureChannel;
		IFrameDescription* infraredLongExposureFrameDescription	= 0;
		int32_t infraredLongExposureWidth						= 0;
		int32_t infraredLongExposureHeight						= 0;
		uint32_t infraredLongExposureBufferSize					= 0;
		uint16_t* infraredLongExposureBuffer					= 0;

		hr = depthFrame->get_RelativeTime( &timeStamp );

		// TODO audio
		if ( mDeviceOptions.isAudioEnabled() ) {

		}

		if ( mDeviceOptions.isBodyEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = bodyFrame->get_RelativeTime( &bodyTime );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = bodyFrame->GetAndRefreshBodyData( BODY_COUNT, kinectBodies );
			}
			if ( SUCCEEDED( hr ) ) {
				for ( uint8_t i = 0; i < 6; ++i ) {
					IBody* kinectBody = kinectBodies[ i ];
					if ( kinectBody != 0 ) {
						uint8_t isTracked	= false;
						hr					= kinectBody->get_IsTracked( &isTracked );
						if ( SUCCEEDED( hr ) && isTracked ) {
							Joint joints[ JointType_Count ];
							kinectBody->GetJoints( JointType_Count, joints );

							JointOrientation jointOrientations[ JointType_Count ];
							kinectBody->GetJointOrientations( JointType_Count, jointOrientations );

							uint64_t id = 0;
							kinectBody->get_TrackingId( &id );

							HandState leftHandState = HandState_Unknown;
                            HandState rightHandState = HandState_Unknown;
                            kinectBody->get_HandLeftState(&leftHandState);
                            kinectBody->get_HandRightState(&rightHandState);

							std::map<JointType, Body::Joint> jointMap;
							for ( int32_t j = 0; j < JointType_Count; ++j ) {
								Body::Joint joint( 
									toVec3f( joints[ j ].Position ), 
									toQuatf( jointOrientations[ j ].Orientation ), 
									joints[ j ].TrackingState
									);
								jointMap.insert( pair<JointType, Body::Joint>( static_cast<JointType>( j ), joint ) );
							}
							Body body(id, i, jointMap,leftHandState,rightHandState);
							bodies.push_back( body );
						}
					}
				}
			}
		}
		if ( SUCCEEDED( hr ) ) {
			Vector4 floorPlane;
			hr = bodyFrame->get_FloorClipPlane( &floorPlane );
			mFloorPlane = toVec4f(floorPlane);
		}

		if ( mDeviceOptions.isBodyIndexEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = bodyIndexFrame->get_RelativeTime( &bodyIndexTime );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = bodyIndexFrame->get_FrameDescription( &bodyIndexFrameDescription );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = bodyIndexFrameDescription->get_Width( &bodyIndexWidth );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = bodyIndexFrameDescription->get_Height( &bodyIndexHeight );
			}
			if ( SUCCEEDED( hr ) ) {
 				hr = bodyIndexFrame->AccessUnderlyingBuffer( &bodyIndexBufferSize, &bodyIndexBuffer );
			}
			if ( SUCCEEDED( hr ) ) {
				bodyIndexChannel = Channel8u( bodyIndexWidth, bodyIndexHeight );
				memcpy( bodyIndexChannel.getData(), bodyIndexBuffer, bodyIndexWidth * bodyIndexHeight * sizeof( uint8_t ) );
			}
		}

		if ( mDeviceOptions.isColorEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = colorFrame->get_FrameDescription( &colorFrameDescription );
				if ( SUCCEEDED( hr ) ) {
					float vFov = 0.0f;
					float hFov = 0.0f;
					float dFov = 0.0f;
					colorFrameDescription->get_VerticalFieldOfView( &vFov );
					colorFrameDescription->get_HorizontalFieldOfView( &hFov );
					colorFrameDescription->get_DiagonalFieldOfView( &dFov );
				}
			}
			if ( SUCCEEDED( hr ) ) {
				hr = colorFrameDescription->get_Width( &colorWidth );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = colorFrameDescription->get_Height( &colorHeight );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = colorFrame->get_RawColorImageFormat( &colorImageFormat );
			}
			if ( SUCCEEDED( hr ) ) {
				colorBufferSize = colorWidth * colorHeight * sizeof( uint8_t ) * 4;
				colorBuffer		= new uint8_t[ colorBufferSize ];
				hr = colorFrame->CopyConvertedFrameDataToArray( colorBufferSize, reinterpret_cast<uint8_t*>( colorBuffer ), ColorImageFormat_Rgba );
			
				if ( SUCCEEDED( hr ) ) {
					colorSurface = Surface8u( colorWidth, colorHeight, false, SurfaceChannelOrder::RGBA );
					memcpy( colorSurface.getData(), colorBuffer, colorWidth * colorHeight * sizeof( uint8_t ) * 4 );
				}

				delete [] colorBuffer;
				colorBuffer = 0;
			}
		}

		if ( mDeviceOptions.isDepthEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrame->get_FrameDescription( &depthFrameDescription );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrameDescription->get_Width( &depthWidth );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrameDescription->get_Height( &depthHeight );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrame->get_DepthMinReliableDistance( &depthMinReliableDistance );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrame->get_DepthMaxReliableDistance( &depthMaxReliableDistance );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = depthFrame->AccessUnderlyingBuffer( &depthBufferSize, &depthBuffer );
			}
			if ( SUCCEEDED( hr ) ) {
				depthChannel = Channel16u( depthWidth, depthHeight );
				memcpy( depthChannel.getData(), depthBuffer, depthWidth * depthHeight * sizeof( uint16_t ) );
			}
		}

		if ( mDeviceOptions.isInfraredEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = infraredFrame->get_FrameDescription( &infraredFrameDescription );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredFrameDescription->get_Width( &infraredWidth );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredFrameDescription->get_Height( &infraredHeight );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredFrame->AccessUnderlyingBuffer( &infraredBufferSize, &infraredBuffer );
			}
			if ( SUCCEEDED( hr ) ) {
				infraredChannel = Channel16u( infraredWidth, infraredHeight );
				memcpy( infraredChannel.getData(), infraredBuffer,  infraredWidth * infraredHeight * sizeof( uint16_t ) );
			}
		}

		if ( mDeviceOptions.isInfraredLongExposureEnabled() ) {
			if ( SUCCEEDED( hr ) ) {
				hr = infraredLongExposureFrame->get_FrameDescription( &infraredLongExposureFrameDescription );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredLongExposureFrameDescription->get_Width( &infraredLongExposureWidth );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredLongExposureFrameDescription->get_Height( &infraredLongExposureHeight );
			}
			if ( SUCCEEDED( hr ) ) {
				hr = infraredLongExposureFrame->AccessUnderlyingBuffer( &infraredLongExposureBufferSize, &infraredLongExposureBuffer );
			}
			if ( SUCCEEDED( hr ) ) {
				infraredLongExposureChannel = Channel16u( infraredLongExposureWidth, infraredLongExposureHeight );
				memcpy( infraredLongExposureChannel.getData(), infraredLongExposureBuffer, infraredLongExposureWidth * infraredLongExposureHeight * sizeof( uint16_t ) );
			}
		}

		if ( SUCCEEDED( hr ) ) {
			mFrame.mBodies						= bodies;
			mFrame.mChannelBodyIndex			= bodyIndexChannel;
			mFrame.mChannelDepth				= depthChannel;
			mFrame.mChannelInfrared				= infraredChannel;
			mFrame.mChannelInfraredLongExposure	= infraredLongExposureChannel;
			mFrame.mDeviceId					= mDeviceOptions.getDeviceId();
			mFrame.mSurfaceColor				= colorSurface;
			mFrame.mTimeStamp					= timeStamp;
		}

		if ( bodyIndexFrameDescription != 0 ) {
			bodyIndexFrameDescription->Release();
			bodyIndexFrameDescription = 0;
		}
		if ( colorFrameDescription != 0 ) {
			colorFrameDescription->Release();
			colorFrameDescription = 0;
		}
		if ( depthFrameDescription != 0 ) {
			depthFrameDescription->Release();
			depthFrameDescription = 0;
		}
		if ( infraredFrameDescription != 0 ) {
			infraredFrameDescription->Release();
			infraredFrameDescription = 0;
		}
		if ( infraredLongExposureFrameDescription != 0 ) {
			infraredLongExposureFrameDescription->Release();
			infraredLongExposureFrameDescription = 0;
		}
	}

	if ( audioFrame != 0 ) {
		audioFrame->Release();
		audioFrame = 0;
	}
	if ( bodyFrame != 0 ) {
		bodyFrame->Release();
		bodyFrame = 0;
	}
	if ( bodyIndexFrame != 0 ) {
		bodyIndexFrame->Release();
		bodyIndexFrame = 0;
	}
	if ( colorFrame != 0 ) {
		colorFrame->Release();
		colorFrame = 0;
	}
	if ( depthFrame != 0 ) {
		depthFrame->Release();
		depthFrame = 0;
	}
	if ( frame != 0 ) {
		frame->Release();
		frame = 0;
	}
	if ( infraredFrame != 0 ) {
		infraredFrame->Release();
		infraredFrame = 0;
	}
	if ( infraredLongExposureFrame != 0 ) {
		infraredLongExposureFrame->Release();
		infraredLongExposureFrame = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

const char* Device::Exception::what() const throw()
{
	return mMessage;
}

Device::ExcDeviceEnumerationFailed::ExcDeviceEnumerationFailed( long hr ) throw()
{
	sprintf( mMessage, "Unable to enumerate devices. Error: %i", hr );
}

Device::ExcDeviceInitFailed::ExcDeviceInitFailed( long hr, const string& id ) throw()
{
	sprintf( mMessage, "Device initialization failed. Device ID: %s. Error: %i", id, hr );
}

Device::ExcDeviceNotAvailable::ExcDeviceNotAvailable( long hr ) throw()
{
	sprintf( mMessage, "No devices are available. Error: %i", hr );
}

Device::ExcDeviceOpenFailed::ExcDeviceOpenFailed( long hr, const string& id ) throw()
{
	sprintf( mMessage, "Unable to open device. Device ID: %s. Error: %i", id, hr );
}

Device::ExcGetCoordinateMapperFailed::ExcGetCoordinateMapperFailed( long hr, const string& id ) throw()
{
	sprintf( mMessage, "Unable to get device coordinate mapper. Device ID: %s. Error: %i", id, hr );
}

Device::ExcOpenFrameReaderFailed::ExcOpenFrameReaderFailed( long hr, const string& id ) throw()
{
	sprintf( mMessage, "Unable to open frame reader. Device ID: %s. Error: %i", id, hr );
}

}
