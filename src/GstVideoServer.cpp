
#include "GstVideoServer.h"

#define TCP_SENDER_PORT 10000
using namespace asio;
using namespace asio::ip;

GstVideoServer::GstVideoServer()
    : mGstClock(NULL)
    , mGstPipeline(NULL)
    , mGstBaseTime(0)
    , mClockIp("127.0.0.1")
    , mClockPort(1234)
    , mServerRcvPort(mClockPort+1)
    , mClientRcvPort(mClockPort+2)
    , mInitialized(false)
{
    GstPlayer::initialize();
}

GstVideoServer::~GstVideoServer()
{

    if( mInitialized ){
        mMovieEndedConnection.disconnect();
        mInitialized = false;
    }
    
    if(mOscReceiver)
        mOscReceiver->close();

    for(auto it = mConnectedClients.begin(); it != mConnectedClients.end(); it++){
        it->second.close();
    }
	
	if(mGstClock)
		g_object_unref( GST_OBJECT(mGstClock) );
	mGstClock = nullptr;
}

//void GstVideoServer::exit(ofEventArgs & args)
//{
//    //> Triggered when app exits.
//    if( !m_isMaster && mInitialized ){
//        osc::Message m("/client-exited");
//        m.setAddress("/client-exited");
//        m.addInt64Arg(m_rcvPort);
//        if( mOscSender ){
//            mOscSender->sendMessage(m,false);
//        }
//        mInitialized = false;
//    }
//}

void GstVideoServer::movieEnded()
{
    console() << "GstVideoServer: Movie ended.. " << std::endl;
    if( ! GstPlayer::isPaused() ){
        resetBaseTime();
        sendToClients( getLoopMsg() );
    }else{
        sendToClients( getEosMsg() );
    }


}

void GstVideoServer::init( const std::string _clockIp, const uint16_t _clockPort,  const uint16_t _oscMasterRcvPort, const uint16_t _oscSlaveRcvPort)
{
    if( mInitialized ){
        console() << " Player already initialized as MASTER with Ip : " << _clockIp << std::endl;
        return;
    }

    mClockIp = _clockIp;
    mClockPort = _clockPort;
	mServerRcvPort = _oscMasterRcvPort;
    mClientRcvPort = _oscSlaveRcvPort;
	
    mOscReceiver = shared_ptr<osc::ReceiverTcp>(new osc::ReceiverTcp(mServerRcvPort));
    mOscReceiver->setSocketTransportErrorFn( std::bind(&GstVideoServer::socketError, this, std::placeholders::_1,std::placeholders::_2 ) );
    mOscReceiver->setOnAcceptFn( std::bind(&GstVideoServer::clientAccepted, this, std::placeholders::_1,std::placeholders::_2) );
    mOscReceiver->setListener("/client-loaded" , std::bind(&GstVideoServer::clientLoadedMessage , this, std::placeholders::_1 ));
    mOscReceiver->setListener("/client-exited" , std::bind(&GstVideoServer::clientExitedMessage , this, std::placeholders::_1 ));
    mOscReceiver->bind();
    mOscReceiver->listen();

    getEndedSignal().connect( std::bind( &GstVideoServer::movieEnded , this ));

    mInitialized = true;
}

//void GstVideoServer::loadAsync( std::string _path )
//{
//    m_videoPlayer.loadAsync(_path);
//
//    ///> Now that we have loaded we can grab the pipeline..
//    mGstPipeline = m_gstPlayer.getPipeline();
//
//    ///> Since we will provide a network clock for the synchronization
//    ///> we disable here the internal handling of the base time.
//    gst_element_set_start_time(mGstPipeline, GST_CLOCK_TIME_NONE);
//
//    if( !m_isMaster ){
//        osc::Message m("/client-loaded");
//         m.append((int64_t)m_rcvPort);
//         if( mOscSender ){
//            mOscSender->sendMessage(m,false);
//         }
//    }
//
//}

void GstVideoServer::load( const fs::path& path )
{

    GstPlayer::load(path.string());
	mCurrentFileName = path.filename().string();
    ///> Now that we have loaded we can grab the pipeline..
    mGstPipeline = GstPlayer::getPipeline();

	setupNetworkClock();
    
}

void GstVideoServer::play()
{
    if( !GstPlayer::isPaused() ) return;

    resetBaseTime();
    GstPlayer::play();
    sendToClients( getPlayMsg() );

}

void GstVideoServer::socketError(  const asio::error_code &error, uint64_t identifier )
{
    auto clientEntry = mConnectedClients.find( identifier );
    if(clientEntry != mConnectedClients.end() ){
        auto& client = clientEntry->second;
		auto address = client.getRemoteEndpoint().address().to_string();
        console() << "Socket Error for Client with ip address " << address <<". Error:  " << error.message() << std::endl;
		client.shutdown();
        client.close();
		mConnectedAdressedClients.erase(address);
        mConnectedClients.erase(clientEntry);
        mOscReceiver->closeConnection(identifier);
    }else{
        console() << "Couldn't find client for id: " << identifier << " and errror: " << error.message() << std::endl;
    }

}

void GstVideoServer::clientAccepted( osc::TcpSocketRef socket, uint64_t identifier  ){

    std::string addressString = socket->remote_endpoint().address().to_string();
    console() << "New Client with ip address " << addressString <<" on port " << socket->local_endpoint().port() << std::endl;
	
	auto element = mConnectedClients.emplace( std::piecewise_construct, std::forward_as_tuple(identifier) , std::forward_as_tuple(TCP_SENDER_PORT, addressString , mClientRcvPort)  );
	//auto element = mConnectedClients.emplace( std::piecewise_construct, std::forward_as_tuple(addressString) , std::forward_as_tuple( socket )  );
    if( element.second ){

		mConnectedAdressedClients.emplace(addressString, element.first  );
		
		element.first->second.setOnConnectFn(
				[this]( osc::TcpSocketRef socketRef){
					console() << "Connected to new Client." << std::endl;
					
					osc::Message m;
					m.setAddress("/init-time");
					m.append((int64_t)mGstBaseTime);
					sendToClient(m, socketRef->remote_endpoint().address().to_string());
					
					m.clear();
					m.setAddress("/load-file");
					m.append(mCurrentFileName);
					sendToClient(m , socketRef->remote_endpoint().address().to_string() );
				}
		);
		
		
		element.first->second.bind();
        element.first->second.connect();
		
    }else{
        console() << "Client with address: " << addressString << " already exists." << std::endl;
    }

}

void GstVideoServer::setupNetworkClock()
{
    ///> Remove any previously used clock.
    if(mGstClock) g_object_unref((GObject*) mGstClock);
    mGstClock = NULL;

    ///> Grab the pipeline clock.
    mGstClock = gst_pipeline_get_clock(GST_PIPELINE(mGstPipeline));

    ///> Set this clock as the master network provider clock.
    ///> The slaves are going to poll this clock through the network
    ///> and adjust their times based on this clock and their local observations.
    gst_net_time_provider_new(mGstClock, mClockIp.c_str(), mClockPort);

    ///> Be explicit on which clock we are going to use.
    gst_pipeline_use_clock(GST_PIPELINE(mGstPipeline), mGstClock);
    gst_pipeline_set_clock(GST_PIPELINE(mGstPipeline), mGstClock);

}

void GstVideoServer::resetBaseTime()
{
    mGstBaseTime = gst_clock_get_time(mGstClock);
    ///> Since we will provide a network clock for the synchronization
    ///> we disable here the internal handling of the base time.
    gst_element_set_start_time(mGstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(mGstPipeline, mGstBaseTime);
}
//void GstVideoServer::draw( ofPoint _pos, float _width, float _height )
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

//void GstVideoServer::drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy )
//{
//    m_videoPlayer.getTexture().drawSubsection(_x, _y, _w, _h, _sx, _sy);
//}


void GstVideoServer::sendToClients(const osc::Message &m)
{
    for( auto& clientKey : mConnectedClients){
        auto& client = clientKey.second;
        console() << "Sending " << m.getAddress() << " to " << client.getRemoteEndpoint().address() << ":" << client.getRemoteEndpoint().port() << std::endl;
        client.send(m);
    }
}
void GstVideoServer::sendToClient( const osc::Message &m, const asio::ip::address &address )
{
    sendToClient(m,address.to_string());
}

void GstVideoServer::sendToClient( const osc::Message &m, const std::string &address )
{
    const auto& clientKey = mConnectedAdressedClients.find(address);
    if(clientKey != mConnectedAdressedClients.end() ){
        auto& client = clientKey->second->second;
        console() << "Sending " << m.getAddress() << " to " << client.getRemoteEndpoint().address() << ":" << client.getRemoteEndpoint().port() << std::endl;
        client.send(m);
    } else{
        console() << "Couldn't find client with address: " << address << std::endl;
    }
}


void GstVideoServer::pause()
{
	stop();
	gst_element_query_position(GST_ELEMENT(mGstPipeline),GST_FORMAT_TIME,&mPos);
	GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
	
	if (!gst_element_seek_simple (mGstPipeline, GST_FORMAT_TIME, _flags, mPos)) {
			console() << "Pausing seek failed" << std::endl;
	}

	sendToClients( getPauseMsg() );
}

const osc::Message GstVideoServer::getPauseMsg() const
{
    osc::Message m;
    m.setAddress("/pause");
    m.append((int64_t)mPos);
    return m;
}

const osc::Message GstVideoServer::getPlayMsg() const
{
    osc::Message m;
    m.setAddress("/play");
    m.append((int64_t)mGstBaseTime);
    return m;
}

const osc::Message GstVideoServer::getLoopMsg() const
{
    osc::Message m;
    m.setAddress("/loop");
    m.append((int64_t)mGstBaseTime);
    return m;
}

const osc::Message GstVideoServer::getEosMsg() const
{
    osc::Message m;
    m.setAddress("/eos");
    return m;
}

void GstVideoServer::clientLoadedMessage(const osc::Message &message ){

    ///> If the video loading has failed on the master there is little we can do here...
    if(!isLoaded()){
        console() << " ERROR: Client loaded but master NOT !! Playback wont work properly... " << std::endl;
        return;
    }
    console() << "GstVideoServer: New client loaded" << std::endl;

//    osc::Message m;
//    m.setAddress("/init-time");
//    m.append((int64_t)mGstBaseTime);
//    m.append((int64_t)mPos);
//    sendToClient(m, message.getSenderIpAddress() );

    ///> If the master is paused when the client connects pause the client also.
    if( GstPlayer::isPaused() ){
        sendToClient(getPauseMsg(), message.getSenderIpAddress() );
        return;
    } else{
        sendToClient(getPlayMsg(), message.getSenderIpAddress() );
        return;
    };

}

void GstVideoServer::clientExitedMessage(const osc::Message &message ){

    console() <<"GstVideoServer: Disconnecting client" << std::endl;
   // ClientKey _clientExit(_exitingClient, _exitingClientPort);
    //clientsIter it = mConnectedClients.find(_clientExit);

    //if( it != mConnectedClients.end() ){
      //  auto temp = it;
        //console() <<"GstVideoServer: Disconnecting client with IP : " << _exitingClient << " and port : " << _exitingClientPort << std::endl;
       // mConnectedClients.erase(temp);
   // }

}

ci::gl::Texture2dRef GstVideoServer::getTexture()
{
    return GstPlayer::getVideoTexture();
}

//void GstVideoServer::setPixelFormat( const ofPixelFormat & _pixelFormat )
//{
//    m_videoPlayer.setPixelFormat(_pixelFormat);
//}
//
//const Clients& GstVideoServer::getConnectedClients()
//{
//    return mConnectedClients;
//}
