#include "imgui.h"
#include "../include/network_tools.h"
#include "../include/profiles.h"
#include "../include/shortcuts.h"
#include <vector>
#include <string>
#include "../include/tray.h"

void renderNetworkTools()
{
    ImGui::Separator();
    ImGui::Text("Network Tools (Windows)");

    static char adapter[128] = "Ethernet";
    static char ip[64] = "192.168.1.10";
    static char mask[64] = "255.255.255.0";
    static char gw[64] = "192.168.1.1";
    static char devIp[64] = "192.168.1.100";
    static char sshUser[64] = "";

    static char statusMsg[256] = "";

    static std::vector<NetworkProfile> profiles;
    static int selectedProfile = -1;
    static bool loaded = false;
    if (!loaded)
    {
        loadProfiles(profiles);
        loaded = true;
    }

    // Profiles section
    ImGui::Text("Profiles");
    if (ImGui::BeginListBox("##profiles", ImVec2(-FLT_MIN, 120)))
    {
        for (int i = 0; i < (int)profiles.size(); ++i)
        {
            const bool isSelected = (selectedProfile == i);
            if (ImGui::Selectable(profiles[i].name.c_str(), isSelected))
                selectedProfile = i;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
    if (ImGui::Button("Load"))
    {
        if (selectedProfile >= 0 && selectedProfile < (int)profiles.size())
        {
            auto &p = profiles[selectedProfile];
            snprintf(adapter, sizeof(adapter), "%s", p.adapter.c_str());
            snprintf(ip, sizeof(ip), "%s", p.ip.c_str());
            snprintf(mask, sizeof(mask), "%s", p.mask.c_str());
            snprintf(gw, sizeof(gw), "%s", p.gateway.c_str());
            snprintf(devIp, sizeof(devIp), "%s", p.deviceIp.c_str());
            snprintf(sshUser, sizeof(sshUser), "%s", p.sshUser.c_str());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As New"))
    {
        NetworkProfile p;
        p.name = std::string("Profile ") + std::to_string((int)profiles.size() + 1);
        p.adapter = adapter;
        p.ip = ip;
        p.mask = mask;
        p.gateway = gw;
        p.deviceIp = devIp;
        p.sshUser = sshUser;
        profiles.push_back(p);
        saveProfiles(profiles);
    }
    ImGui::SameLine();
    if (ImGui::Button("Overwrite"))
    {
        if (selectedProfile >= 0 && selectedProfile < (int)profiles.size())
        {
            auto &p = profiles[selectedProfile];
            p.adapter = adapter;
            p.ip = ip;
            p.mask = mask;
            p.gateway = gw;
            p.deviceIp = devIp;
            p.sshUser = sshUser;
            saveProfiles(profiles);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
    {
        if (selectedProfile >= 0 && selectedProfile < (int)profiles.size())
        {
            profiles.erase(profiles.begin() + selectedProfile);
            selectedProfile = -1;
            saveProfiles(profiles);
        }
    }

    ImGui::Separator();

    ImGui::InputText("Adapter", adapter, sizeof(adapter));
    ImGui::InputText("Static IP", ip, sizeof(ip));
    ImGui::InputText("Mask", mask, sizeof(mask));
    ImGui::InputText("Gateway", gw, sizeof(gw));

    if (ImGui::Button("Set Static IP"))
    {
        bool ok = setStaticIP(adapter, ip, mask, gw);
        snprintf(statusMsg, sizeof(statusMsg), ok ? "Static IP applied (requires UAC)" : "Failed to apply Static IP");
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert DHCP"))
    {
        bool ok = setDHCP(adapter);
        snprintf(statusMsg, sizeof(statusMsg), ok ? "DHCP enabled (requires UAC)" : "Failed to enable DHCP");
    }

    ImGui::Separator();
    ImGui::InputText("Device IP", devIp, sizeof(devIp));
    ImGui::InputText("SSH User", sshUser, sizeof(sshUser));

    if (ImGui::Button("Ping Device"))
    {
        bool alive = pingHost(devIp);
        snprintf(statusMsg, sizeof(statusMsg), alive ? "Ping OK" : "Ping failed");
    }
    ImGui::SameLine();
    if (ImGui::Button("Open HTTP"))
    {
        openHttp(devIp);
        snprintf(statusMsg, sizeof(statusMsg), "Opening http://%s/", devIp);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open SSH"))
    {
        openSSH(devIp, sshUser);
        snprintf(statusMsg, sizeof(statusMsg), "Opening SSH to %s", devIp);
    }

    if (ImGui::Button("Run All: Set IP → Ping → Open HTTP"))
    {
        bool ok = setStaticIP(adapter, ip, mask, gw);
        if (!ok)
        {
            snprintf(statusMsg, sizeof(statusMsg), "Run All: failed to set IP");
            trayShowNotification("Shortcut", "Failed to set IP");
        }
        else if (!pingHost(devIp))
        {
            snprintf(statusMsg, sizeof(statusMsg), "Run All: ping failed");
            trayShowNotification("Shortcut", "Ping failed");
        }
        else
        {
            openHttp(devIp);
            snprintf(statusMsg, sizeof(statusMsg), "Run All: done (opened http://%s/)", devIp);
            trayShowNotification("Shortcut", "Run All completed");
        }
    }

    // Push Config: save current XML to AppData/export.xml (placeholder for integration)
    if (ImGui::Button("Push Config (save to AppData)"))
    {
        // We'll just create an empty placeholder file for now; integration will serialize real XML later
        std::string exportPath = getAppDataDir();
#if defined(_WIN32)
        exportPath += "\\export.xml";
#else
        exportPath += "/export.xml";
#endif
        FILE *f = fopen(exportPath.c_str(), "wb");
        if (f)
        {
            const char *placeholder = "<!-- TODO: serialize current XML here -->\n";
            fwrite(placeholder, 1, strlen(placeholder), f);
            fclose(f);
            snprintf(statusMsg, sizeof(statusMsg), "Saved: %s", exportPath.c_str());
            trayShowNotification("Push Config", "Config saved to AppData");
        }
        else
        {
            snprintf(statusMsg, sizeof(statusMsg), "Failed to save: %s", exportPath.c_str());
            trayShowNotification("Push Config", "Failed to save config");
        }
    }

    if (statusMsg[0])
    {
        ImGui::Separator();
        ImGui::Text("%s", statusMsg);
    }

    // Shortcuts section
    ImGui::Separator();
    ImGui::Text("Shortcuts");
    static std::vector<ShortcutItem> shortcuts;
    static bool sc_loaded = false;
    if (!sc_loaded) { loadShortcuts(shortcuts); sc_loaded = true; }
    if (ImGui::BeginListBox("##shortcuts", ImVec2(-FLT_MIN, 100)))
    {
        for (int i=0;i<(int)shortcuts.size();++i)
        {
            ImGui::Selectable(shortcuts[i].name.c_str(), false);
        }
        ImGui::EndListBox();
    }
    static char sc_name[128] = "My Shortcut";
    static int sc_action = 0; // 0..3
    static char sc_adapter[128] = "";
    static char sc_ip[64] = "";
    static char sc_mask[64] = "";
    static char sc_gw[64] = "";
    static char sc_devip[64] = "";
    static char sc_sshuser[64] = "";
    ImGui::InputText("Name", sc_name, sizeof(sc_name));
    const char* actions[] = { "RunAll", "OpenHTTP", "OpenSSH", "PushConfig" };
    ImGui::Combo("Action", &sc_action, actions, IM_ARRAYSIZE(actions));
    ImGui::Text("Optional network overrides:");
    ImGui::InputText("Adapter##sc", sc_adapter, sizeof(sc_adapter));
    ImGui::InputText("IP##sc", sc_ip, sizeof(sc_ip));
    ImGui::InputText("Mask##sc", sc_mask, sizeof(sc_mask));
    ImGui::InputText("Gateway##sc", sc_gw, sizeof(sc_gw));
    ImGui::InputText("Device IP##sc", sc_devip, sizeof(sc_devip));
    ImGui::InputText("SSH User##sc", sc_sshuser, sizeof(sc_sshuser));
    if (ImGui::Button("Add Shortcut"))
    {
        ShortcutItem it; it.name = sc_name; it.action = actionFromString(actions[sc_action]);
        it.adapter = sc_adapter; it.ip = sc_ip; it.mask = sc_mask; it.gateway = sc_gw; it.deviceIp = sc_devip; it.sshUser = sc_sshuser;
        shortcuts.push_back(it); saveShortcuts(shortcuts);
    }
}


