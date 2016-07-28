
#include "GstVideoClient.h"

#define TCP_SENDER_PORT 10001
using namespace asio;
using namespace asio::ip;

GstVideoClient::GstVideoClient()
    : mGstClock(NULL)
    , mGstPipeline(NULL)
    , mGstBaseTime(0)
    , mClockIp("127.0.0.1")
    , mClockPort(1234)
    , mServerRcvPort(mClockPort+1)
    , mClientRcvPort(mClockPort+2)
    , mInitialized(false)
    , mUniqueClientId(0)
{
    GstPlayer::initialize();
}

GstVideoClient::~GstVideoClient()
{
	console() << "Shutdown Client!" << std::endl;
	
	if( mInitialized ){
        osc::Message m("/client-exited");
        m.append(mUniqueClientId);
         if( mOscSender ){
             mOscSender->send(m);
         }
         mInitialized = false;
    }

    if(mOscReceiver) {
		mOscReceiver->closeAcceptor();
		mOscReceiver->close();
	}
    if(mOscSender) {
		mOscSender->shutdown();
		mOscSender->close();
	}
}

//void GstVideoClient::exit(ofEventArgs & args)
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

void GstVideoClient::init( const std::string _clockIp, const uint16_t _clockPort, const uint16_t _oscMasterRcvPort,
						   const uint16_t _oscSlaveRcvPort )
{
    if( mInitialized ){
        console() << " Player already initialized as SLAVE with Ip : " << _clockIp << std::endl;
        return;
    }

    mClockIp = _clockIp;
    mClockPort = _clockPort;

    mServerRcvPort = _oscMasterRcvPort;
    mClientRcvPort = _oscSlaveRcvPort;

    mOscSender = shared_ptr<osc::SenderTcp>(new osc::SenderTcp(TCP_SENDER_PORT,mClockIp,mServerRcvPort));
    mOscSender->setSocketTransportErrorFn( std::bind( &GstVideoClient::socketErrorSender, this, std::placeholders::_1, std::placeholders::_2 ));
	
	mOscSender->bind();
    mOscSender->connect();

    mOscReceiver = shared_ptr<osc::ReceiverTcp>(new osc::ReceiverTcp(mClientRcvPort));

    mOscReceiver->setSocketTransportErrorFn(
			std::bind( &GstVideoClient::socketErrorReceiver, this, std::placeholders::_1, std::placeholders::_2 ) );
    mOscReceiver->setListener("/play" , std::bind(&GstVideoClient::playMessage , this, std::placeholders::_1 ));
    mOscReceiver->setListener("/pause" , std::bind(&GstVideoClient::pauseMessage , this, std::placeholders::_1 ));
    mOscReceiver->setListener("/loop" , std::bind(&GstVideoClient::loopMessage , this, std::placeholders::_1 ));
    mOscReceiver->setListener("/eos" , std::bind(&GstVideoClient::eosMessage , this, std::placeholders::_1 ));
	mOscReceiver->setListener("/init-time" , std::bind(&GstVideoClient::initTimeMessage , this, std::placeholders::_1 ));
	mOscReceiver->setListener("/load-file" , std::bind(&GstVideoClient::loadFileMessage , this, std::placeholders::_1 ));

    mOscReceiver->bind();
    mOscReceiver->listen();

}

//void GstVideoClient::loadAsync( std::string _path )
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

void GstVideoClient::load( const fs::path& path )
{
    GstPlayer::load(path.string());

    ///> Now that we have loaded we can grab the pipeline..
    mGstPipeline = GstPlayer::getPipeline();

	osc::Message m;
	 m.setAddress("/client-loaded");
	 m.append(mUniqueClientId);
	if( mOscSender ){
		mOscSender->send(m);
	 }
}

void GstVideoClient::socketErrorReceiver( const asio::error_code &error, uint64_t identifier )
{
    console() << "Receiver Socket Error for ip address. Error:  " << error.message() << std::endl;
    //close recv and sender
    // TODO add solution for reconnecting
    mOscReceiver->close();
    mOscSender->close();

}

void GstVideoClient::socketErrorSender( const asio::error_code &error, const std::string &oscaddress )
{
	console() << "Sender Socket Error: " << error.message() << std::endl;
	if( oscaddress.length() > 0 )
		console() << "Trying sending message: " << oscaddress << std::endl;
	
	//close recv and sender
	// TODO add solution for reconnecting
	mOscReceiver->close();
	mOscSender->close();
}
void GstVideoClient::initClock( GstClockTime _baseTime )
{
    ///> Remove any previously used clock.
    if(mGstClock) g_object_unref((GObject*) mGstClock);
    mGstClock = NULL;

    ///> Create the slave network clock with an initial time.
    mGstClock = gst_net_client_clock_new(NULL, mClockIp.c_str(), mClockPort, _baseTime);
	
}
void GstVideoClient::setClock()
{
	///> Be explicit.
	gst_pipeline_use_clock(GST_PIPELINE(mGstPipeline), mGstClock);
	gst_pipeline_set_clock(GST_PIPELINE(mGstPipeline), mGstClock);
	
	///> Disable internal handling of time and set the base time.
	gst_element_set_start_time(mGstPipeline, GST_CLOCK_TIME_NONE);
	gst_element_set_base_time(mGstPipeline,  mGstBaseTime);
	
}

void GstVideoClient::setBaseTime( GstClockTime _baseTime )
{
    ///> The arrived master base_time.
    mGstBaseTime = _baseTime;
    gst_element_set_start_time(mGstPipeline, GST_CLOCK_TIME_NONE);
    gst_element_set_base_time(mGstPipeline,  mGstBaseTime);

}

//void GstVideoClient::draw( ofPoint _pos, float _width, float _height )
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

//void GstVideoClient::drawSubsection( float _x, float _y, float _w, float _h, float _sx, float _sy )
//{
//    m_videoPlayer.getTexture().drawSubsection(_x, _y, _w, _h, _sx, _sy);
//}

void GstVideoClient::initTimeMessage(const osc::Message &message ){
    console() <<"GstVideoClient:  CLIENT RECEIVED MASTER INIT TIME! " <<std::endl;

    ///> Initial base time for the clients.
    ///> Set the slave network clock that is going to poll the master.
	initClock(( GstClockTime ) message.getArgInt64( 0 ));

}

void GstVideoClient::loadFileMessage(const osc::Message &message ){
	console() << "GstVideoClient: CLIENT ---> LOAD FILE: " << message.getArgString(0) << std::endl;
	
	auto fileName = message.getArgString(0);
	
	fs::path filePath = getAssetPath( fileName );
	
	if( ! filePath.empty() ) {
		load( filePath );
		///> whenever we load a new file, a new pipeline is created and we need
		///> to set the network clock again
		setClock();
	}else{
		console() << "Couldn't find file: " << fileName << std::endl;
	}
}

void GstVideoClient::playMessage(const osc::Message &message ){
    console() << "GstVideoClient: CLIENT ---> PLAY " << std::endl;

    ///> Set the base time of the slave network clock.
	setBaseTime( message.getArgInt64( 0 ));
    GstPlayer::play();

    ///> and go..
    //gst_element_set_state(mGstPipeline, GST_STATE_PLAYING);
    //gst_element_get_state(mGstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    // TODO do a seek here if position is to far away

}
void GstVideoClient::pauseMessage(const osc::Message &message ){
    mPos = message.getArgInt64(0);
    console() << "GstVideoClient: CLIENT ---> PAUSE " << std::endl;

    //gst_element_set_state(mGstPipeline, GST_STATE_PAUSED);
   // gst_element_get_state(mGstPipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    stop();
    ///> This needs more thinking but for now it gives acceptable results.
    ///> When we pause, we seek to the position of the master when paused was called.
    ///> If we dont do this there is a delay before the pipeline starts again i.e when hitting play() again after pause()..
    ///> I m pretty sure this can be done just by adjusting the base_time based on the position but
    ///> havent figured it out exactly yet..
    GstSeekFlags _flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

    if( !gst_element_seek_simple (mGstPipeline, GST_FORMAT_TIME, _flags, mPos )) {
        console () << "Pausing seek failed" << std::endl;
    }

}
void GstVideoClient::loopMessage(const osc::Message &message ){

    console() << "GstVideoClient: CLIENT ---> LOOP " << std::endl;

    ///> Set the slave base time.
   // GstPlayer::stop();
	setBaseTime( message.getArgInt64( 0 ));
    seekToTime(0);
    //GstPlayer::play();

}
void GstVideoClient::eosMessage(const osc::Message &message ){
    console() << "GstVideoClient CLIENT ---> EOS " << std::endl;

    ///> master stream has ended and there is no loop or source change in the near future
    stop();
}
//void GstVideoClient::initMessage(const osc::Message &message ){
//    console() << "GstVideoClient CLIENT ---> INIT with " << message.getArgInt32(0) << std::endl;
//    mUniqueClientId = message.getArgInt32(0);
//    mInitialized = true;
//}
ci::gl::Texture2dRef GstVideoClient::getTexture()
{
    return GstPlayer::getVideoTexture();
}


//void GstVideoClient::setPixelFormat( const ofPixelFormat & _pixelFormat )
//{
//    m_videoPlayer.setPixelFormat(_pixelFormat);
//}
//
//const Clients& GstVideoClient::getConnectedClients()
//{
//    return m_connectedClients;
//}
