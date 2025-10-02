#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdint>
#include "network_ui.h"
#include "xml_parser.h"
#include "tray.h"

// Глобальные переменные
XMLParser g_parser;
MyXMLNode *g_selectedNode = nullptr;
char g_newNodeName[128] = "new_node";
char g_newAttributeName[128] = "attribute";
char g_newAttributeValue[128] = "value";
char g_nodeValue[1024] = "";
char g_currentFile[256] = "untitled.xml";
bool g_unsavedChanges = false;
static GLFWwindow *g_mainWindow = nullptr;
static bool g_minimizeToTrayOption = true;
static bool g_startMinimizedOption = false;

static void onTrayActivate()
{
    if (g_mainWindow)
    {
        glfwShowWindow(g_mainWindow);
        glfwRestoreWindow(g_mainWindow);
        glfwFocusWindow(g_mainWindow);
    }
}

static void onWindowCloseCallback(GLFWwindow *win)
{
    if (g_minimizeToTrayOption)
    {
        glfwHideWindow(win);
        glfwSetWindowShouldClose(win, GLFW_FALSE);
    }
}

// Globals for file dialogs
static bool g_showLoadDialog = false;
static bool g_showSaveAsDialog = false;
static char g_loadFilename[256] = "config.xml";
static char g_saveAsFilename[256] = "new_config.xml";

// moved to network_ui.h/.cpp

void markUnsavedChanges()
{
    g_unsavedChanges = true;
}

void loadFile(const std::string &filename)
{
    if (g_parser.loadFromFile(filename))
    {
        g_selectedNode = g_parser.getRoot();
        strcpy(g_currentFile, filename.c_str());
        g_unsavedChanges = false;
        std::cout << "Loaded: " << filename << std::endl;
    }
    else
    {
        std::cout << "Failed to load: " << filename << std::endl;
    }
}

void saveFile(const std::string &filename)
{
    if (g_parser.saveToFile(filename))
    {
        strcpy(g_currentFile, filename.c_str());
        g_unsavedChanges = false;
        std::cout << "Saved: " << filename << std::endl;
    }
    else
    {
        std::cout << "Failed to save: " << filename << std::endl;
    }
}

void renderXmlTree(MyXMLNode *node, int depth = 0)
{
    if (!node)
        return;

    std::string indent(depth * 2, ' ');
    std::string label = indent + node->name;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (g_selectedNode == node)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (node->children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool isOpen = ImGui::TreeNodeEx(node, flags, "%s", label.c_str());

    if (ImGui::IsItemClicked())
    {
        g_selectedNode = node;
        strcpy(g_nodeValue, node->value.c_str());
    }

    // Контекстное меню для узла
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Add Child"))
        {
            auto newNode = node->addChild("new_node");
            markUnsavedChanges();
        }
        if (ImGui::MenuItem("Delete Node") && node->parent)
        {
            // Найдем и удалим узел из родителя
            auto &children = node->parent->children;
            children.erase(
                std::remove_if(children.begin(), children.end(),
                               [node](const std::unique_ptr<MyXMLNode> &n)
                               {
                                   return n.get() == node;
                               }),
                children.end());
            g_selectedNode = node->parent;
            markUnsavedChanges();
        }
        ImGui::EndPopup();
    }

    if (isOpen)
    {
        for (auto &child : node->children)
        {
            renderXmlTree(child.get(), depth + 1);
        }
        ImGui::TreePop();
    }
}

void renderPropertyEditor()
{
    if (!g_selectedNode)
    {
        ImGui::Text("Select a node to edit");
        return;
    }

    ImGui::Text("Editing: %s", g_selectedNode->name.c_str());
    ImGui::Separator();

    // Редактор имени узла
    static char nodeName[256];
    strcpy(nodeName, g_selectedNode->name.c_str());
    if (ImGui::InputText("Node Name", nodeName, sizeof(nodeName)))
    {
        g_selectedNode->name = nodeName;
        markUnsavedChanges();
    }

    // Редактор значения узла
    if (ImGui::InputTextMultiline("Node Value", g_nodeValue, sizeof(g_nodeValue)))
    {
        g_selectedNode->value = g_nodeValue;
        markUnsavedChanges();
    }

    // Редактор атрибутов
    ImGui::Separator();
    ImGui::Text("Attributes:");

    std::vector<std::string> attributesToRemove;

    for (auto &[key, value] : g_selectedNode->attributes)
    {
        ImGui::PushID(key.c_str());

        ImGui::Columns(2);
        ImGui::Text("%s", key.c_str());
        ImGui::NextColumn();

        static char attrValue[256];
        strcpy(attrValue, value.c_str());
        if (ImGui::InputText("##value", attrValue, sizeof(attrValue)))
        {
            g_selectedNode->attributes[key] = attrValue;
            markUnsavedChanges();
        }

        ImGui::SameLine();
        if (ImGui::Button("X"))
        {
            attributesToRemove.push_back(key);
        }

        ImGui::Columns(1);
        ImGui::PopID();
    }

    // Удаляем отмеченные атрибуты
    for (const auto &attr : attributesToRemove)
    {
        g_selectedNode->attributes.erase(attr);
        markUnsavedChanges();
    }

    // Добавление нового атрибута
    ImGui::Separator();
    ImGui::Text("Add Attribute:");
    ImGui::InputText("Name", g_newAttributeName, sizeof(g_newAttributeName));
    ImGui::InputText("Value", g_newAttributeValue, sizeof(g_newAttributeValue));
    if (ImGui::Button("Add Attribute") && strlen(g_newAttributeName) > 0)
    {
        g_selectedNode->attributes[g_newAttributeName] = g_newAttributeValue;
        g_newAttributeName[0] = '\0';
        g_newAttributeValue[0] = '\0';
        markUnsavedChanges();
    }

    // Добавление дочернего узла
    ImGui::Separator();
    ImGui::Text("Add Child Node:");
    ImGui::InputText("Child Name", g_newNodeName, sizeof(g_newNodeName));
    if (ImGui::Button("Add Child") && strlen(g_newNodeName) > 0)
    {
        g_selectedNode->addChild(g_newNodeName);
        g_newNodeName[0] = '\0';
        markUnsavedChanges();
    }
}

void renderFileDialogs()
{
    // Note: Menю обрабатывается в верхней панели, здесь только сами попапы

    // Диалог загрузки
    if (g_showLoadDialog)
    {
        ImGui::OpenPopup("Load File");
        g_showLoadDialog = false;
    }

    if (ImGui::BeginPopupModal("Load File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Load XML File:");
        ImGui::InputText("Filename", g_loadFilename, sizeof(g_loadFilename));

        ImGui::Text("Examples: config.xml, settings.xml, data.xml");

        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            loadFile(g_loadFilename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Диалог сохранения как
    if (g_showSaveAsDialog)
    {
        ImGui::OpenPopup("Save As");
        g_showSaveAsDialog = false;
    }

    if (ImGui::BeginPopupModal("Save As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save XML File As:");
        ImGui::InputText("Filename", g_saveAsFilename, sizeof(g_saveAsFilename));

        ImGui::Text("Examples: my_config.xml, backup.xml, settings_new.xml");

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            saveFile(g_saveAsFilename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

int main()
{
    // Инициализация GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Создание окна
    GLFWwindow *window = glfwCreateWindow(1400, 800, "XML Config Editor", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    g_mainWindow = window;
    glfwSetWindowCloseCallback(window, onWindowCloseCallback);

    // Настройка ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
#ifdef XMLCONFIG_ENABLE_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // enable docking (requires docking-enabled ImGui)
#endif
    ImGui::StyleColorsDark();

    // Инициализация рендереров
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Трей
    trayInitialize();
    traySetOnActivate(onTrayActivate);

    // Инициализация XML парсера с примером
    g_parser.createNewDocument("config");
    auto root = g_parser.getRoot();

    // Создаем пример структуры
    auto settings = root->addChild("settings");
    settings->addChild("window")->attributes = {{"width", "1280"}, {"height", "720"}};
    settings->addChild("audio")->attributes = {{"volume", "80"}, {"muted", "false"}};

    g_selectedNode = root;

    // Если выбрано стартовать свёрнутым — спрячем окно
    if (g_startMinimizedOption)
    {
        glfwHideWindow(window);
    }

    // Главный цикл
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Auto-hide taskbar icon when minimized (Windows-specific behavior via GLFW hints is limited)
        if (g_minimizeToTrayOption)
        {
            int iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
            if (iconified)
            {
                // Hide window until restored via tray callback
                glfwHideWindow(window);
            }
        }

        // Начало кадра ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#ifdef XMLCONFIG_ENABLE_DOCKING
        // DockSpace host (fullscreen)
        ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("DockSpace", nullptr, dockspace_flags);
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);

        // persistent view toggles
        static bool showXmlTreeWin = true;
        static bool showPropertiesWin = true;
        static bool showNetworkWin = true;
        static bool showStatusBarWin = true;

        // Menu bar (File/View) — без Debug
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New"))
                {
                    g_parser.createNewDocument("config");
                    g_selectedNode = g_parser.getRoot();
                    strcpy(g_currentFile, "untitled.xml");
                    g_unsavedChanges = false;
                }
                if (ImGui::MenuItem("Open..."))
                {
                    g_showLoadDialog = true; strcpy(g_loadFilename, "config.xml");
                }
                bool canSave = strcmp(g_currentFile, "untitled.xml") != 0;
                if (ImGui::MenuItem("Save", nullptr, false, canSave))
                {
                    saveFile(g_currentFile);
                }
                if (ImGui::MenuItem("Save As..."))
                {
                    g_showSaveAsDialog = true; strcpy(g_saveAsFilename, g_currentFile);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("XML Tree", nullptr, &showXmlTreeWin);
                ImGui::MenuItem("Properties", nullptr, &showPropertiesWin);
                ImGui::MenuItem("Network Tools", nullptr, &showNetworkWin);
                ImGui::MenuItem("Status Bar", nullptr, &showStatusBarWin);
            if (ImGui::BeginMenu("Settings"))
                {
                    if (ImGui::MenuItem("Shortcuts"))
                    {
                        // Open Network Tools where shortcuts live for now
                        showNetworkWin = true;
                    }
                if (ImGui::MenuItem("Tray Options")) {}
                ImGui::Checkbox("Start minimized to tray", &g_startMinimizedOption);
                ImGui::Checkbox("Minimize to tray on close/minimize", &g_minimizeToTrayOption);
                    {
                        // Persist options later if needed
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // File dialogs popups
        renderFileDialogs();

        // Separate windows
        if (showXmlTreeWin)
        {
            ImGui::Begin("XML Tree");
            renderXmlTree(g_parser.getRoot());
            ImGui::End();
        }

        if (showPropertiesWin)
        {
            ImGui::Begin("Properties");
            renderPropertyEditor();
            ImGui::End();
        }

        if (showNetworkWin)
        {
            ImGui::Begin("Network Tools");
            renderNetworkTools();
            ImGui::End();
        }

        // Status Bar window
        if (showStatusBarWin)
        {
            ImGuiWindowFlags sb_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
            ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 22.0f));
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 22.0f));
            ImGui::Begin("Status Bar", nullptr, sb_flags);
            ImGui::Text("File: %s %s", g_currentFile, g_unsavedChanges ? "*" : "");
            ImGui::End();
        }

        ImGui::End();
#else
        // Fallback layout without docking: independent top-level windows + global menu bar
        static bool showXmlTreeWin = true;
        static bool showPropertiesWin = true;
        static bool showNetworkWin = true;
        static bool showStatusBarWin = true;

        // Global Main Menu Bar — без Debug
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New"))
                {
                    g_parser.createNewDocument("config");
                    g_selectedNode = g_parser.getRoot();
                    strcpy(g_currentFile, "untitled.xml");
                    g_unsavedChanges = false;
                }
                if (ImGui::MenuItem("Open..."))
                {
                    g_showLoadDialog = true; strcpy(g_loadFilename, "config.xml");
                }
                bool canSave = strcmp(g_currentFile, "untitled.xml") != 0;
                if (ImGui::MenuItem("Save", nullptr, false, canSave))
                {
                    saveFile(g_currentFile);
                }
                if (ImGui::MenuItem("Save As..."))
                {
                    g_showSaveAsDialog = true; strcpy(g_saveAsFilename, g_currentFile);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("XML Tree", nullptr, &showXmlTreeWin);
                ImGui::MenuItem("Properties", nullptr, &showPropertiesWin);
                ImGui::MenuItem("Network Tools", nullptr, &showNetworkWin);
                ImGui::MenuItem("Status Bar", nullptr, &showStatusBarWin);
            if (ImGui::BeginMenu("Settings"))
                {
                    if (ImGui::MenuItem("Shortcuts"))
                    {
                        showNetworkWin = true;
                    }
                if (ImGui::MenuItem("Tray Options")) {}
                ImGui::Checkbox("Start minimized to tray", &g_startMinimizedOption);
                ImGui::Checkbox("Minimize to tray on close/minimize", &g_minimizeToTrayOption);
                    {
                        // Persist options later if needed
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // File dialogs popups
        renderFileDialogs();

        // Independent windows
        if (showXmlTreeWin)
        {
            ImGui::Begin("XML Tree");
            renderXmlTree(g_parser.getRoot());
            ImGui::End();
        }

        if (showPropertiesWin)
        {
            ImGui::Begin("Properties");
            renderPropertyEditor();
            ImGui::End();
        }

        if (showNetworkWin)
        {
            ImGui::Begin("Network Tools");
            renderNetworkTools();
            ImGui::End();
        }

        // Status Bar window at bottom
        if (showStatusBarWin)
        {
            ImGuiIO &fio = ImGui::GetIO();
            ImGuiWindowFlags sb_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoInputs;
            ImGui::SetNextWindowPos(ImVec2(0.0f, fio.DisplaySize.y - 22.0f));
            ImGui::SetNextWindowSize(ImVec2(fio.DisplaySize.x, 22.0f));
            ImGui::Begin("Status Bar", nullptr, sb_flags);
            ImGui::Text("File: %s %s", g_currentFile, g_unsavedChanges ? "*" : "");
            ImGui::End();
        }
#endif

        // Рендеринг
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Очистка
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    trayShutdown();

    return 0;
}