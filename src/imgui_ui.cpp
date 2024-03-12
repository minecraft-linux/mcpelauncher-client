#include "imgui_ui.h"
#include "settings.h"

#include <time.h>
#include <game_window_manager.h>
#include <mcpelauncher/path_helper.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <build_info.h>
#include <GLES2/gl2.h>
#if defined(__i386__) || defined(__x86_64__)
#include "cpuid.h"
#endif

static double g_Time = 0.0;

void ImGuiUIInit(GameWindow* window) {
    if(!ImGui::GetCurrentContext() && glViewport) {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        static std::string ininame = PathHelper::getPrimaryDataDirectory() + "imgui.ini";
        io.IniFilename = ininame.data();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        io.BackendPlatformName = "imgui_impl_mcpelauncher";
        io.ClipboardUserData = window;
        io.SetClipboardTextFn = [](void *user_data, const char *text) {
            if(text != nullptr) {
                ((GameWindow *)user_data)->setClipboardText(text);
            }
        };
        io.GetClipboardTextFn = [](void *user_data) -> const char* {
            return Settings::clipboard.data();
        };

        ImGui_ImplOpenGL3_Init("#version 100");

        Settings::load();
        auto modes = window->getFullscreenModes();
        for(auto&& mode : modes) {
            if(Settings::videoMode == mode.description) {
                window->setFullscreenMode(mode);
            }
        }
    }
}

void ImGuiUIDrawFrame(GameWindow* window) {
    if(!glViewport) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();

    // Setup display size (every frame to accommodate for window resizing)
    int32_t window_width;
    int32_t window_height;
    window->getWindowSize(window_width, window_height);
    int display_width = window_width;
    int display_height = window_height;

    io.DisplaySize = ImVec2((float)window_width, (float)window_height);
    if (window_width > 0 && window_height > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_width / window_width, (float)display_height / window_height);

    // Setup time step
    struct timespec current_timespec;
    clock_gettime(CLOCK_MONOTONIC, &current_timespec);
    double current_time = (double)(current_timespec.tv_sec) + (current_timespec.tv_nsec / 1000000000.0);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    ImGui::NewFrame();
    static auto showMenuBar = true;
    static auto menuFocused = false;
    auto autoShowMenubar = (!window->getFullscreen() || io.MousePos.y == 0 && io.MouseDelta.y < -5 || menuFocused) && !window->getCursorDisabled();
    static auto showFilePicker = false;
    static auto show_demo_window = false;
    static auto show_confirm_popup = false;
    static auto show_about = false;
    if(Settings::enable_menubar && showMenuBar && autoShowMenubar && ImGui::BeginMainMenuBar())

    {
        menuFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        if(ImGui::BeginMenu("File")) {
#ifndef NDEBUG
            if(ImGui::MenuItem("Open")) {
                showFilePicker = true;
            }
#endif
            if(ImGui::MenuItem("Hide Menubar")) {
                show_confirm_popup = true;
            }
#ifndef NDEBUG
            if(ImGui::MenuItem("Show Demo")) {
                show_demo_window = true;
            }
#endif
            if(ImGui::MenuItem("Close")) {
                window->close();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Mods")) {
            if(ImGui::MenuItem("Enable Keyboard Tab/Up/Down Patches for 1.20.60+", nullptr, Settings::enable_keyboard_tab_patches_1_20_60)) {
                Settings::enable_keyboard_tab_patches_1_20_60 ^= true;
                Settings::save();
            }
            if(ImGui::MenuItem("Enable Keyboard AutoFocus Patches for 1.20.60+", nullptr, Settings::enable_keyboard_autofocus_patches_1_20_60)) {
                Settings::enable_keyboard_autofocus_patches_1_20_60 ^= true;
                Settings::save();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View")) {
            if(ImGui::BeginMenu("Show FPS-Hud")) {
                if (ImGui::MenuItem("None", nullptr, Settings::fps_hud_location == -1)) {
                    Settings::fps_hud_location = -1;
                    Settings::save();
                }
                if (ImGui::MenuItem("Top Left", nullptr, Settings::fps_hud_location == 0)) {
                    Settings::fps_hud_location = 0;
                    Settings::save();
                }
                if (ImGui::MenuItem("Top Right", nullptr, Settings::fps_hud_location == 1)) {
                    Settings::fps_hud_location = 1;
                    Settings::save();
                }
                if (ImGui::MenuItem("Bottom Left", nullptr, Settings::fps_hud_location == 2)) {
                    Settings::fps_hud_location = 2;
                    Settings::save();
                }
                if (ImGui::MenuItem("Bottom Right", nullptr, Settings::fps_hud_location == 3)) {
                    Settings::fps_hud_location = 3;
                    Settings::save();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Video")) {
            auto modes = window->getFullscreenModes();
            if(ImGui::MenuItem("Toggle Fullscreen", nullptr, window->getFullscreen())) {
                window->setFullscreen(!window->getFullscreen());
            }
            if(!modes.empty()) {
                ImGui::Separator();
            }
            for(auto&& mode : modes) {
                if(ImGui::MenuItem(mode.description.data(), nullptr, mode.id == window->getFullscreenMode().id)) {
                    window->setFullscreenMode(mode);
                    Settings::videoMode = mode.description;
                    Settings::save();
                }
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, &show_about);
            ImGui::EndMenu();
        }
        auto size = ImGui::GetWindowSize();
        Settings::menubarsize = (int)size.y;
        ImGui::EndMainMenuBar();
    } else {
        Settings::menubarsize = 0;
        menuFocused = false;
    }
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    auto name = "Hide Menubar until exit?";
    if(show_confirm_popup) {
        show_confirm_popup = false;
        ImGui::OpenPopup(name);
    }
    if (ImGui::BeginPopupModal(name, NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        static bool rememberMyDecision = false;
        if(rememberMyDecision) {
            ImGui::TextWrapped("After doing this you cannot access the functionality provided by the menubar until you manually change/delete the settings file");
        } else {
            ImGui::TextWrapped("After doing this you cannot access the functionality provided by the menubar until you restart Minecraft");
        }
        ImGui::Separator();
        ImGui::Checkbox("Remember my Decision Forever (a really long time)", &rememberMyDecision);
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            showMenuBar = false;
            if(rememberMyDecision) {
                Settings::enable_menubar = false;
                Settings::save();
            }
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    if(show_about) {
        if(ImGui::Begin("About", &show_about)) {
            ImGui::Text("mcpelauncher-client %s / manifest %s\n", CLIENT_GIT_COMMIT_HASH, MANIFEST_GIT_COMMIT_HASH);
#if defined(__i386__) || defined(__x86_64__)
            CpuId cpuid;
            ImGui::Text("CPU: %s %s\n", cpuid.getManufacturer(), cpuid.getBrandString());
            ImGui::Text("SSSE3 support: %s\n", cpuid.queryFeatureFlag(CpuId::FeatureFlag::SSSE3) ? "YES" : "NO");
#endif
            ImGui::Text("GL Vendor: %s\n", glGetString(0x1F00 /* GL_VENDOR */));
            ImGui::Text("GL Renderer: %s\n", glGetString(0x1F01 /* GL_RENDERER */));
            ImGui::Text("GL Version: %s\n", glGetString(0x1F02 /* GL_VERSION */));
        }
        ImGui::End();
    }
    if(showFilePicker) {
        if(ImGui::Begin("filepicker", &showFilePicker)) {
            static char path[256];
            ImGui::InputText("Path", path, 256);
            if(ImGui::Button("Open")) {
                
            }
        }
        ImGui::End();
    }
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    if (Settings::fps_hud_location >= 0)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (Settings::fps_hud_location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (Settings::fps_hud_location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (Settings::fps_hud_location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (Settings::fps_hud_location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin("fps-hud", nullptr, window_flags))
        {
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        }
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
