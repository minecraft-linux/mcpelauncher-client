#include "imgui_ui.h"
#include "settings.h"

#include <time.h>
#include <game_window_manager.h>
#include <mcpelauncher/path_helper.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <build_info.h>
#include <GLES3/gl3.h>
#if defined(__i386__) || defined(__x86_64__)
#include "cpuid.h"
#endif
#include <string_view>
#include <log.h>
#include <util.h>
#include <chrono>

static double g_Time = 0.0;
static bool allowGPU = true;

static std::vector<long> lmb;
static std::vector<long> rmb;
static bool lmbLast = false;
static bool rmbLast = false;

static ImFont* fontDefaultSize;
static ImFont* fontMediumSize;
static ImFont* fontLargeSize;
static ImFont* fontVeryLargeSize;

static std::string_view myGlGetString(GLenum t) {
    auto raw = glGetString(t);
    if(!raw) {
        return {};
    }
    return (const char*)raw;
}

void ImGuiUIInit(GameWindow* window) {
    if(!glGetString) {
        return;
    }
    Log::info("GL", "Vendor: %s\n", glGetString(GL_VENDOR));
    Log::info("GL", "Renderer: %s\n", glGetString(GL_RENDERER));
    Log::info("GL", "Version: %s\n", glGetString(GL_VERSION));
    if(!Settings::enable_imgui.value_or(allowGPU) || ImGui::GetCurrentContext()) {
        return;
    }
    if(!Settings::enable_imgui.has_value() ) {
        allowGPU = GLAD_GL_ES_VERSION_3_0;
        if(!allowGPU) {
            Log::error("ImGuiUIInit", "Disabling ImGui Overlay due to OpenGLES 2");
        }
    }
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplOpenGL3_Init("#version 100");

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.OversampleH = 1; // Disable horizontal oversampling
    fontConfig.OversampleV = 1; // Disable vertical oversampling
    fontConfig.PixelSnapH = true; // Snap to integer pixel positions


    // One of these three fonts is present in all Minecraft versions newer than 1.0 (earliest supproted by launcher)
    std::string path = PathHelper::getGameDir() + "/assets/assets/fonts/Mojangles.ttf";
    if (!PathHelper::fileExists(path)) {
        path = PathHelper::getGameDir() + "/assets/fonts/Mojangles.ttf";
            if (!PathHelper::fileExists(path)) {
                path = PathHelper::getGameDir() + "/assets/fonts/SegoeWP.ttf";
            }
    }

    size_t data_size = 0;
    void* data = ImFileLoadToMemory(path.data(), "rb", &data_size, 0);

    fontDefaultSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, 15, &fontConfig);
    io.FontDefault = fontDefaultSize;

    fontMediumSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, 18, &fontConfig);

    fontLargeSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, 24, &fontConfig);

    fontVeryLargeSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, 36, &fontConfig);

    IM_FREE(data);

    auto modes = window->getFullscreenModes();
    for(auto&& mode : modes) {
        if(Settings::videoMode == mode.description) {
            window->setFullscreenMode(mode);
        }
    }

    // auto && style = ImGui::GetStyle();
    // style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    // style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // style.Colors[ImGuiCol_Button] = ImVec4(0x1e / 255.0, 0x1e / 255.0, 0x1e / 255.0, 0xff);
    // //style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0x1e / 255.0, 0x1e / 255.0, 0x1e / 255.0, 0xff);
    // style.Colors[ImGuiCol_ButtonActive] = ImVec4(0x30 / 255.0, 0x30 / 255.0, 0x30 / 255.0, 0xff);

}

static void CenterText(int x, int yPos, std::string text) {
    ImVec2 sz = ImGui::CalcTextSize(text.c_str());
    ImVec2 start = ImVec2(ImGui::GetWindowPos().x+ImGui::GetStyle().FramePadding.x, ImGui::GetWindowPos().y);
    start.x += ((x-sz.x) / 2);
    start.y += yPos;
    ImGui::RenderTextWrapped(start, text.c_str(), NULL, 999);
}

void ImGuiUIDrawFrame(GameWindow* window) {
    if(!Settings::enable_imgui.value_or(allowGPU) || !glViewport) {
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
    auto now = std::chrono::high_resolution_clock::now();
    static auto mouseOnY0Since = now;
    bool showMenuBarViaMouse = false;
    if(io.MousePos.y) {
        mouseOnY0Since = now;
    } else {
        auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(now - mouseOnY0Since).count();
        showMenuBarViaMouse = secs >= 500;
    }
    auto autoShowMenubar = (!window->getFullscreen() || showMenuBarViaMouse || menuFocused);
    static auto showFilePicker = false;
    static auto show_demo_window = false;
    static auto show_confirm_popup = false;
    static auto show_about = false;
    auto wantfocusnextframe = io.KeyAlt;
    if(wantfocusnextframe) {
        ImGui::SetNextFrameWantCaptureKeyboard(true);
    }
    static bool lastwantfocusnextframe = false;
    if(Settings::enable_menubar && showMenuBar && (autoShowMenubar || wantfocusnextframe) && ImGui::BeginMainMenuBar())

    {
        menuFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        if(wantfocusnextframe) {
            auto w = ImGui::GetCurrentWindow();
            if(!lastwantfocusnextframe) {
                auto id = ImGui::GetID("File");
                ImGui::SetFocusID(id, w);
                ImGuiContext& g = *ImGui::GetCurrentContext();
                g.NavDisableHighlight = false;
            }
            menuFocused = true;
        }
        lastwantfocusnextframe = wantfocusnextframe;
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
            if(ImGui::BeginMenu("Show Keystroke-Mouse-Hud")) {
                if (ImGui::MenuItem("None", nullptr, Settings::keystroke_mouse_hud_location == -1)) {
                    Settings::keystroke_mouse_hud_location = -1;
                    Settings::save();
                }
                if (ImGui::MenuItem("Top Left", nullptr, Settings::keystroke_mouse_hud_location == 0)) {
                    Settings::keystroke_mouse_hud_location = 0;
                    Settings::save();
                }
                if (ImGui::MenuItem("Top Right", nullptr, Settings::keystroke_mouse_hud_location == 1)) {
                    Settings::keystroke_mouse_hud_location = 1;
                    Settings::save();
                }
                if (ImGui::MenuItem("Bottom Left", nullptr, Settings::keystroke_mouse_hud_location == 2)) {
                    Settings::keystroke_mouse_hud_location = 2;
                    Settings::save();
                }
                if (ImGui::MenuItem("Bottom Right", nullptr, Settings::keystroke_mouse_hud_location == 3)) {
                    Settings::keystroke_mouse_hud_location = 3;
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
        lastwantfocusnextframe = false;
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
    if(Settings::keystroke_mouse_hud_location >= 0) {
        const float SMALL_PAD = 5.0f;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (Settings::keystroke_mouse_hud_location & 1) ? (work_pos.x + work_size.x - SMALL_PAD) : (work_pos.x + SMALL_PAD);
        window_pos.y = (Settings::keystroke_mouse_hud_location & 2) ? (work_pos.y + work_size.y - SMALL_PAD) : (work_pos.y + SMALL_PAD);
        window_pos_pivot.x = (Settings::keystroke_mouse_hud_location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (Settings::keystroke_mouse_hud_location & 2) ? 1.0f : 0.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowBgAlpha(0); // Transparent background
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::Begin("hud", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15.0f, 5.0f));

        ImGui::PushFont(fontVeryLargeSize);

        ImVec2 keySizeNoPad = ImGui::CalcTextSize("W");
        ImVec2 keySize = ImVec2(keySizeNoPad.x+ImGui::GetStyle().FramePadding.x*2, keySizeNoPad.y+ImGui::GetStyle().FramePadding.y*2);

        auto adj = [&](ImVec2 b) {
            return b;
        };

        ImGui::SetCursorPos(adj(ImVec2(SMALL_PAD + keySize.x, work_pos.y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(ImGuiKey_W)->Down ? 0.70f : 0.15f); // Transparent background
        ImVec2 size;
        
        if (ImGui::BeginChild("W", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("W");
            size = ImGui::GetWindowSize();
        }
        ImGui::EndChild();
        
        auto x = work_pos.x;
        auto y = work_pos.y + size.y + SMALL_PAD;
        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(ImGuiKey_A)->Down ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("A", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("A");
            auto pos = ImGui::GetWindowPos();
            size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(ImGuiKey_S)->Down ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("S", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("S");
            auto pos = ImGui::GetWindowPos();
            auto size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImVec2 finalSize = ImVec2(x, keySize.y);

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(ImGuiKey_D)->Down ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("D", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("D");
            auto pos = ImGui::GetWindowPos();
            auto size = ImGui::GetWindowSize();
            finalSize = ImVec2(x + size.x - SMALL_PAD, size.y);
            x -= 2*(SMALL_PAD + size.x);
            y += SMALL_PAD + size.y;
        }
        ImGui::EndChild();

        ImGui::PopFont();

        ImVec2 spaceSize = ImVec2(keySize.x*3 + SMALL_PAD*2 - ImGui::GetStyle().FramePadding.x*2, keySize.y/2);

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(ImGuiKey_Space)->Down ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("Space", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Dummy(spaceSize);

            float lineWidth = spaceSize.x/3; // Adjust as needed

            auto size = ImGui::GetWindowSize();

            int padding = spaceSize.x/3;

            // Draw horizontal line
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(ImGui::GetWindowPos().x+padding, ImGui::GetWindowPos().y+(size.y/2)),// EndChild point (x = 10 + lineWidth, y = 50)
                ImVec2(ImGui::GetWindowPos().x+size.x-padding, ImGui::GetWindowPos().y+(size.y/2)),// EndChild point (x = 10 + lineWidth, y = 50)
                ImColor(255, 255, 255),          // Color (white)
                2.0f                             // Thickness
            );

            auto pos = ImGui::GetWindowPos();
            y += SMALL_PAD + size.y;
        }
        ImGui::EndChild();

        ImGui::PushFont(fontLargeSize);

        bool leftClicked = false;
        bool leftDown = ImGui::GetKeyData(ImGuiKey_MouseLeft)->Down;
        bool rightClicked = false;
        bool rightDown = ImGui::GetKeyData(ImGuiKey_MouseRight)->Down;

        leftClicked = leftDown && !lmbLast;
        rightClicked = rightDown && !rmbLast;

        lmbLast = leftDown;
        rmbLast = rightDown;

        auto now = std::chrono::system_clock::now();
        // Get the duration since the epoch
        auto duration = now.time_since_epoch();
        // Convert duration to milliseconds
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        if (leftClicked) {
            lmb.push_back(milliseconds);
        }
        if (rightClicked) {
            rmb.push_back(milliseconds);
        }

        std::vector<long> newLmb;
        std::vector<long> newRmb;
        for(const long& i : lmb) {
            if (milliseconds - i <= 1000) {
                newLmb.push_back(i);
            }
        }
        for(const long& i : rmb) {
            if (milliseconds - i <= 1000) {
                newRmb.push_back(i);
            }
        }

        lmb = newLmb;
        rmb = newRmb;

        ImVec2 cpsSize = ImVec2((spaceSize.x - ImGui::GetStyle().FramePadding.x*2) / 2 - (SMALL_PAD/2), keySize.y - 10);

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(leftDown ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("LMB", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            auto pos = ImGui::GetWindowPos();
            CenterText(cpsSize.x, 5, "LMB");
            ImGui::Dummy(ImVec2(cpsSize.x, ImGui::GetFontSize()));
            ImGui::PushFont(fontMediumSize);
            CenterText(cpsSize.x, (int)ImGui::GetFontSize()+(ImGui::GetFontSize()/2), std::to_string(lmb.size()) + " CPS");
            ImGui::Dummy(ImVec2(cpsSize.x, (ImGui::GetFontSize() / 1.5)));
            ImGui::PopFont();
            auto size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(rightDown ? 0.70f : 0.15f); // Transparent background
        if (ImGui::BeginChild("RMB", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            auto pos = ImGui::GetWindowPos();
            CenterText(cpsSize.x, 5, "RMB");
            ImGui::Dummy(ImVec2(cpsSize.x, ImGui::GetFontSize()));
            ImGui::PushFont(fontMediumSize);
            CenterText(cpsSize.x, (int)ImGui::GetFontSize()+(ImGui::GetFontSize()/2), std::to_string(rmb.size()) + " CPS");
            ImGui::Dummy(ImVec2(cpsSize.x, (ImGui::GetFontSize() / 1.5)));

            ImGui::PopFont();
            auto size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
