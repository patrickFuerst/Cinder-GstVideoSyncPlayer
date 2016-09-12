#pragma once

#include "cinder/gl/gl.h"
#include "cinder/linux/GstPlayer.h"
#include <gst/net/gstnet.h>
#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace gst::video;
using namespace std;

class GstVideoClient : public GstPlayer{

    public:

        GstVideoClient();
        ~GstVideoClient();

        void                            init( const std::string _clockIp, const uint16_t _clockPort,
											  const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort );
        
        void                            update(); 
        //void                            loadAsync( const fs::path& path );
        //void                            draw( vec2 _pos, float _width = -1, float _height = -1 );
        //void                            drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy );
        gl::Texture2dRef                getTexture();
        //void                          exit(ofEventArgs & args);
        //void                          setPixelFormat( const ofPixelFormat & _pixelFormat );
    protected:

        shared_ptr<osc::SenderTcp>      mOscSender;        ///> osc sender for slave.
        shared_ptr<osc::ReceiverTcp>    mOscReceiver;      ///> osc receiver.

        bool                            mInitialized;      ///> If the player initialized properly ??
    private:
	
		void                            socketErrorReceiver( const asio::error_code &error, uint64_t identifier );
		void 							socketErrorSender(const asio::error_code & error, const std::string & oscAddress );
		void                            initClock( GstClockTime _baseTime );
		void                            setClock();
        void                            setBaseTime( GstClockTime _baseTime );
        
		
		void                            playMessage(const osc::Message &message );
        void                            pauseMessage(const osc::Message &message );
        void                            stopMessage(const osc::Message &message );
        void                            loopMessage(const osc::Message &message );
        void                            eosMessage(const osc::Message &message );
		void                            initTimeMessage(const osc::Message &message );
		void                            loadFileMessage(const osc::Message &message );
	
		void                            load( const fs::path& path );
		

    private:

        GstClock*                       mGstClock;         ///> The network clock.
        GstElement*                     mGstPipeline;      ///> The running pipeline.
        GstClockTime                    mGstBaseTime;     ///> The base time.
        std::string                     mClockIp;          ///> The IP of the server.
        uint16_t                        mClockPort;        ///> The port that should be used for the synchronization.
        uint16_t                        mServerRcvPort;    ///> osc communication.
        uint16_t                        mClientRcvPort;     ///> osc communication.
        int32_t                         mUniqueClientId;    ///> osc communication.
        gint64                          mPos;              ///> Position of the player.
        bool                            mLoopMessage; 

};
