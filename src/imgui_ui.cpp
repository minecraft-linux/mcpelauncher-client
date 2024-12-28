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
#include <sstream>
#include "window_callbacks.h"
#include <mutex>
#include <mcpelauncher/linker.h>

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

static void ReloadFont() {
    ImGuiIO& io = ImGui::GetIO();
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

    io.Fonts->Clear();
    fontDefaultSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, (int)ceil(15 * Settings::scale), &fontConfig);
    io.FontDefault = fontDefaultSize;

    fontMediumSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, (int)ceil(18 * Settings::scale), &fontConfig);

    fontLargeSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, (int)ceil(24 * Settings::scale), &fontConfig);

    fontVeryLargeSize = io.Fonts->AddFontFromMemoryTTF(data, data_size, (int)ceil(36 * Settings::scale), &fontConfig);

    IM_FREE(data);

    ImGui_ImplOpenGL3_CreateFontsTexture();
}


struct MenuEntry {
    std::string name;
    std::function<bool()> selected;
    std::function<void()> click;
    std::vector<MenuEntry> subentries;
};

struct WindowControl
{
   WindowControl() {
    
   }
   WindowControl(WindowControl&& control) {
    type = control.type;
    memcpy(&data, &control.data, sizeof(control.data));
    control.type = -1;
   }
   int type = -1;
   union Data {
    Data() {

    }
    struct {
      std::string label;
      void* user;
      void(*onClick)(void* user);
    } button;
    struct {
      std::string label;
      int min;
      int def;
      int max;
      void* user;
      void(*onChange)(void* user, int value);
    } sliderint;
    struct {
      std::string label;
      float min;
      float def;
      float max;
      void* user;
      void(*onChange)(void* user, float value);
    } sliderfloat;
    struct {
      std::string label;
      int size; // 0 normal/ 1 small titel...
    } text;
    struct {
      std::string label;
      std::string def;
      std::string placeholder;
      void* user;
      void(*onChange)(void* user, const char* value);
    } textinput;
    ~Data() {}
  } data;

  ~WindowControl() {
    switch (type)
    {
    case 0:
        data.button.label.~basic_string();
        break;
    case 1:
        data.sliderint.label.~basic_string();
        break;
    case 2:
        data.sliderfloat.label.~basic_string();
        break;
    case 3:
        data.text.label.~basic_string();
        break;
    case 4:
        data.textinput.label.~basic_string();
        data.textinput.def.~basic_string();
        data.textinput.placeholder.~basic_string();
        break;
    
    default:
        break;
    }
  }
};

struct ActiveWindow {
    std::string title;
    bool isModal;
    bool open = true;
    bool modalOpened = false;
    void* user;
    void (*onClose)(void *user);
    std::vector<WindowControl> controls;
};

static std::vector<MenuEntry> menuentries;
static std::mutex menuentrieslock;
static std::vector<std::shared_ptr<ActiveWindow>> activeWindows;
static std::mutex activeWindowsLock;
static std::vector<std::function<void()>> onDrawCallbacks;
static std::mutex onDrawCallbacksLock;

static void convertEntries(std::vector<MenuEntry>& menuentries, size_t length, MenuEntryABI* entries) {
    for(size_t i = 0; i < length; i++) {
        std::vector<MenuEntry> subentries;
        convertEntries(subentries, entries[i].length, entries[i].subentries);
        menuentries.emplace_back(MenuEntry{ .name = entries[i].name, .selected = std::bind(entries[i].selected, entries[i].user), .click = std::bind(entries[i].click, entries[i].user), .subentries = subentries });
    }
}

static void appendMenu(std::vector<MenuEntry>& menuentries) {
    for(size_t i = 0; i < menuentries.size(); i++) {
        if(menuentries[i].subentries.size()) {
            if(ImGui::BeginMenu(menuentries[i].name.data())) {
                appendMenu(menuentries[i].subentries);
                ImGui::EndMenu();
            }
        } else if(ImGui::MenuItem(menuentries[i].name.data(), nullptr, menuentries[i].selected(), true)) {
            menuentries[i].click();
        }
    }
}


void mcpelauncher_addmenu(size_t length, MenuEntryABI* entries) {
    menuentrieslock.lock();
    convertEntries(menuentries, length, entries);
    menuentrieslock.unlock();
}

void mcpelauncher_show_window(const char *title, int isModal, void *user, void (*onClose)(void *user), int count, control *controls) {
    activeWindowsLock.lock();
    std::vector<WindowControl> subentries(count);
    for(int i = 0; i < count; i++) {
        subentries[i].type = controls[i].type;
        switch (controls[i].type)
        {
        case 0:
            new(&subentries[i].data.button.label) std::string(controls[i].data.button.label ? controls[i].data.button.label : "");
            subentries[i].data.button.user = controls[i].data.button.user;
            subentries[i].data.button.onClick = controls[i].data.button.onClick;
            break;
        case 1:
            new(&subentries[i].data.sliderint.label) std::string(controls[i].data.sliderint.label ? controls[i].data.sliderint.label : "");
            subentries[i].data.sliderint.def = controls[i].data.sliderint.def;
            subentries[i].data.sliderint.max = controls[i].data.sliderint.max;
            subentries[i].data.sliderint.min = controls[i].data.sliderint.min;
            subentries[i].data.sliderint.user = controls[i].data.sliderint.user;
            subentries[i].data.sliderint.onChange = controls[i].data.sliderint.onChange;
            break;
        case 2:
            new(&subentries[i].data.sliderfloat.label) std::string(controls[i].data.sliderfloat.label ? controls[i].data.sliderfloat.label : "");
            subentries[i].data.sliderfloat.def = controls[i].data.sliderfloat.def;
            subentries[i].data.sliderfloat.max = controls[i].data.sliderfloat.max;
            subentries[i].data.sliderfloat.min = controls[i].data.sliderfloat.min;
            subentries[i].data.sliderfloat.user = controls[i].data.sliderfloat.user;
            subentries[i].data.sliderfloat.onChange = controls[i].data.sliderfloat.onChange;
            break;
        case 3:
            new(&subentries[i].data.text.label) std::string(controls[i].data.text.label ? controls[i].data.text.label : "");
            subentries[i].data.text.size = controls[i].data.text.size;
            break;
        case 4:
            new(&subentries[i].data.textinput.label) std::string(controls[i].data.textinput.label ? controls[i].data.textinput.label : "");
            new(&subentries[i].data.textinput.def) std::string(controls[i].data.textinput.def ? controls[i].data.textinput.def : "");
            new(&subentries[i].data.textinput.placeholder) std::string(controls[i].data.textinput.placeholder ? controls[i].data.textinput.placeholder : "");
            subentries[i].data.textinput.user = controls[i].data.textinput.user;
            subentries[i].data.textinput.onChange = controls[i].data.textinput.onChange;
            break;
        
        default:
            break;
        }
    }

    if(auto activeWindow = std::find_if(activeWindows.begin(), activeWindows.end(), [title](auto&& wnd) {
            return wnd->title == (title ? title : "Untitled");
        }); activeWindow != activeWindows.end()) {
        (*activeWindow)->isModal = !!isModal;
        (*activeWindow)->user = user;
        (*activeWindow)->onClose = onClose;
        (*activeWindow)->controls = std::move(subentries);
    } else {
        activeWindows.push_back(std::make_shared<ActiveWindow>(ActiveWindow{
            .title = title ? title : "Untitled",
            .isModal = !!isModal,
            .user = user,
            .onClose = onClose,
            .controls = std::move(subentries),
        }));
    }
    activeWindowsLock.unlock();
}

void mcpelauncher_close_window(const char *title) {
    activeWindowsLock.lock();
    if(auto activeWindow = std::find_if(activeWindows.begin(), activeWindows.end(), [title](auto&& wnd) {
            return wnd->title == (title ? title : "Untitled");
        }); activeWindow != activeWindows.end()) {
        activeWindows.erase(activeWindow);
    }
    activeWindowsLock.unlock();
}

void mcpelauncher_add_draw_callback(void* user, void(*onDraw)(void* user)) {
    onDrawCallbacksLock.lock();
    onDrawCallbacks.push_back(std::bind(onDraw, user));
    onDrawCallbacksLock.unlock();
}

struct ImGuiContext* mcpelauncher_get_imgui_context() {
    return ImGui::GetCurrentContext();
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

    ReloadFont();

    auto modes = window->getFullscreenModes();
    for(auto&& mode : modes) {
        if(Settings::videoMode == mode.description) {
            window->setFullscreenMode(mode);
        }
    }
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
    bool reloadFont = false;
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
    auto wantfocusnextframe = Settings::menubarFocusKey == "alt" && ImGui::IsKeyPressed(ImGuiKey_ModAlt) || Settings::menubarFocusKey == "shift+m+p" && ImGui::IsKeyPressed(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_M) && ImGui::IsKeyPressed(ImGuiKey_P);
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
                g.NavCursorVisible = true;
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
            if(ImGui::MenuItem("Use Alt to Focus Menubar", nullptr, Settings::menubarFocusKey == "alt")) {
                Settings::menubarFocusKey = Settings::menubarFocusKey == "alt" ? "" : "alt";
                Settings::save();
            }

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
            if(menuentrieslock.try_lock()) {
                appendMenu(menuentries);
                menuentrieslock.unlock();
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
            if(ImGui::BeginMenu("UI Scale")) {
                for(int i = 4; i <= 20; i++) {
                    std::stringstream ss;
                    ss << 25*i;
                    if (ImGui::MenuItem((ss.str() + "%").data(), nullptr, Settings::scale == i / 4.)) {
                        Settings::scale = i / 4.;
                        Settings::save();
                        reloadFont = true;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Video")) {
            auto modes = window->getFullscreenModes();
            if(ImGui::MenuItem("Toggle Fullscreen", nullptr, window->getFullscreen())) {
                window->setFullscreen(!Settings::fullscreen);
                Settings::fullscreen = !Settings::fullscreen;
                Settings::save();
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
#if defined(__linux__)
#define TARGET "Linux"
#elif defined(__APPLE__)
#define TARGET "macOS"
#elif defined(__FreeBSD__)
#define TARGET "FreeBSD"
#else
#define TARGET "Unknown"
#endif
#if defined(__x86_64__)
#define ARCH "x86_64"
#elif defined(__i386__)
#define ARCH "x86"
#elif defined(__aarch64__)
#define ARCH "arm64"
#elif defined(__arm__)
#define ARCH "arm"
#else
#define ARCH "Unknown"
#endif
            ImGui::Text("OS: %s, Arch: %s\n", TARGET, ARCH);
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
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs;
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
        const float SMALL_PAD = 5.0f * Settings::scale;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMouseInputs;
        
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

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.7, 0.7, 0.7, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15.0f * Settings::scale, 5.0f * Settings::scale));

        ImGui::PushFont(fontVeryLargeSize);

        ImVec2 keySizeNoPad = ImGui::CalcTextSize("W");
        ImVec2 keySize = ImVec2(keySizeNoPad.x+ImGui::GetStyle().FramePadding.x*2, keySizeNoPad.y+ImGui::GetStyle().FramePadding.y*2);

        auto adj = [&](ImVec2 b) {
            return b;
        };

        ImGui::SetCursorPos(adj(ImVec2(SMALL_PAD + keySize.x, work_pos.y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(WindowCallbacks::mapImGuiKey((KeyCode)(GameOptions::fullKeyboard ? GameOptions::upKeyFullKeyboard : GameOptions::upKey)))->Down ? 0.70f : 0.2f); // Transparent background
        ImVec2 size;
        
        if (ImGui::BeginChild("W", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%c",(GameOptions::fullKeyboard ? GameOptions::upKeyFullKeyboard : GameOptions::upKey));
            size = ImGui::GetWindowSize();
        }
        ImGui::EndChild();
        
        auto x = work_pos.x;
        auto y = work_pos.y + size.y + SMALL_PAD;
        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(WindowCallbacks::mapImGuiKey((KeyCode)(GameOptions::fullKeyboard ? GameOptions::leftKeyFullKeyboard : GameOptions::leftKey)))->Down ? 0.70f : 0.2f); // Transparent background
        if (ImGui::BeginChild("A", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%c",(GameOptions::fullKeyboard ? GameOptions::leftKeyFullKeyboard : GameOptions::leftKey));
            auto pos = ImGui::GetWindowPos();
            size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(WindowCallbacks::mapImGuiKey((KeyCode)(GameOptions::fullKeyboard ? GameOptions::downKeyFullKeyboard : GameOptions::downKey)))->Down ? 0.70f : 0.2f); // Transparent background
        if (ImGui::BeginChild("S", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%c",(GameOptions::fullKeyboard ? GameOptions::downKeyFullKeyboard : GameOptions::downKey));
            auto pos = ImGui::GetWindowPos();
            auto size = ImGui::GetWindowSize();
            x += SMALL_PAD + size.x;
        }
        ImGui::EndChild();

        ImVec2 finalSize = ImVec2(x, keySize.y);

        ImGui::SetCursorPos(adj(ImVec2(x, y)));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(ImGui::GetKeyData(WindowCallbacks::mapImGuiKey((KeyCode)(GameOptions::fullKeyboard ? GameOptions::rightKeyFullKeyboard : GameOptions::rightKey)))->Down ? 0.70f : 0.2f); // Transparent background
        if (ImGui::BeginChild("D", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, window_flags & ~ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%c",(GameOptions::fullKeyboard ? GameOptions::rightKeyFullKeyboard : GameOptions::rightKey));
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
            CenterText(cpsSize.x, 5 * Settings::scale, "LMB");
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
            CenterText(cpsSize.x, 5 * Settings::scale, "RMB");
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
    // Custom Windows
    if(activeWindowsLock.try_lock()) {

        for(size_t i = 0, len = activeWindows.size(); i < len; i++) {
            
            if(activeWindows[i]->isModal && !activeWindows[i]->modalOpened) {
                activeWindows[i]->modalOpened = true;
                ImGui::OpenPopup(activeWindows[i]->title.data());
            }
            if(activeWindows[i]->isModal ? ImGui::BeginPopupModal(activeWindows[i]->title.data(), &activeWindows[i]->open) : ImGui::Begin(activeWindows[i]->title.data(), &activeWindows[i]->open)) {
                
                for(auto&& control : activeWindows[i]->controls) {
                    switch (control.type)
                    {
                    case 0:
                        if(ImGui::Button(control.data.button.label.data())) {
                            control.data.button.onClick(control.data.button.user);
                        }
                        break;
                    case 1:
                        if(control.data.sliderint.label.length() > 0) {
                            ImGui::Text("%s", control.data.sliderint.label.data());
                        }
                        if(ImGui::SliderInt(control.data.sliderint.label.data(), &control.data.sliderint.def, control.data.sliderint.min, control.data.sliderint.max)) {
                            control.data.sliderint.onChange(control.data.sliderint.user, control.data.sliderint.def);
                        }
                        break;
                    case 2:
                        if(control.data.sliderfloat.label.length() > 0) {
                            ImGui::Text("%s", control.data.sliderfloat.label.data());
                        }
                        if(ImGui::SliderFloat(control.data.sliderfloat.label.data(), &control.data.sliderfloat.def, control.data.sliderfloat.min, control.data.sliderfloat.max)) {
                            control.data.sliderfloat.onChange(control.data.sliderfloat.user, control.data.sliderfloat.def);
                        }
                        break;
                    case 3: {
                        ImFont *font = nullptr;
                        switch(control.data.text.size) {
                            case 1:
                            font = fontMediumSize;
                            break;
                            case 2:
                            font = fontLargeSize;
                            break;
                            case 3:
                            font = fontVeryLargeSize;
                            break;
                        }
                        if(font) {
                            ImGui::PushFont(font);
                        }
                        if(control.data.text.label.length() > 0) {
                            ImGui::Text("%s", control.data.text.label.data());
                        }
                        if(font) {
                            ImGui::PopFont();
                        }
                        break;
                    }
                    case 4:
                        ImGui::InputTextWithHint(control.data.textinput.label.data(), control.data.textinput.label.data(), control.data.textinput.def.data(), control.data.textinput.def.capacity() + 1, ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit, [](ImGuiInputTextCallbackData* ev) -> int{
                            if(ev->EventFlag == ImGuiInputTextFlags_CallbackResize)
                            {
                                std::string& str = ((WindowControl*)ev->UserData)->data.textinput.def;
                                str.resize(ev->BufTextLen);
                                ev->Buf = (char*)str.c_str();
                            } else if(ev->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
                                ((WindowControl*)ev->UserData)->data.textinput.onChange(((WindowControl*)ev->UserData)->data.textinput.user, ev->Buf);
                            }
                            return 0;
                        }, &control);
                        break;
                    default:
                        break;
                    }
                }
                if(activeWindows[i]->isModal) {
                    ImGui::EndPopup();
                } else {
                    ImGui::End();
                }
            }
            if(!activeWindows[i]->open) {
                activeWindows[i]->onClose(activeWindows[i]->user);
                activeWindows.erase(activeWindows.begin() + i);
                --i;
                --len;
            }
        }
        activeWindowsLock.unlock();
    }

    onDrawCallbacksLock.lock();
    for(size_t i = 0; i < onDrawCallbacks.size(); i++) {
        onDrawCallbacks[i]();
    }
    onDrawCallbacksLock.unlock();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if(reloadFont) {
        ReloadFont();
    }
}
