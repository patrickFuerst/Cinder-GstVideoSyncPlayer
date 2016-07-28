#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "GstVideoClient.h"

#define SERVER_CLOCK_SYNC_PORT 12366 // The port that will be used from GStreamer for master-slave clock synchronization.
#define SERVER_OSC_RCV_PORT 12777 //The port that the master listens for incoming osc messages from the clients.
#define CLIENT_OSC_RCV_PORT 12778 //The port that the slave listens for incoming osc messages from the master.

using namespace ci;
using namespace ci::app;

// We'll create a new Cinder Application by deriving from the App class.
class SimpleClientPlayer : public App {
public:

	void setup() override;
    void update() override;
    void draw() override;

private:
    GstVideoClient player;
};

void prepareSettings( SimpleClientPlayer::Settings* settings )
{
    settings->setMultiTouchEnabled( false );
}

void SimpleClientPlayer::setup(){


    fs::path videoPath = getAssetPath("bbb.mp4");

    ///> Call the appropriate init function depending on if you are on a master or a slave.
    ///> The IP should be the same in both cases and it refers to the IP the master is running.
    player.init("192.168.69.172", SERVER_CLOCK_SYNC_PORT, SERVER_OSC_RCV_PORT,CLIENT_OSC_RCV_PORT);
    player.load(videoPath);
}

void SimpleClientPlayer::update(){

}

void SimpleClientPlayer::draw()
{
    gl::clear();

    gl::draw( player.getTexture() );

}

// This line tells Cinder to actually create and run the application.
CINDER_APP( SimpleClientPlayer, RendererGl, prepareSettings )