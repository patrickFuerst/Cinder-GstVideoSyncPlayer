#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "GstVideoServer.h"
#include <regex>

#define SERVER_CLOCK_SYNC_PORT 12366 // The port that will be used from GStreamer for master-slave clock synchronization.
#define SERVER_OSC_RCV_PORT 12777 //The port that the master listens for incoming osc messages from the clients.
#define CLIENT_OSC_RCV_PORT 12778 //The port that the slave listens for incoming osc messages from the master.

using namespace ci;
using namespace ci::app;

// We'll create a new Cinder Application by deriving from the App class.
class SimpleServerPlayer : public App {
public:

	
	void keyDown( KeyEvent event ) override;
	void setup() override;
	void update() override;
	void draw() override;
	bool listAvailableVideoFiles();
private:
    GstVideoServer player;
    fs::path mCurrentVideoPath;
    std::vector<fs::path> mAvailableFiles;
    int mCurrentVideoIndex; 
 
};

void prepareSettings( SimpleServerPlayer::Settings* settings )
{
//	settings->setFullScreen(true);
	settings->setWindowSize( 1920 , 1080);
//	auto format = settings->getDefaultWindowFormat();
//	format.fullScreen(true);
//	settings->prepareWindow(format);
}

void SimpleServerPlayer::keyDown( KeyEvent event )
{

    if(  event.getChar() == 'p' ){
        player.pause();
    }
    else if(  event.getChar() == 's' ){
        player.play();
    }else if(  event.getChar() == 'l' ){
        mCurrentVideoIndex = mCurrentVideoIndex < mAvailableFiles.size()-1 ? ++mCurrentVideoIndex : 0;
        mCurrentVideoPath = mAvailableFiles.at(mCurrentVideoIndex);
        player.load(mCurrentVideoPath);
    }

}

void SimpleServerPlayer::setup(){
	
	
	
	std::string localIpAddress;
	const auto& args = App::get()->getCommandLineArgs();
	if(args.size() > 1 ){
		localIpAddress = args[1];
	} else{
		CI_LOG_E("Pass local IP address as argument.");
		exit(-1);
	}

	if( ! listAvailableVideoFiles() ){
		CI_LOG_E("No video files found.");
		exit(-1);
	}
	
	mCurrentVideoIndex = 0;
    mCurrentVideoPath = mAvailableFiles.at(mCurrentVideoIndex);
	
	///> Call the appropriate init function depending on if you are on a master or a slave.
    ///> The IP should be the same in both cases and it refers to the IP the master is running.
    player.init(localIpAddress, SERVER_CLOCK_SYNC_PORT, SERVER_OSC_RCV_PORT,CLIENT_OSC_RCV_PORT);
    player.load(mCurrentVideoPath);
    player.setLoop(true);
	player.setVolume(0.0);
    player.play();
}

void SimpleServerPlayer::update(){

}

void SimpleServerPlayer::draw()
{
    gl::clear();
	
	gl::draw( player.getTexture(), getWindowBounds() );

}

bool SimpleServerPlayer::listAvailableVideoFiles()
{
	std::regex txt_regex("^.*\.(mp4|mov)$");
	auto videoFilesFolder =  std::string(getenv("HOME")) + "/videoWallContent/";

	mAvailableFiles.clear();
	for( auto it = fs::directory_iterator(videoFilesFolder); it != fs::directory_iterator(); it++ ){

		std::string fileName = it->path().filename().string();
		if( std::regex_match( fileName, txt_regex) ){
			CI_LOG_I( "Found video file: " << fileName );
			mAvailableFiles.push_back(*it);
		}
	}
	return mAvailableFiles.size() > 0;
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( SimpleServerPlayer, RendererGl, prepareSettings )