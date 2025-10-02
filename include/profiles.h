#pragma once
#include <string>
#include <vector>

struct NetworkProfile
{
    std::string name;
    std::string adapter;
    std::string ip;
    std::string mask;
    std::string gateway;
    std::string deviceIp;
    std::string sshUser;
};

// Profiles storage API
std::string getAppDataDir();
std::string getProfilesPath();
bool loadProfiles(std::vector<NetworkProfile> &outProfiles);
bool saveProfiles(const std::vector<NetworkProfile> &profiles);


