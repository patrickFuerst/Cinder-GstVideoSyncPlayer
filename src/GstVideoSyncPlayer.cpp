
#include "GstVideoSyncPlayer.h"

#define TCP_SENDER_PORT 10000
using namespace asio;
using namespace asio::ip;

GstVideoSyncPlayer::GstVideoSyncPlayer()
    : m_gstClock(NULL)
    , m_gstPipeline(NULL)
    , m_gstClockTime(0)
    , m_isMaster(false)
    , m_clockIp("127.0.0.1")
    , m_clockPort(1234)
    , m_masterRcvPort(m_clockPort+1)
    , m_slaveRcvPort(m_clockPort+2)
    , m_loop(false)
    , m_initialized(false)
    , m_movieEnded(false)
    , m_paused(true)
    , mUniqueClientId(0)
{
}

GstVideoSyncPlayer::~GstVideoSyncPlayer()
{

    if( m_isMaster && m_initialized ){
        mMovieEndedConnection.disconnect();
        m_initialized = false;
    }
    else if( !m_isMaster && m_initialized ){
        osc::Message m("/client-exited");
        m.append(mUniqueClientId);
         if( m_oscSender ){
             m_oscSender->send(m);
         }
         m_initialized = false;
    }
}

//void GstVideoSyncPlayer::exit(ofEventArgs & args)
//{
//    //> Triggered when app exits.
//    if( !m_isMaster && m_initialized ){
//        osc::Message m("/client-exited");
//        m.setAddress("/client-exited");
//        m.addInt64Arg(m_rcvPort);
//        if( m_oscSender ){
//            m_oscSender->sendMessage(m,false);
//        }
//        m_initialized = false;
//    }
//}

void GstVideoSyncPlayer::movieEnded()
{
    if( !m_movieEnded ){
        console() << "GstVideoSyncPlayer: Movie ended.. " << std::endl;
        m_movieEnded = true;
        m_paused = true;
        sendToClients( getEosMsg() );
    }
}

void GstVideoSyncPlayer::loop( bool _loop )
{
    m_loop = _loop;
}

void GstVideoSyncPlayer::initAsMaster( const std::string _clockIp, const uint16_t _clockPort,  const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort)
{
    if( m_initialized ){
        console() << " Player already initialized as MASTER with Ip : " << _clockIp << std::endl;
        return;
    }

    m_clockIp = _clockIp;
    m_clockPort = _clockPort;

    m_isMaster = true;

    m_masterRcvPort = _oscMasterRcvPort;
    m_slaveRcvPort = _oscSlaveRcvPort;

   // m_oscSender = shared_ptr<osc::SenderTcp>(new osc::SenderTcp( TCP_SENDER_PORT,m_clockIp, m_rcvPort ));
   // m_oscSender->bind();
   // m_oscSender->connect();

    m_oscReceiver = shared_ptr<osc::ReceiverTcp>(new osc::ReceiverTcp(m_masterRcvPort));
    m_oscReceiver->setOnAcceptFn( std::bind(&GstVideoSyncPlayer::clientAccepted, this, std::placeholders::_1,std::placeholders::_2) );
    m_oscReceiver->setListener("/client-loaded" , std::bind(&GstVideoSyncPlayer::clientLoadedMessage , this, std::placeholders::_1 ));
    m_oscReceiver->setListener("/client-exited" , std::bind(&GstVideoSyncPlayer::clientExitedMessage , this, std::placeholders::_1 ));
    m_oscReceiver->bind();
    m_oscReceiver->listen();

    getEndedSignal().connect( std::bind( &GstVideoSyncPlayer::movieEnded , this ));

    m_initialized = true;
}

void GstVideoSyncPlayer::initAsSlave( const std::string _clockIp, const uint16_t _clockPort,  const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort)
{
    if( m_initialized ){
        console() << " Player already initialized as SLAVE with Ip : " << _clockIp << std::endl;
        return;
    }

    m_clockIp = _clockIp;
    m_clockPort = _clockPort;

    m_isMaster = false;

    m_masterRcvPort = _oscMasterRcvPort;
    m_slaveRcvPort = _oscSlaveRcvPort;

    m_oscSender = shared_ptr<osc::SenderTcp>(new osc::SenderTcp(TCP_SENDER_PORT,m_clockIp,m_masterRcvPort));
    m_oscSender->bind();
    m_oscSender->connect();

    m_oscReceiver = shared_ptr<osc::ReceiverTcp>(new osc::ReceiverTcp(m_slaveRcvPort));

    m_oscReceiver->setListener("/play" , std::bind(&GstVideoSyncPlayer::playMessage , this, std::placeholders::_1 ));
    m_oscReceiver->setListener("/pause" , std::bind(&GstVideoSyncPlayer::pauseMessage , this, std::placeholders::_1 ));
    m_oscReceiver->setListener("/loop" , std::bind(&GstVideoSyncPlayer::loopMessage , this, std::placeholders::_1 ));
    m_oscReceiver->setListener("/eos" , std::bind(&GstVideoSyncPlayer::eosMessage , this, std::placeholders::_1 ));
    m_oscReceiver->setListener("/init-time" , std::bind(&GstVideoSyncPlayer::initTimeMessage , this, std::placeholders::_1 ));

    m_oscReceiver->bind();
    m_oscReceiver->listen();

}

//void GstVideoSyncPlayer::loadAsync( std::string _path )
//{
//    m_videoPlayer.loadAsync(_path);
//
//    ///> Now that we have loaded we can grab the pipeline..
//    m_gstPipeline = m_gstPlayer.getPipeline();
//
//    ///> Since we will provide a network clock for the synchronization
//    ///> we disable here the internal handling of the base time.
//    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);
//
//    if( !m_isMaster ){
//        osc::Message m("/client-loaded");
//         m.append((int64_t)m_rcvPort);
//         if( m_oscSender ){
//            m_oscSender->sendMessage(m,false);
//         }
//    }
//
//}

bool GstVideoSyncPlayer::load( const fs::path& path )
{

    MovieBase::initFromPath(path);

    ///> Now that we have loaded we can grab the pipeline..
    m_gstPipeline = mGstPlayer->getPipeline();

    ///> Since we will provide a network clock for the synchronization
    ///> we disable here the internal handling of the base time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);

    if( m_isMaster ){
        ///> Remove any previously used clock.
        if(m_gstClock) g_object_unref((GObject*) m_gstClock);
        m_gstClock = NULL;

        ///> Grab the pipeline clock.
        m_gstClock = gst_pipeline_get_clock(GST_PIPELINE(m_gstPipeline));
    
        ///> Set this clock as the master network provider clock.
        ///> The slaves are going to poll this clock through the network
        ///> and adjust their times based on this clock and their local observations.
        gst_net_time_provider_new(m_gstClock, m_clockIp.c_str(), m_clockPort);
    }

    if( !m_isMaster ){
        osc::Message m;
         m.setAddress("/client-loaded");
         m.append(mUniqueClientId);
        if( m_oscSender ){
            m_oscSender->send(m);
         }
    }

    //return _loaded;
}


void GstVideoSyncPlayer::update()
{
    if( m_isMaster && m_loop && m_movieEnded ){
        

        ///> Get ready to start over..
        gst_element_set_state(m_gstPipeline, GST_STATE_READY);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> Set the master clock i.e This the clock that the slaves will poll 
        ///> in order to keep in-sync.
        setMasterClock();

        ///> ..and start playing the master..
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> Let the slaves know that they should start over..
        sendToClients( getLoopMsg() );

        m_movieEnded = false;
        m_paused = false;
    }

}

void GstVideoSyncPlayer::play()
{
    if( !m_isMaster || !m_paused ) return;

    setMasterClock();

    if( m_movieEnded ){
        ///> Get ready to start over..
        gst_element_set_state(m_gstPipeline, GST_STATE_READY);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        ///> ..and start playing the master..
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        sendToClients( getLoopMsg() );
    }
    else{
        gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        sendToClients( getPlayMsg() );
    }

    m_movieEnded = false;
    m_paused = false;

}

void GstVideoSyncPlayer::clientAccepted( osc::TcpSocketRef socket, uint64_t identifier  ){

    std::string addressString = socket->remote_endpoint().address().to_string();
    console() << "New Client with ip address " << addressString <<" on port " << socket->remote_endpoint().port() << std::endl;

    auto element = m_connectedClients.emplace( std::piecewise_construct, std::forward_as_tuple(addressString) , std::forward_as_tuple(TCP_SENDER_PORT, addressString , m_slaveRcvPort)  );
    if( element.second ){
        element.first->second.bind();
        element.first->second.connect();
    }else{
        console() << "Client with address: " << addressString << " already exists." << std::endl;
    }

}

void GstVideoSyncPlayer::setMasterClock()
{
    if( !m_isMaster ) return;


    ///> Be explicit on which clock we are going to use.
    gst_pipeline_use_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);
    gst_pipeline_set_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);


    ///> Grab the base time..
    m_gstClockTime = gst_clock_get_time(m_gstClock);

    ///> and explicitely set it for this pipeline after disabling the internal handling of time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(m_gstPipeline, m_gstClockTime);
}

void GstVideoSyncPlayer::setClientClock( GstClockTime _baseTime )
{
    if( m_isMaster ) return;

    ///> The arrived master base_time.
    m_gstClockTime = _baseTime;

    ///> Remove any previously used clock.
    if(m_gstClock) g_object_unref((GObject*) m_gstClock);
    m_gstClock = NULL;

    ///> Create the slave network clock with an initial time.
    m_gstClock = gst_net_client_clock_new(NULL, m_clockIp.c_str(), m_clockPort, m_gstClockTime);

    ///> Be explicit.
    gst_pipeline_use_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);
    gst_pipeline_set_clock(GST_PIPELINE(m_gstPipeline), m_gstClock);

    ///> Disable internal handling of time and set the base time.
    gst_element_set_start_time(m_gstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(m_gstPipeline,  m_gstClockTime);

}

//void GstVideoSyncPlayer::draw( ofPoint _pos, float _width, float _height )
//{
//    if( !m_videoPlayer.getPixels().isAllocated() || !m_videoPlayer.getTexture().isAllocated() ) return;
//
//    if( _width == -1 || _height == -1 ){
//        m_videoPlayer.draw(_pos);
//    }
//    else{
//        m_videoPlayer.draw(_pos, _width, _height);
//    }
//}

//void GstVideoSyncPlayer::drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy )
//{
//    m_videoPlayer.getTexture().drawSubsection(_x, _y, _w, _h, _sx, _sy);
//}


void GstVideoSyncPlayer::sendToClients(const osc::Message &m)
{
    for( auto& clientKey : m_connectedClients){
        auto& client = clientKey.second;
        console() << "Sending " << m.getAddress() << " to " << client.getRemoteEndpoint().address() << ":" << client.getRemoteEndpoint().port() << std::endl;
        client.send(m);
    }
}
void GstVideoSyncPlayer::sendToClient( const osc::Message &m, const asio::ip::address &address )
{
    sendToClient(m,address.to_string());
}

void GstVideoSyncPlayer::sendToClient( const osc::Message &m, const std::string &address )
{
    const auto& clientKey = m_connectedClients.find(address);
    if(clientKey != m_connectedClients.end() ){
        auto& client = clientKey->second;
        console() << "Sending " << m.getAddress() << " to " << client.getRemoteEndpoint().address() << ":" << client.getRemoteEndpoint().port() << std::endl;
        client.send(m);
    } else{
        console() << "Couldn't find client with address: " << address << std::endl;
    }
}


void GstVideoSyncPlayer::pause()
{
    if( !m_isMaster ) return;

    if( !m_paused ){
        if( !isLoaded() ){
            console() << " Cannot pause non loaded video ! " <<std::endl;
            return;
        }

        gst_element_set_state(m_gstPipeline, GST_STATE_PAUSED);
        gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        
        gst_element_query_position(GST_ELEMENT(m_gstPipeline),GST_FORMAT_TIME,&m_pos);

        GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
        
        if (!gst_element_seek_simple (m_gstPipeline, GST_FORMAT_TIME, _flags, m_pos)) {
                console() << "Pausing seek failed" << std::endl;
        }

        m_paused = true;
        
        sendToClients( getPauseMsg() );
    }
}

const osc::Message GstVideoSyncPlayer::getPauseMsg() const
{
    osc::Message m;
    m.setAddress("/pause");
    m.append((int64_t)m_pos);
    return m;
}

const osc::Message GstVideoSyncPlayer::getPlayMsg() const
{
    osc::Message m;
    m.setAddress("/play");
    m.append((int64_t)m_gstClockTime);
    return m;

}

const osc::Message GstVideoSyncPlayer::getLoopMsg() const
{
    osc::Message m;
    m.setAddress("/loop");
    m.append((int64_t)m_gstClockTime);
    return m;

}

const osc::Message GstVideoSyncPlayer::getEosMsg() const
{
    osc::Message m;
    m.setAddress("/eos");
    return m;

}

void GstVideoSyncPlayer::clientLoadedMessage(const osc::Message &message ){

    ///> If the video loading has failed on the master there is little we can do here...
    if(!isLoaded()){
        console() << " ERROR: Client loaded but master NOT !! Playback wont work properly... " << std::endl;
        return;
    }
    console() << "GstVideoSyncPlayer: New client loaded" << std::endl;

    ///> If the master is paused when the client connects pause the client also.
    if( m_paused ){
        sendToClient(getPauseMsg(), message.getSenderIpAddress() );
        return;
    }

    osc::Message m;
    m.setAddress("/client-init-time");
    m.append((int64_t)m_gstClockTime);
    m.append((int64_t)m_pos);
    sendToClients(m);

}


void GstVideoSyncPlayer::clientExitedMessage(const osc::Message &message ){

    console() <<"GstVideoSyncPlayer: Disconnecting client" << std::endl;
   // ClientKey _clientExit(_exitingClient, _exitingClientPort);
    //clients_iter it = m_connectedClients.find(_clientExit);

    //if( it != m_connectedClients.end() ){
      //  auto temp = it;
        //console() <<"GstVideoSyncPlayer: Disconnecting client with IP : " << _exitingClient << " and port : " << _exitingClientPort << std::endl;
       // m_connectedClients.erase(temp);
   // }

}
void GstVideoSyncPlayer::initTimeMessage(const osc::Message &message ){
    console() <<"GstVideoSyncPlayer:  CLIENT RECEIVED MASTER INIT TIME! " <<std::endl;
    m_pos = message.getArgInt64(1);

    ///> Initial base time for the clients.
    ///> Set the slave network clock that is going to poll the master.
    setClientClock((GstClockTime)message.getArgInt64(0));

    ///> And start playing..
    gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    m_paused = false;
    m_movieEnded = false;
}
void GstVideoSyncPlayer::playMessage(const osc::Message &message ){
    console() << "GstVideoSyncPlayer: CLIENT ---> PLAY " << std::endl;

    ///> Set the base time of the slave network clock.
    setClientClock(message.getArgInt64(0));

    ///> And start playing..
    gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    m_paused = false;
    m_movieEnded =false;

}
void GstVideoSyncPlayer::pauseMessage(const osc::Message &message ){
    m_pos = message.getArgInt64(0);
    console() << "GstVideoSyncPlayer: CLIENT ---> PAUSE " << std::endl;

    gst_element_set_state(m_gstPipeline, GST_STATE_PAUSED);
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    ///> This needs more thinking but for now it gives acceptable results.
    ///> When we pause, we seek to the position of the master when paused was called.
    ///> If we dont do this there is a delay before the pipeline starts again i.e when hitting play() again after pause()..
    ///> I m pretty sure this can be done just by adjusting the base_time based on the position but
    ///> havent figured it out exactly yet..
    GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

    if( !gst_element_seek_simple (m_gstPipeline, GST_FORMAT_TIME, _flags, m_pos )) {
        console () << "Pausing seek failed" << std::endl;
    }

    m_paused = true;


}
void GstVideoSyncPlayer::loopMessage(const osc::Message &message ){

    console() << "GstVideoSyncPlayer: CLIENT ---> LOOP " << std::endl;

    ///> Get ready to start over..
    gst_element_set_state(m_gstPipeline, GST_STATE_NULL);
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    ///> Set the slave clock and base_time.
    setClientClock(message.getArgInt64(0));

    ///> and go..
    gst_element_set_state(m_gstPipeline, GST_STATE_PLAYING);
    gst_element_get_state(m_gstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    m_movieEnded = false;
    m_paused = false;
    
}
void GstVideoSyncPlayer::eosMessage(const osc::Message &message ){
    console() << "GstVideoSyncPlayer CLIENT ---> EOS " << std::endl;
    m_movieEnded = true;
    m_paused = true;
}
//void GstVideoSyncPlayer::initMessage(const osc::Message &message ){
//    console() << "GstVideoSyncPlayer CLIENT ---> INIT with " << message.getArgInt32(0) << std::endl;
//    mUniqueClientId = message.getArgInt32(0);
//    m_initialized = true;
//}
ci::gl::Texture2dRef GstVideoSyncPlayer::getTexture()
{
    return mGstPlayer->getVideoTexture();
}

bool GstVideoSyncPlayer::isPaused()
{
    return m_paused;
}

bool GstVideoSyncPlayer::isMovieEnded()
{
    return m_movieEnded;
}

bool GstVideoSyncPlayer::isMaster()
{
    return m_isMaster;
}

//void GstVideoSyncPlayer::setPixelFormat( const ofPixelFormat & _pixelFormat )
//{
//    m_videoPlayer.setPixelFormat(_pixelFormat);
//}
//
//const Clients& GstVideoSyncPlayer::getConnectedClients()
//{
//    return m_connectedClients;
//}
