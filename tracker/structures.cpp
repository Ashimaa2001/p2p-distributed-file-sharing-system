#include "structures.h"
#include<map>
using namespace std;

map<int, Group*> groupInfo;
map<string, Client*> clientInfo; 
map<string, File*> fileInfo;
map<string, string> activeUsers;