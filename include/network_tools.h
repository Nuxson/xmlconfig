#pragma once
#include <string>

// Windows network helpers (no-ops on non-Windows)

int maskToPrefixLength(const std::string &mask);

bool setStaticIP(const std::string &adapterName,
                 const std::string &ip,
                 const std::string &mask,
                 const std::string &gateway);

bool setDHCP(const std::string &adapterName);

bool pingHost(const std::string &ip);

void openHttp(const std::string &ip);

void openSSH(const std::string &ip, const std::string &user);


