import zmq
import numpy as np

# ZMQ connections, 3 threads
context = zmq.Context(3)


#
bsocket = context.socket(zmq.PUB)
bsocket.bind("ipc:///tmp/ballpos.pipe")

#
fsocket = context.socket(zmq.SUB)
fsocket.connect ("ipc:///tmp/fearures.pipe")
fsocket.setsockopt(zmq.SUBSCRIBE, '' ) # subscribe with no filter, receive all messages

while run:
    try:
            features = fsocket.recv()
            # if received data
            if features != zmq.EAGAIN:
                    # convert string array to numpy array
                    features_vec = np.array([float(x) for x in features.split(',')])
                    print features_vec
            socket.send("1.0");
    except:
            pass

