import argparse
import time
import sys

import fastdds_wrapper

#Argument parsing
#############################################

parser = argparse.ArgumentParser(
    description='Run a simple FastDDS publisher or subscriber.')
parser.add_argument('endpoint', action='store',
                    choices=['publisher', 'subscriber'],
                    help='the kind of endpoint to create')
parser.add_argument('--domain', action='store', default=0, type=int,
                    help='domain identifier')
parser.add_argument('--topic', action='store', default='HelloWorld',
                    help='topic name')

args = parser.parse_args()


# Loop for the publisher
if (args.endpoint == 'publisher'):

    # Create a publisher on the topic
    try:
        pub = fastdds_wrapper.HelloWorldDataWriter(args.domain, args.topic)
    except Exception as e:
        print('Error creating publisher: {error}'.format(
            error=e.__class__))
        sys.exit(-1)

    # Create the message to send
    msg = fastdds_wrapper.HelloWorld()
    msg.message('Hello world')

    try:
        counter = 0
        while True:
            # Set the index in the message
            counter = counter + 1
            msg.index(counter)
            print('Publishing {str} {idx}'.format(
                str=msg.message(), idx=msg.index()))

            # publish
            pub.write_sample(msg)
            time.sleep(1)
    except KeyboardInterrupt:
        print ('Shutdown requested...exiting')

# Loop for the subscriber
else:

    # Create a publisher on the topic
    try:
        sub = fastdds_wrapper.HelloWorldDataReader(args.domain, args.topic)
    except Exception as e:
        print('Error creating subscriber: {error}'.format(
            error=e.__class__))
        sys.exit(-1)

    # Create the message to receive the data
    msg = fastdds_wrapper.HelloWorld()

    try:
        while True:
            # Wait for a message for 2 seconds
            if (sub.wait_for_sample(2)):
                # Read the received message
                if (sub.take_sample(msg)):
                    print('Received {str} {idx}'.format(
                        str=msg.message(), idx=msg.index()))
                else:
                    print('Bad sample')
            else:
                print('No messages received in the last loop')
    except KeyboardInterrupt:
        print ('Shutdown requested...exiting')
