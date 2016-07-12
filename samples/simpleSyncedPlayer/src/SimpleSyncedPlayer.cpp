#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "GstVideoSyncPlayer.h"

#define MASTER_CLOCK_SYNC_PORT 12366 ///> The port that will be used from GStreamer for master-slave clock synchronization.
#define MASTER_OSC_RCV_PORT 12777 ///> The port that the master listens for incoming osc messages from the clients.
#define SLAVE_OSC_SND_PORT MASTER_OSC_RCV_PORT ///> The osc send port of the slave should be the same as the receive port of the master.
#define SLAVE_OSC_RCV_PORT 12789 ///> You can choose any valid port for the slave to receive. Be carefull to choose a different one if you have multiple clients running on the same machine.

using namespace ci;
using namespace ci::app;

// We'll create a new Cinder Application by deriving from the App class.
class SimpleSyncedPlayer : public App {
public:


    void keyDown( KeyEvent event ) override;
    void setup() override;
    void update() override;
    void draw() override;

private:
    GstVideoSyncPlayer player;
};

void prepareSettings( SimpleSyncedPlayer::Settings* settings )
{
    settings->setMultiTouchEnabled( false );
}



void SimpleSyncedPlayer::keyDown( KeyEvent event )
{

    if(  event.getChar() == 'p' ){
        player.pause();
    }
    else if(  event.getChar() == 's' ){
        player.play();
    }

}

void SimpleSyncedPlayer::setup(){


    fs::path videoPath = getAssetPath("fingers.mov");

    ///> Call the appropriate init function depending on if you are on a master or a slave.
    ///> The IP should be the same in both cases and it refers to the IP the master is running.

    //player.initAsMaster("192.168.69.188", MASTER_CLOCK_SYNC_PORT, MASTER_OSC_RCV_PORT);
    player.initAsSlave("192.168.69.188", MASTER_CLOCK_SYNC_PORT, SLAVE_OSC_RCV_PORT, SLAVE_OSC_SND_PORT);
    player.load(videoPath.string());
    player.loop(true);
    player.play();
}

void SimpleSyncedPlayer::update(){

    player.update();

}

void SimpleSyncedPlayer::draw()
{
    gl::clear();

    gl::draw( player.getTexture() );

}

// This line tells Cinder to actually create and run the application.
CINDER_APP( SimpleSyncedPlayer, RendererGl, prepareSettings )