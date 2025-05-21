#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <algorithm>
#include <fcntl.h>
#include <fstream>

using namespace std;

vector<string> split(const string &input) {
    istringstream stream(input);
    string word;
    vector<string> words;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}


void* handleDownloadConnection(void* arg) {
    int newSocket = *(int*)arg;
    delete (int*)arg;

    char buffer[1024] = {0};
    int bytesRead = recv(newSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
    }

    string request(buffer);
    vector<string> requestParts = split(request);

    string filePath = requestParts[0];
    int chunkNumber = stoi(requestParts[1]);
    
    int fileDescriptor = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor >= 0) {
        const size_t chunkSize = 512 * 1024;
        off_t chunkOffset = chunkNumber * chunkSize; 
        
        lseek(fileDescriptor, chunkOffset, SEEK_SET);

        vector<char> chunkBuffer(chunkSize);
        ssize_t bytesRead = read(fileDescriptor, chunkBuffer.data(), chunkSize);

        if (bytesRead > 0) {
            send(newSocket, chunkBuffer.data(), bytesRead, 0);
        }

        close(fileDescriptor); 
        cout << "Finished sending chunk " << chunkNumber << " of file: " << filePath << endl;
    }

    close(newSocket);
    return nullptr;
}


void* startListener(void* arg) {
    string ip_port = *(string*)arg;
    delete (string*)arg;

    size_t colonPos = ip_port.find(':');
    if (colonPos == string::npos) return nullptr;

    string ip = ip_port.substr(0, colonPos);
    int port = stoi(ip_port.substr(colonPos + 1));

    int listenerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenerSocket < 0) {
        cout << "Not able to connect with peer" << endl;
        return nullptr;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);

    if (bind(listenerSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cout << "Bind failed" << endl;
        close(listenerSocket);
        return nullptr;
    }

    if (listen(listenerSocket, 3) < 0) {
        cout << "Listen failed" << endl;
        close(listenerSocket);
        return nullptr;
    }

    // cout << "Listening on " << ip << ":" << port << endl;

    while (true) {
        int* newSocket = new int(accept(listenerSocket, nullptr, nullptr));
        if (*newSocket < 0) {
            cout << "Accept failed" << endl;
            delete newSocket;
            continue;
        }

        pthread_t threadId;
        pthread_create(&threadId, nullptr, handleDownloadConnection, newSocket);
        pthread_detach(threadId);
    }

    close(listenerSocket);
    return nullptr;
}


void connectToSeeder(const string& seeder, string fileName, string destination) {
    size_t colonPos = seeder.find(':');

    string ip = seeder.substr(0, colonPos);
    int port = stoi(seeder.substr(colonPos + 1));

    int seederSock = socket(AF_INET, SOCK_STREAM, 0);
    if (seederSock < 0) {
        cout << "Socket creation error for seeder: " << seeder << endl;
        return;
    }

    struct sockaddr_in seederAddr;
    seederAddr.sin_family = AF_INET;
    seederAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &seederAddr.sin_addr) <= 0) {
        close(seederSock);
        return;
    }

    if (connect(seederSock, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
        close(seederSock);
        return;
    }

    const char* message = (fileName).c_str();
    send(seederSock, (fileName + " " + to_string(0)).c_str(), (fileName).size() + to_string(0).length() + 1, 0);

    int destinationFile = open(destination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destinationFile < 0) {
        close(seederSock);
        return;
    }

    char responseBuffer[512 * 1024];
    while (true) {
        ssize_t bytesRead = recv(seederSock, responseBuffer, sizeof(responseBuffer), 0);
        if (bytesRead > 0) {
            write(destinationFile, responseBuffer, bytesRead);
        }
        else if (bytesRead == 0) {
            break;
        } else {
            cout << "Error receiving file from seeder." << endl;
            break;
        }
    }
    

    close(destinationFile);
    close(seederSock);
    cout << "Downloaded successfully" << destination << endl;
}




int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if (argc < 3) {
        cout << "Number of arguments incorrect";
        return 0;
    }

    //send this to the tracker and store the values by separating via the delimiter
    //In the Client structure maintain two variables for IP and port
    //keep these values updated for each user if they are active

    string ip_port = argv[1];
    string trackerFile = argv[2];

    pthread_t listenerThread;
    string* ipPortPtr = new string(ip_port); 
    pthread_create(&listenerThread, nullptr, startListener, ipPortPtr);
    pthread_detach(listenerThread);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }
    string trackerIP;
    int trackerPort;


    ifstream inputFile("../tracker/trackerInfo.txt");
    if (!inputFile.is_open()) {
        cerr << "Not able to opentrackerInfo.txt" << endl;
        return -1;
    }

    getline(inputFile, trackerIP);
    inputFile >> trackerPort;
    inputFile.close();

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(trackerPort);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address / Address not supported" << endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }

    string message;
    while (true) {
        cout << "Enter message: ";
        getline(cin, message);
        send(sock, message.c_str(), message.size(), 0);
        if (message == "exit") {
            break; 
        }
        vector<string> words = split(message);

        if(words[0]=="login"){
            send(sock, ip_port.c_str(), ip_port.size(), 0);
        }

        else if(words[0]=="download_file"){
            string fileName= words[2];
            string destination= words[3];
            char buffer[1024] = {0};
            int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cout << buffer << endl; 

                    string seedersList = string(buffer);
                    istringstream ss(seedersList);
                    string seeder;

                    vector<thread> threads;


                while (getline(ss, seeder, ' ')) {
                    threads.emplace_back([=]() { connectToSeeder(seeder, fileName, destination); }); //add chunkNumber here
                    }
                    
                for (auto& t : threads) {
                    t.join();
                }
            }       
            }

            char buffer[1024] = {0};
                
            int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cout << buffer << endl;
            }
    }

    close(sock);
    return 0;
}
