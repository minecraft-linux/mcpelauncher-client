#include "imgui_ui.h"
#include "settings.h"

#include <time.h>
#include <game_window_manager.h>
#include <mcpelauncher/path_helper.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLES2/gl2.h>

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
        // ImGui::StyleColorsDark();
        ImGui::StyleColorsLight();
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
    static auto showFilePicker = false;
    static auto show_demo_window = false;
    if(showMenuBar && ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File")) {
#ifndef NDEBUG
            if(ImGui::MenuItem("Open")) {
                showFilePicker = true;
            }
#endif
            if(ImGui::MenuItem("Hide Menubar")) {
                showMenuBar = false;
            }
#ifndef NDEBUG
            if(ImGui::MenuItem("Show Demo")) {
                show_demo_window = true;
            }
#endif
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Mods")) {
            if(ImGui::MenuItem("Enable Keyboard Tab Patches for 1.20.60+", nullptr, Settings::enable_keyboard_tab_patches_1_20_60)) {
                Settings::enable_keyboard_tab_patches_1_20_60 ^= true;
                Settings::save();
            }
            if(ImGui::MenuItem("Enable Keyboard AutoFocus Patches for 1.20.60+", nullptr, Settings::enable_keyboard_autofocus_patches_1_20_60)) {
                Settings::enable_keyboard_autofocus_patches_1_20_60 ^= true;
                Settings::save();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Video")) {
            auto modes = window->getFullscreenModes();
            for(auto&& mode : modes) {
                if(ImGui::MenuItem(mode.description.data())) {
                    window->setFullscreenMode(mode);
                    Settings::videoMode = mode.description;
                    Settings::save();
                }
            }
            ImGui::EndMenu();
        }
        auto size = ImGui::GetWindowSize();
        Settings::menubarsize = (int)size.y;
        ImGui::EndMainMenuBar();
    } else {
        Settings::menubarsize = 0;
    }
    if(showFilePicker) {
        if(ImGui::Begin("filepicker", &showFilePicker)) {
            ImGui::Text("Nothing to see");
            static char path[256];
            ImGui::InputText("Path", path, 256);
            ImGui::Text("other: %s", path);
        }
        ImGui::End();
    }
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
