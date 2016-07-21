#pragma once

#include "cinder/gl/gl.h"
#include "cinder/linux/GstPlayer.h"
#include <gst/net/gstnet.h>
#include "Osc.h"

using namespace ci;
using namespace ci::app;
using namespace gst::video;
using namespace std;

typedef std::map<std::string, osc::SenderTcp> Clients;
typedef Clients::iterator clients_iter;

class GstVideoSyncPlayer : public GstPlayer{

    public:

        GstVideoSyncPlayer();
        ~GstVideoSyncPlayer();

        void                            initAsMaster( const std::string _clockIp, const uint16_t _clockPort, const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort);
        void                            initAsSlave( const std::string _clockIp, const uint16_t _clockPort, const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort );
        void                            loadAsync( const fs::path& path );
        void                            load( const std::string& path );
        void                            play();
        void                            update();
        void                            draw( vec2 _pos, float _width = -1, float _height = -1 );
        void                            drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy );
        void                            loop( bool _loop );
        void                            pause();
        gl::Texture2dRef                getTexture();
        bool                            isPaused();
        bool                            isMovieEnded();
        bool                            isMaster();
        //void                          exit(ofEventArgs & args);
        //void                          setPixelFormat( const ofPixelFormat & _pixelFormat );
        const Clients&                  getConnectedClients();
    protected:

        Clients                         m_connectedClients; ///> Our connected clients.
        shared_ptr<osc::SenderTcp>      m_oscSender;        ///> osc sender for slave.
        shared_ptr<osc::ReceiverTcp>    m_oscReceiver;      ///> osc receiver.

        bool                            m_isMaster;         ///> Is the master?
        bool                            m_initialized;      ///> If the player initialized properly ??
    private:

        //------------------ ERROR Handling ---------------
        void                            socketErrorMaster( const asio::error_code &error, uint64_t identifier, const osc::ReceiverTcp::protocol::endpoint &endpoint );
        void                            socketErrorSlave( const asio::error_code &error, uint64_t identifier, const osc::ReceiverTcp::protocol::endpoint &endpoint );

        //------------------ MASTER -----------------------
        void                            clientAccepted( osc::TcpSocketRef socket, uint64_t identifier   );
        void                            sendToClients(const osc::Message &m);
        void                            sendToClient(const osc::Message &m, const std::string &address);
        void                            sendToClient(const osc::Message &m, const asio::ip::address &address);

        void                            setMasterClock();

        const osc::Message              getPauseMsg() const;
        const osc::Message              getPlayMsg() const;
        const osc::Message              getLoopMsg() const;
        const osc::Message              getEosMsg() const;

        void                            clientLoadedMessage(const osc::Message &message );
        void                            clientExitedMessage(const osc::Message &message );

        //------------------ SLAVE -----------------------

        void                            setClientClock( GstClockTime _baseTime );
        void                            setClientBaseTime( GstClockTime _baseTime );
        void                            playMessage(const osc::Message &message );
        void                            pauseMessage(const osc::Message &message );
        void                            loopMessage(const osc::Message &message );
        void                            eosMessage(const osc::Message &message );
        void                            initTimeMessage(const osc::Message &message );

        void                            movieEnded();

    private:

        GstClock*                       m_gstClock;         ///> The network clock.
        GstElement*                     m_gstPipeline;      ///> The running pipeline.
        GstClockTime                    mGstBaseTime;     ///> The base time.
        std::string                     m_clockIp;          ///> The IP of the server.
        uint16_t                        m_clockPort;        ///> The port that should be used for the synchronization.
        uint16_t                        m_masterRcvPort;    ///> osc communication.
        uint16_t                        m_slaveRcvPort;     ///> osc communication.
        int32_t                         mUniqueClientId;    ///> osc communication.
        bool                            m_loop;             ///> Should we loop?
        bool                            m_movieEnded;       ///> Has the video ended??
        gint64                          m_pos;              ///> Position of the player.
        bool                            m_paused;           ///> Is the player paused ??
        signals::Connection             mMovieEndedConnection;
};
