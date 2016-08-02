#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "GstVideoServer.h"

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

private:
    GstVideoServer player;
};

void prepareSettings( SimpleServerPlayer::Settings* settings )
{
	
}

void SimpleServerPlayer::keyDown( KeyEvent event )
{

    if(  event.getChar() == 'P' ){
        player.pause();
    }
    else if(  event.getChar() == 'S' ){
        player.play();
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
	
    fs::path videoPath = getAssetPath("bbb.mp4");

	
	///> Call the appropriate init function depending on if you are on a master or a slave.
    ///> The IP should be the same in both cases and it refers to the IP the master is running.
    player.init(localIpAddress, SERVER_CLOCK_SYNC_PORT, SERVER_OSC_RCV_PORT,CLIENT_OSC_RCV_PORT);
    player.load(videoPath);
    player.setLoop(true);
	player.setVolume(0.0);
    player.play();
}

void SimpleServerPlayer::update(){

}

void SimpleServerPlayer::draw()
{
    gl::clear();

    gl::draw( player.getTexture() );

}

// This line tells Cinder to actually create and run the application.
CINDER_APP( SimpleServerPlayer, RendererGl, prepareSettings )