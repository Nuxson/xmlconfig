#include "../include/profiles.h"
#include <fstream>
#include <sstream>
#include <filesystem>

std::string getAppDataDir()
{
#if defined(_WIN32)
    const char *appdata = std::getenv("APPDATA");
    if (appdata && *appdata)
        return std::string(appdata) + "\\xmlconfig";
    return std::string(".");
#else
    const char *home = std::getenv("HOME");
    return (home ? std::string(home) : std::string(".")) + "/.xmlconfig";
#endif
}

std::string getProfilesPath()
{
    std::string dir = getAppDataDir();
    std::filesystem::create_directories(dir);
#if defined(_WIN32)
    return dir + "\\profiles.json";
#else
    return dir + "/profiles.json";
#endif
}

// Minimal JSON serialization without external deps
static std::string escapeJson(const std::string &s)
{
    std::string o; o.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': o += "\\\\"; break;
        case '"': o += "\\\""; break;
        case '\n': o += "\\n"; break;
        case '\r': o += "\\r"; break;
        case '\t': o += "\\t"; break;
        default: o += c; break;
        }
    }
    return o;
}

bool saveProfiles(const std::vector<NetworkProfile> &profiles)
{
    std::ofstream f(getProfilesPath(), std::ios::binary);
    if (!f) return false;
    f << "[\n";
    for (size_t i = 0; i < profiles.size(); ++i)
    {
        const auto &p = profiles[i];
        f << "  {\n";
        f << "    \"name\": \"" << escapeJson(p.name) << "\",\n";
        f << "    \"adapter\": \"" << escapeJson(p.adapter) << "\",\n";
        f << "    \"ip\": \"" << escapeJson(p.ip) << "\",\n";
        f << "    \"mask\": \"" << escapeJson(p.mask) << "\",\n";
        f << "    \"gateway\": \"" << escapeJson(p.gateway) << "\",\n";
        f << "    \"deviceIp\": \"" << escapeJson(p.deviceIp) << "\",\n";
        f << "    \"sshUser\": \"" << escapeJson(p.sshUser) << "\"\n";
        f << "  }" << (i + 1 < profiles.size() ? "," : "") << "\n";
    }
    f << "]\n";
    return true;
}

// Extremely simple JSON reader for our fixed shape
static std::string readAll(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static std::string getValue(const std::string &obj, const std::string &key)
{
    // Find "key": "value"
    std::string pattern = "\"" + key + "\"";
    size_t pos = obj.find(pattern);
    if (pos == std::string::npos) return {};
    pos = obj.find('"', obj.find(':', pos));
    if (pos == std::string::npos) return {};
    size_t end = obj.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return obj.substr(pos + 1, end - pos - 1);
}

bool loadProfiles(std::vector<NetworkProfile> &outProfiles)
{
    outProfiles.clear();
    std::string txt = readAll(getProfilesPath());
    if (txt.empty()) return true; // no file yet

    // naive split by '{' '}' blocks
    size_t pos = 0;
    while (true)
    {
        size_t b = txt.find('{', pos);
        if (b == std::string::npos) break;
        size_t e = txt.find('}', b);
        if (e == std::string::npos) break;
        std::string obj = txt.substr(b, e - b + 1);
        NetworkProfile p;
        p.name = getValue(obj, "name");
        p.adapter = getValue(obj, "adapter");
        p.ip = getValue(obj, "ip");
        p.mask = getValue(obj, "mask");
        p.gateway = getValue(obj, "gateway");
        p.deviceIp = getValue(obj, "deviceIp");
        p.sshUser = getValue(obj, "sshUser");
        if (!p.name.empty()) outProfiles.push_back(std::move(p));
        pos = e + 1;
    }
    return true;
}


