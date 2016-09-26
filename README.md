# Cinder-GstSyncedPlayer

A cinder block for playing back videos in sync over the network with GStreamer.
Ported from Petros Kataras's ofxGstVideoSyncPlayer https://github.com/PetrosKataras/ofxGstVideoSyncPlayer. 

Currently just working with the custom Gstreamer branch found here: https://github.com/patrickFuerst/Cinder/tree/gstreamer

Though synchronisation and network communication are working it needs more testing. 

As long theres is no offical support of Gstreamer in Cinder, this Repository should be used on your own risk. 



## Future changes: 

Don't split "Server/Client Player", rather create one Clock Provider and one Player, thus keeping it more maintainable. 