#include "eeg_receiver.h"
#include <strstream>

EegReceiver::EegReceiver():
    context(1), eeg_subscriber(context, ZMQ_SUB)
{
    eeg_subscriber.connect("tcp://192.168.56.101:5556");
    eeg_subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

}

EegReceiver::receive(float* channels) {
    zmq::message_t update;
    eeg_subscriber.recv(&update);
    std::istringstream iss(static_cast<char*>(update.data()));

    for (int i=0; i<65; i++) {
        iss >> str;
        channels[i] = atof(str.c_str());
    }

}
