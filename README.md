# Compiling the tracker

To compile the tracker, run the following command:
g++ structures.cpp tracker.cpp -o tracker -lssl -lcrypto -Wno-deprecated-declarations

# Executing the tracker

After compilation, execute the tracker with the following command:
./tracker tracker_info.txt tracker_no

# Compiling the client

To compile the client, run the following command:
g++ client.cpp -o client1

# Executing the client

After compilation, run the client:
./client1 <ip:port> trackerInfo.txt

Here,
<ip:port> : The IP address and port of the client
trackerInfo.txt: Path to the tracker IP:Port information file
tracker_no: Specifies the tracker which will be considered the master tracker
