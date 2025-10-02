#include "../include/network_tools.h"
#include <array>
#include <cstdio>
#include <cstdlib>

static bool runCommand(const std::string &cmd)
{
    return std::system(cmd.c_str()) == 0;
}

int maskToPrefixLength(const std::string &mask)
{
    std::array<int, 4> octets{0, 0, 0, 0};
    if (std::sscanf(mask.c_str(), "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]) != 4)
        return -1;
    int bits = 0;
    for (int o : octets)
    {
        if (o < 0 || o > 255) return -1;
        unsigned v = static_cast<unsigned>(o);
        for (int i = 7; i >= 0; --i)
        {
            if (v & (1u << i))
                ++bits;
            else
                break;
        }
    }
    return bits;
}

bool setStaticIP(const std::string &adapterName,
                 const std::string &ip,
                 const std::string &mask,
                 const std::string &gateway)
{
#if defined(_WIN32)
    int prefix = maskToPrefixLength(mask);
    if (prefix <= 0 || prefix > 32) return false;

    std::string ps =
        "\"$alias='" + adapterName + "';"
        "$ip='" + ip + "';"
        "$pref=" + std::to_string(prefix) + ";"
        "$gw='" + gateway + "';"
        "Try {"
            "Get-NetIPAddress -InterfaceAlias $alias -AddressFamily IPv4 -ErrorAction SilentlyContinue | Where-Object { $_.IPAddress -ne $ip } | ForEach-Object { Remove-NetIPAddress -InputObject $_ -Confirm:$false } ;"
            "New-NetIPAddress -InterfaceAlias $alias -IPAddress $ip -PrefixLength $pref" + (gateway.empty() ? std::string(";") : std::string(" -DefaultGateway $gw;")) +
        " Write-Output 'OK' } Catch { Write-Output 'ERR' }\"";

    std::string command =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \"Start-Process powershell -Verb runAs -WindowStyle Hidden -ArgumentList '" + ps + "'\"";
    return runCommand(command);
#else
    (void)adapterName; (void)ip; (void)mask; (void)gateway;
    return false;
#endif
}

bool setDHCP(const std::string &adapterName)
{
#if defined(_WIN32)
    std::string ps =
        "\"$alias='" + adapterName + "';"
        "Try {"
            "Set-NetIPInterface -InterfaceAlias $alias -Dhcp Enabled -ErrorAction Stop;"
            "Set-DnsClientServerAddress -InterfaceAlias $alias -ResetServerAddresses -ErrorAction SilentlyContinue;"
            "ipconfig /renew;"
        " Write-Output 'OK' } Catch { Write-Output 'ERR' }\"";

    std::string command =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \"Start-Process powershell -Verb runAs -WindowStyle Hidden -ArgumentList '" + ps + "'\"";
    return runCommand(command);
#else
    (void)adapterName;
    return false;
#endif
}

bool pingHost(const std::string &ip)
{
#if defined(_WIN32)
    std::string command = "ping -n 1 -w 1000 " + ip + " >NUL";
    return runCommand(command);
#else
    std::string command = "ping -c 1 -W 1 " + ip + " >/dev/null";
    return runCommand(command);
#endif
}

void openHttp(const std::string &ip)
{
#if defined(_WIN32)
    std::string command = "start \"\" http://" + ip + "/";
    std::system(command.c_str());
#else
    (void)ip;
#endif
}

void openSSH(const std::string &ip, const std::string &user)
{
#if defined(_WIN32)
    std::string command = "start cmd /c \"ssh " + (user.empty() ? std::string("") : user + "@") + ip + "\"";
    std::system(command.c_str());
#else
    (void)ip; (void)user;
#endif
}


