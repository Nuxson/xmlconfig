#include "../include/shortcuts.h"
#include "../include/profiles.h"
#include <fstream>
#include <sstream>

static std::string path()
{
    std::string d = getAppDataDir();
#if defined(_WIN32)
    return d + "\\shortcuts.json";
#else
    return d + "/shortcuts.json";
#endif
}

const char* actionToString(ShortcutAction a)
{
    switch (a)
    {
    case ShortcutAction::RunAll: return "RunAll";
    case ShortcutAction::OpenHTTP: return "OpenHTTP";
    case ShortcutAction::OpenSSH: return "OpenSSH";
    case ShortcutAction::PushConfig: return "PushConfig";
    }
    return "RunAll";
}

ShortcutAction actionFromString(const std::string &s)
{
    if (s == "RunAll") return ShortcutAction::RunAll;
    if (s == "OpenHTTP") return ShortcutAction::OpenHTTP;
    if (s == "OpenSSH") return ShortcutAction::OpenSSH;
    if (s == "PushConfig") return ShortcutAction::PushConfig;
    return ShortcutAction::RunAll;
}

static std::string escapeJson(const std::string &s)
{
    std::string o; o.reserve(s.size()+8);
    for (char c: s) { if (c=='\\' || c=='"') { o.push_back('\\'); o.push_back(c);} else if (c=='\n') { o += "\\n";} else o.push_back(c);} return o;
}

bool saveShortcuts(const std::vector<ShortcutItem> &items)
{
    std::ofstream f(path(), std::ios::binary);
    if (!f) return false;
    f << "[\n";
    for (size_t i=0;i<items.size();++i)
    {
        const auto &it = items[i];
        f << "  {\n";
        f << "    \"name\": \"" << escapeJson(it.name) << "\",\n";
        f << "    \"action\": \"" << actionToString(it.action) << "\",\n";
        f << "    \"adapter\": \"" << escapeJson(it.adapter) << "\",\n";
        f << "    \"ip\": \"" << escapeJson(it.ip) << "\",\n";
        f << "    \"mask\": \"" << escapeJson(it.mask) << "\",\n";
        f << "    \"gateway\": \"" << escapeJson(it.gateway) << "\",\n";
        f << "    \"deviceIp\": \"" << escapeJson(it.deviceIp) << "\",\n";
        f << "    \"sshUser\": \"" << escapeJson(it.sshUser) << "\"\n";
        f << "  }" << (i+1<items.size()?",":"") << "\n";
    }
    f << "]\n";
    return true;
}

static std::string readAll(const std::string &p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

bool loadShortcuts(std::vector<ShortcutItem> &out)
{
    out.clear();
    std::string txt = readAll(path());
    if (txt.empty()) return true;
    size_t pos=0;
    while (true)
    {
        size_t b = txt.find('{', pos); if (b==std::string::npos) break; size_t e = txt.find('}', b); if (e==std::string::npos) break;
        std::string obj = txt.substr(b, e-b+1);
        auto getv=[&](const char* k){ std::string pat = std::string("\"")+k+"\""; size_t p=obj.find(pat); if(p==std::string::npos) return std::string(); p=obj.find('"', obj.find(':',p)); if(p==std::string::npos) return std::string(); size_t q=obj.find('"', p+1); if(q==std::string::npos) return std::string(); return obj.substr(p+1,q-p-1); };
        ShortcutItem it; it.name=getv("name"); it.action=actionFromString(getv("action"));
        it.adapter=getv("adapter"); it.ip=getv("ip"); it.mask=getv("mask"); it.gateway=getv("gateway"); it.deviceIp=getv("deviceIp"); it.sshUser=getv("sshUser");
        if(!it.name.empty()) out.push_back(it); pos=e+1;
    }
    return true;
}


