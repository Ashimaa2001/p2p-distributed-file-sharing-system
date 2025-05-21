#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <fstream>
#include <algorithm>
#include "structures.h"

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

bool userExists(string username) {
    return clientInfo.count(username) != 0;
}

bool createUser(string username, string password) {
    if (!userExists(username)) {
        clientInfo.emplace(username, new Client(username, password));
        return true;
    }
    return false;
}

bool login(string username, string password, string buffer) {
    if (userExists(username)) {
        if (clientInfo[username]->checkPassword(password)) {
            activeUsers[username]= buffer;
            return true;
        }
    }
    return false;
}

bool groupExists(int group_id) {
    return groupInfo.count(group_id) != 0;
}

bool createGroup(int group_id, string username) {
    if (!groupExists(group_id)) {
        groupInfo.emplace(group_id, new Group(group_id, username));
        return true;
    }
    return false;
}

bool joinGroup(int group_id, string username) {
    if (groupExists(group_id)) {
        groupInfo[group_id]->addRequest(username);
        return true;
    }
    return false;
}

bool leaveGroup(int group_id, string username) {
    if (userExists(username)) {
        return groupInfo[group_id]->removeUser(username);
    }
    return false;
}

set<string> listPendingJoins(int group_id) {
    return groupInfo[group_id]->getRequestList();
}

bool acceptRequest(int group_id, string curClient, string requester) {
    if (groupInfo[group_id]->isOwner(curClient) && userExists(requester)) {
        return groupInfo[group_id]->acceptRequest(requester);
    }
    return false;
}

vector<int> listGroups() {
    vector<int> groupIds;
    for (auto id : groupInfo) {
        groupIds.push_back(id.first);
    }
    return groupIds;
}

string getFileName(string path){
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash == string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

bool fileExists(const string& path) {
    ifstream file(path, ios::binary);
    return file.good();
}


bool uploadFile(string path, int group_id, string username){

    string fileName= getFileName(path);
    if(!fileExists(path) || (!groupInfo[group_id]->isMember(username) && !groupInfo[group_id]->isOwner(username))){
        return false;
    }
    if(fileInfo.count(fileName)>0 && fileInfo[fileName]->alreadyUploaded(group_id)){
        fileInfo[fileName]->addSeeder(username);
        return true;
    }

    File* curFile= new File(path, fileName, group_id, username);
    fileInfo.emplace(fileName, curFile);
    groupInfo[group_id]->addSharableFile(fileName);
    fileInfo[fileName]->addSeeder(username);
    return true;
}

bool removeFileFromGroup(int group_id, string fileName){
    return groupInfo[group_id]->stopFileSharing(fileName);
}

set<string> listGroupFiles(int group_id){
    return groupInfo[group_id]->getSharableFile();
}

void afterDownload(string username, string group_id, string fileName, string destPath){

    clientInfo[username]->addToDownload(fileName, group_id);
    fileInfo[fileName]->addSeeder(username);
    //check if the user is active or not and based on that ask for chunks
}

set<pair<string, string>> getDownloads(string username){ 
    return clientInfo[username]-> showDownloads();
}


void selectRarestPieces(int group_id, const string& fileName) {
    File* file = fileInfo[fileName];
    map<int, set<string>> chunkAvailability = file->getChunkDetails();
    
    vector<pair<int, int>> rarestPieces;

    for (const auto& entry : chunkAvailability) {
        int chunkNumber = entry.first;
        int seederCount = entry.second.size();
        rarestPieces.emplace_back(chunkNumber, seederCount);
    }

    sort(rarestPieces.begin(), rarestPieces.end(), [](const pair<int, int>& a, const pair<int, int>& b) {
        return a.second < b.second;
    });


}


void* handleClient(void* arg) {
    
    int clientSocket = *(int*)arg;
    delete (int*)arg;
    string curUser = "";

    char buffer[1024] = {0};


    while (true) {
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break;
        }

        vector<string> words = split(buffer);
        string trackerLog="";

        if(words[0]=="create_user" && words.size()==3){
                if(createUser(words[1], words[2])){
                    trackerLog= "User created successfully";
                }
                else{
                    trackerLog= "User already exists";
                }
            }

            else if(words[0]=="login" && words.size()==3){

                memset(buffer, 0, sizeof(buffer));
                int bytesRead= read(clientSocket, buffer, sizeof(buffer));

                if(curUser!=""){
                    trackerLog= "Please log out first";
                }
                
                else if(login(words[1], words[2], buffer)){
                    curUser= words[1];
                    trackerLog= "Login successful";
                }
                else{
                    trackerLog= "Username or password incorrect";
                }
            }

            else if(curUser!=""){
            if(words[0]=="create_group" && words.size()==2){
                if(createGroup(stoi(words[1]), curUser)){
                    trackerLog= "Group created with id: "+ words[1];
                }
                else{
                    trackerLog= "Group already exists";
                }
            }

            else if(words[0]=="join_group" && words.size()==2){
                if(joinGroup(stoi(words[1]), curUser)){
                    trackerLog= "User "+ curUser+" requested to join group "+ words[1];
                }
                else{
                    trackerLog= "Group "+ words[1]+" does not exist";
                }
            }

            else if(words[0]=="leave_group" && words.size()==2){
                if(leaveGroup(stoi(words[1]), curUser)){
                    trackerLog= "Left group "+ words[1];
                }
                else{
                    trackerLog= "Username incorrect or not present in group or no other member left";
                }
            }

            else if(words[0]=="list_requests" && words.size()==2){
                if(groupExists(stoi(words[1]))){
                    cout<<"Group requests: "<<endl;
                    for(string str: listPendingJoins(stoi(words[1]))){
                        trackerLog+= str+" ";
                    }
                }
                else{
                    trackerLog= "Group id is incorrect for listing requests";
                }
                trackerLog+=" ";

            }

            else if(words[0]=="accept_request"){
                if(acceptRequest(stoi(words[1]), curUser, words[2])){
                    trackerLog="Request accepted";
                }
                else{
                    trackerLog= "Not the owner/ No request present";
                }

            }

            else if(words[0]=="list_groups" && words.size()==1){
                cout<<"Listing groups: "<<endl;

                for(int id: listGroups()){
                    trackerLog+=to_string(id)+" ";
                }
                trackerLog+=" ";

                cout<<"tracker log: "<<trackerLog<<endl;
                
            }

            else if(words[0]=="upload_file" && words.size()==3){
                if(!groupExists(stoi(words[2]))){
                    trackerLog= "Group does not exist";
                }
                else if(uploadFile("../client/"+words[1], stoi(words[2]), curUser)){
                    trackerLog="File uploaded";
                }
                else{
                    trackerLog= "You are not a member of the group or file does not exist";
                }
            }

            else if(words[0]=="list_files" && words.size()==2){
                for(string file: listGroupFiles(stoi(words[1]))){
                    trackerLog+= file+" ";
                }
                trackerLog+=" ";
            }

            else if(words[0]=="download_file" && words.size()==4){
                if(!groupExists(stoi(words[1])) || !groupInfo[stoi(words[1])]->hasFile(words[2])){
                    trackerLog= "Group does not have the file";
                }

                else if(!groupInfo[stoi(words[1])]->isMember(curUser) && !groupInfo[stoi(words[1])]->isOwner(curUser)){
                    trackerLog= "You are not a member of the group";
                }

                else{
                    set<string> seeders = fileInfo[words[2]]->getSeeders();
                    stringstream seederInfo;

                    for (const auto& seeder : seeders) {
                        if(activeUsers.count(seeder)>0){
                            seederInfo << activeUsers[seeder] << " "; // Also find the chunk number and append here 
                        }
                    } 

                    string response = seederInfo.str();
                    send(clientSocket, response.c_str(), response.size(), 0);

                    afterDownload(curUser, words[1], words[2], words[3]);
                    trackerLog="Downloaded";
                }
            }

            else if(words[0]=="show_downloads" && words.size()==1){
                for(pair<string, string> files: getDownloads(curUser)){
                    trackerLog+= "C "+files.second+" "+ files.first+"   ";
                }
                trackerLog+=" ";
            }

            else if(words[0]=="stop_share" && words.size()==3){
                if(removeFileFromGroup(stoi(words[1]), words[2])){
                    trackerLog=" File sharing stopped successfully";
                }
                else{
                    trackerLog= "File name incorrect or was not being shared";
                }
            }

            else if(words[0]=="logout" && words.size()==1){
                activeUsers.erase(curUser);
                curUser="";
                trackerLog= "Logged out successfully";
            }

            else{
                trackerLog= "Incorrect command";
            }
            }

            else{
                trackerLog= "Please login first";
            }

            cout<<trackerLog<<endl;

        memset(buffer, 0, sizeof(buffer));
        send(clientSocket, trackerLog.c_str(), strlen(trackerLog.c_str()), 0);
    }

    close(clientSocket);
    return nullptr;
}

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    string ip;
    int port;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    ifstream inputFile("trackerInfo.txt");
    if (!inputFile.is_open()) {
        cerr << "Failed to open trackerInfo.txt" << endl;
        return EXIT_FAILURE;
    }

    getline(inputFile, ip);
    inputFile >> port;
    inputFile.close();

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Tracker1 started" << endl;

    while (true) {
        if ((newSocket = accept(serverSocket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            exit(EXIT_FAILURE);
        }

        cout << "Connected new client" << endl;

        pthread_t threadId;
        int* socketPtr = new int(newSocket);
        pthread_create(&threadId, nullptr, handleClient, socketPtr);
        pthread_detach(threadId);
    }

    close(serverSocket);
    return 0;
}
