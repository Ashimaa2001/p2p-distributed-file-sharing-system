# p2p-distributed-file-sharing-system

# Compiling the tracker

To compile the tracker, run the following command:
g++ structures.cpp tracker.cpp -o tracker -lssl -lcrypto -Wno-deprecated-declarations

# Executing the tracker

After compilation, execute the tracker with the following command:
./tracker tracker_info.txt tracker_no