#pragma once
#include <string>
#include <vector>

enum class ShortcutAction
{
    RunAll,
    OpenHTTP,
    OpenSSH,
    PushConfig
};

struct ShortcutItem
{
    std::string name;
    ShortcutAction action;
    // Optional per-shortcut network overrides
    std::string adapter;
    std::string ip;
    std::string mask;
    std::string gateway;
    std::string deviceIp;
    std::string sshUser;
};

bool loadShortcuts(std::vector<ShortcutItem> &out);
bool saveShortcuts(const std::vector<ShortcutItem> &items);

const char* actionToString(ShortcutAction a);
ShortcutAction actionFromString(const std::string &s);


