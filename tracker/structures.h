#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <set>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <openssl/sha.h>
#include <fcntl.h>
#include <unistd.h> 

using namespace std;



class File {
private:
    string filePath;
    string fileName;
    string sha1Hash;
    set<int> group_ids;
    vector<string> hashValues;
    set<string> seeders;
    map<int, set<string>> chunkAvailability;

public:
    File(const string& filePath, const string& fileName, int group_id, string owner){
        this->filePath= filePath;
        this->fileName= fileName;
        group_ids.insert(group_id);
        calculateHashValues();
        cout<<"hash size: "<<hashValues.size()<<endl;
        for(int i=1;i<hashValues.size();i++){
            chunkAvailability[i]= set<string>();
            chunkAvailability[i].insert(owner);
        }
    }

    string getFilePath(){ 
        return filePath; 
    }

    bool alreadyUploaded(int curGroup){
        return group_ids.count(curGroup)>0;
    }

    string getFileName(){ 
        return fileName;
    }
    
    string getSha1Hash(){ 
        return sha1Hash; 
    }
    
    void displayHashValues(){
        for(string hash : hashValues){
            cout<<hash<<endl;
        }
    }

    void addSeeder(string username){
        seeders.insert(username);
    }

    set<string> getSeeders(){
        return seeders;
    }

    string hashToString(const unsigned char hash[SHA_DIGEST_LENGTH]) {
        char buf[SHA_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
            sprintf(buf + i * 2, "%02x", hash[i]);
        }
        return string(buf);
    }

    void calculateHashValues() {
        int fd = open(filePath.c_str(), O_RDONLY);
        if (fd < 0) {
            cerr << "Wrong file path" << endl;
            return;
        }

        size_t chunkSize = 512 * 1024; 
        vector<unsigned char> chunkBuffer(chunkSize);
        unsigned char hashValue[SHA_DIGEST_LENGTH];
        SHA_CTX shaContext; 

        SHA1_Init(&shaContext);

        ssize_t bytesRead;
        while ((bytesRead = read(fd, chunkBuffer.data(), chunkSize)) > 0) {
            SHA1_Update(&shaContext, chunkBuffer.data(), bytesRead); 
            unsigned char chunkHash[SHA_DIGEST_LENGTH];
            SHA1(chunkBuffer.data(), bytesRead, chunkHash); 
            hashValues.push_back(hashToString(chunkHash)); 
        }

        SHA1_Final(hashValue, &shaContext);
        hashValues.push_back(hashToString(hashValue));

        close(fd);

        //last value is complete file hash
    }

    map<int, set<string>> getChunkDetails(){
        return chunkAvailability;
    }
};


class Group {
private:
    int group_id;
    string owner;
    set<string> members;
    set<string> requests;
    set<string> sharable_files;

public:
    Group() {
        cout << "Wrong number of inputs" << endl;
    }

    Group(int group_id, string owner) {
        this->group_id = group_id;
        this->owner = owner;
    }

    bool isOwner(string enteredOwner) {
        return enteredOwner == this->owner;
    }

    bool isMember(string username){
        return members.count(username)>0;
    }

    void addRequest(string username) {
        if (isOwner(username)) {
            cout << "Client is already the owner of the group" << endl;
            return;
        }
        requests.emplace(username);
    }

    bool hasRequested(string client) {
        return requests.count(client) != 0;
    }

    bool removeUser(string client) {
        if (isOwner(client)) {
            if (members.size() == 0) {
                return false;
            }
            auto newOwner = members.begin();
            owner = *newOwner;
            members.erase(newOwner);
            cout << "New owner: " << owner << endl;
            return true;
        }

        if (members.count(client) > 0) {
            members.erase(client);
            return true;
        }
        return false;
    }

    bool acceptRequest(string client) {
        if (requests.count(client) > 0) {
            requests.erase(client);
            members.emplace(client);
            return true;
        }
        return false;
    }

    set<string> getRequestList() {
        return requests;
    }

    set<string> getSharableFile() {
        return sharable_files;
    }

    void addSharableFile(string fileName) {
        sharable_files.insert(fileName);
    }

    bool stopFileSharing(string fileName){
        if(sharable_files.count(fileName)>0){
            sharable_files.erase(fileName);
            return true;
        }
        return false;
    }

    bool hasFile(string fileName){
        return sharable_files.count(fileName)>0;
    }
};

class Client { 
private:
    string username;
    string password;
    set<string> files_shared;
    string ip;
    int port;
    set<pair<string, string>> downloads;

public:
    Client() {
        cout << "No arguments provided" << endl;
    }

    Client(string username, string password) {
        this->username = username;
        this->password = password;
    }

    bool checkPassword(string enteredPassword) {
        return enteredPassword == this->password;
    }

    string getIp() {
        return ip;
    }

    int getPort() {
        return port;
    }

    void addToDownload(string fileName, string groupId){
        downloads.insert(pair(fileName, groupId));
    }

    set<pair<string, string>> showDownloads(){
        return downloads;
    }
};



extern map<int, Group*> groupInfo;
extern map<string, Client*> clientInfo; 
extern map<string, File*> fileInfo;
extern map<string, string> activeUsers;

#endif