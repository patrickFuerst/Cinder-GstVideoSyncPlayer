#pragma once

#include "cinder/gl/gl.h"
#include "cinder/linux/GstPlayer.h"
#include <gst/net/gstnet.h>
#include "Osc.h"

using namespace ci;
using namespace ci::app;
using namespace gst::video;
using namespace std;

typedef std::map<uint64_t, osc::SenderTcp> Clients;
typedef Clients::iterator clientsIter;
typedef std::map<std::string, clientsIter > ClientsAdressCache;

class GstVideoServer : public GstPlayer{

    public:
	
		GstVideoServer();
        ~GstVideoServer();

        void                            init( const std::string _clockIp, const uint16_t _clockPort, const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort);
        //void                            loadAsync( const fs::path& path );
        void                            load( const fs::path& path );
        void                            play();
        //void                            draw( vec2 _pos, float _width = -1, float _height = -1 );
        //void                            drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy );
        //void                            loop( bool _loop );
        void                            pause();
        gl::Texture2dRef                getTexture();
        //void                          exit(ofEventArgs & args);
        //void                          setPixelFormat( const ofPixelFormat & _pixelFormat );
        const Clients&                  getConnectedClients();
    protected:

        Clients                         mConnectedClients; ///> Our connected clients.
		ClientsAdressCache    			mConnectedAdressedClients;
		shared_ptr<osc::ReceiverTcp>    mOscReceiver;      ///> osc receiver.

        bool                            mInitialized;      ///> If the player initialized properly ??
    private:

        //------------------ ERROR Handling ---------------
        void                            socketError( const asio::error_code &error, uint64_t identifier );

        //------------------ MASTER -----------------------
        void                            clientAccepted( osc::TcpSocketRef socket, uint64_t identifier   );
        void                            sendToClients(const osc::Message &m);
        void                            sendToClient(const osc::Message &m, const std::string &address);
        void                            sendToClient(const osc::Message &m, const asio::ip::address &address);

        void                            setupNetworkClock();

        const osc::Message              getPauseMsg() const;
        const osc::Message              getPlayMsg() const;
        const osc::Message              getLoopMsg() const;
        const osc::Message              getEosMsg() const;

        //void                            clientLoadedMessage(const osc::Message &message );
        void                            clientExitedMessage(const osc::Message &message );

        //void                            resetBaseTime();
        void                            movieEnded();

    private:

        GstClock*                       mGstClock;         ///> The network clock.
        GstElement*                     mGstPipeline;      ///> The running pipeline.
        GstClockTime                    mGstBaseTime;     ///> The base time.
        std::string                     mClockIp;          ///> The IP of the server.
        uint16_t                        mClockPort;        ///> The port that should be used for the synchronization.
        uint16_t                        mServerRcvPort;    ///> osc communication.
        uint16_t                        mClientRcvPort;     ///> osc communication.
        gint64                          mPos;              ///> Position of the player.
        signals::Connection             mMovieEndedConnection;
	
		std::string 					mCurrentFileName;
};
