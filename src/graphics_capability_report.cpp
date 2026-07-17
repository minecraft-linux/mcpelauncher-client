#include "graphics_capability_report.h"

#include <build_info.h>
#include <mcpelauncher/path_helper.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#ifdef MCPELAUNCHER_HAS_OPENSSL
#include <openssl/crypto.h>
#include <openssl/opensslv.h>
#endif

#ifdef MCPELAUNCHER_HAS_SDL3
#include <SDL3/SDL_version.h>
#endif

#ifdef MCPELAUNCHER_HAS_GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

#include <sys/stat.h>
#include <sys/utsname.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace {

constexpr const char* FORMAT_NAME = "mcpelauncher.graphics-capability-report";
constexpr int FORMAT_VERSION = 1;

json stringOrNull(const char* value) {
    if(value == nullptr || value[0] == '\0') {
        return nullptr;
    }
    return value;
}

json stringOrNull(const std::string& value) {
    return value.empty() ? json(nullptr) : json(value);
}

json boolOrNull(const char* value) {
    if(value == nullptr) {
        return nullptr;
    }
    if(strcmp(value, "true") == 0 || strcmp(value, "TRUE") == 0 || strcmp(value, "ON") == 0 || strcmp(value, "1") == 0) {
        return true;
    }
    if(strcmp(value, "false") == 0 || strcmp(value, "FALSE") == 0 || strcmp(value, "OFF") == 0 || strcmp(value, "0") == 0) {
        return false;
    }
    return nullptr;
}

json sourceIdentity(const char* version, const char* describe, const char* commit, const char* dirty) {
    return {
        {"version", stringOrNull(version)},
        {"git_describe", stringOrNull(describe)},
        {"git_commit", stringOrNull(commit)},
        {"worktree_dirty_at_configure", boolOrNull(dirty)}
    };
}

std::string dottedVersion(int major, int minor, int patch) {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::vector<std::string> splitList(const char* value, char delimiter) {
    std::vector<std::string> result;
    if(value == nullptr || value[0] == '\0') {
        return result;
    }

    std::string input(value);
    std::size_t offset = 0;
    while(offset <= input.size()) {
        auto end = input.find(delimiter, offset);
        auto entry = input.substr(offset, end == std::string::npos ? std::string::npos : end - offset);
        if(!entry.empty()) {
            result.push_back(std::move(entry));
        }
        if(end == std::string::npos) {
            break;
        }
        offset = end + 1;
    }
    return result;
}

std::string utcTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_r(&time, &utc);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

std::string defaultExperimentId(const std::string& timestamp) {
    std::string result = "capture-";
    for(char c : timestamp) {
        if(c != '-' && c != ':') {
            result.push_back(c);
        }
    }
    return result;
}

bool validExperimentId(const std::string& value) {
    if(value.empty() || value.size() > 128) {
        return false;
    }
    auto isAsciiAlphanumeric = [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    };
    if(!isAsciiAlphanumeric(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [&](unsigned char c) {
        return isAsciiAlphanumeric(c) || c == '.' || c == '_' || c == '-';
    });
}

json makeSection(const char* status, const char* subjectLayer, json data = nullptr) {
    return {
        {"status", status},
        {"subject_layer", subjectLayer},
        {"data", std::move(data)},
        {"errors", json::array()}
    };
}

#ifdef __APPLE__
std::optional<std::string> sysctlString(const char* name) {
    std::size_t size = 0;
    if(sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0) {
        return std::nullopt;
    }
    std::vector<char> buffer(size);
    if(sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0 || size == 0) {
        return std::nullopt;
    }
    if(buffer[size - 1] == '\0') {
        --size;
    }
    return std::string(buffer.data(), size);
}

std::optional<std::uint64_t> sysctlUint64(const char* name) {
    std::uint64_t value = 0;
    std::size_t size = sizeof(value);
    if(sysctlbyname(name, &value, &size, nullptr, 0) != 0 || size != sizeof(value)) {
        return std::nullopt;
    }
    return value;
}
#endif

const char* operatingSystemName() {
#if defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#else
    return "Unknown";
#endif
}

json collectHostIdentity() {
    json kernel = {
        {"name", nullptr},
        {"release", nullptr}
    };
    json architecture = nullptr;
    struct utsname info{};
    if(uname(&info) == 0) {
        kernel["name"] = stringOrNull(info.sysname);
        kernel["release"] = stringOrNull(info.release);
        architecture = stringOrNull(info.machine);
    }

    json productVersion = nullptr;
    json build = nullptr;
    json model = nullptr;
    json processor = nullptr;
    json memoryBytes = nullptr;
#ifdef __APPLE__
    if(auto value = sysctlString("kern.osproductversion")) {
        productVersion = *value;
    }
    if(auto value = sysctlString("kern.osversion")) {
        build = *value;
    }
    if(auto value = sysctlString("hw.model")) {
        model = *value;
    }
    if(auto value = sysctlString("machdep.cpu.brand_string")) {
        processor = *value;
    }
    if(auto value = sysctlUint64("hw.memsize")) {
        memoryBytes = *value;
    }
#endif

    return {
        {"operating_system", {
            {"name", operatingSystemName()},
            {"product_version", std::move(productVersion)},
            {"build", std::move(build)},
            {"architecture", std::move(architecture)},
            {"kernel", std::move(kernel)}
        }},
        {"hardware", {
            {"model", std::move(model)},
            {"processor", std::move(processor)},
            {"memory_bytes", std::move(memoryBytes)}
        }}
    };
}

json collectDependencies() {
    json dependencies = json::object();
    for(const auto& dependency : BUILD_DEPENDENCY_REVISIONS) {
        if(dependency.name == nullptr) {
            break;
        }
        dependencies[dependency.name] = {
            {"source_revision", stringOrNull(dependency.gitCommit)},
            {"pinned_source_revision", stringOrNull(dependency.pinnedGitCommit)},
            {"source_worktree_dirty_at_configure", boolOrNull(dependency.worktreeDirty)}
        };
    }

    auto curlInfo = curl_version_info(CURLVERSION_NOW);
    json curlDependency = {
        {"compile_version", LIBCURL_VERSION},
        {"runtime_version", curlInfo == nullptr ? json(nullptr) : stringOrNull(curlInfo->version)},
        {"runtime_target", curlInfo == nullptr ? json(nullptr) : stringOrNull(curlInfo->host)},
        {"tls_backend", curlInfo == nullptr ? json(nullptr) : stringOrNull(curlInfo->ssl_version)}
    };
    dependencies["curl"] = std::move(curlDependency);

    dependencies["nlohmann_json"] = {
        {"compile_version", dottedVersion(NLOHMANN_JSON_VERSION_MAJOR, NLOHMANN_JSON_VERSION_MINOR, NLOHMANN_JSON_VERSION_PATCH)}
    };

#ifdef MCPELAUNCHER_HAS_OPENSSL
    dependencies["openssl"] = {
        {"compile_version", OPENSSL_VERSION_TEXT},
        {"runtime_version", stringOrNull(OpenSSL_version(OPENSSL_VERSION))}
    };
#endif

#ifdef MCPELAUNCHER_HAS_SDL3
    auto sdlRuntimeVersion = SDL_GetVersion();
    dependencies["sdl3"] = {
        {"compile_version", dottedVersion(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION)},
        {"runtime_version", dottedVersion(SDL_VERSIONNUM_MAJOR(sdlRuntimeVersion), SDL_VERSIONNUM_MINOR(sdlRuntimeVersion), SDL_VERSIONNUM_MICRO(sdlRuntimeVersion))},
        {"runtime_revision", stringOrNull(SDL_GetRevision())}
    };
#endif

#ifdef MCPELAUNCHER_HAS_GLFW
    int glfwMajor = 0;
    int glfwMinor = 0;
    int glfwRevision = 0;
    glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRevision);
    dependencies["glfw"] = {
        {"source_revision", stringOrNull(BUILD_GLFW_SOURCE_REVISION)},
        {"compile_version", dottedVersion(GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION)},
        {"runtime_version", dottedVersion(glfwMajor, glfwMinor, glfwRevision)},
        {"runtime_description", stringOrNull(glfwGetVersionString())}
    };
#endif

    return dependencies;
}

json collectToolchainIdentity() {
    auto architectures = splitList(BUILD_OSX_ARCHITECTURES, ';');
    std::sort(architectures.begin(), architectures.end());

    int cxxStandard = 0;
    try {
        cxxStandard = std::stoi(BUILD_CXX_STANDARD);
    } catch(const std::exception&) {
        cxxStandard = static_cast<int>(__cplusplus);
    }

    return {
        {"compiler", {
            {"id", BUILD_CXX_COMPILER_ID},
            {"version", BUILD_CXX_COMPILER_VERSION},
            {"description", __VERSION__}
        }},
        {"cmake", {
            {"version", BUILD_CMAKE_VERSION},
            {"generator", BUILD_CMAKE_GENERATOR},
            {"generator_tool_version", stringOrNull(BUILD_GENERATOR_TOOL_VERSION)}
        }},
        {"xcode", {
            {"version", stringOrNull(BUILD_XCODE_VERSION)},
            {"build", stringOrNull(BUILD_XCODE_BUILD_VERSION)}
        }},
        {"sdk", {
            {"name", stringOrNull(BUILD_SDK_NAME)},
            {"version", stringOrNull(BUILD_SDK_VERSION)},
            {"build", stringOrNull(BUILD_SDK_BUILD_VERSION)}
        }},
        {"build", {
            {"configuration", MCPELAUNCHER_BUILD_CONFIGURATION},
            {"cxx_standard", cxxStandard},
            {"target_system", BUILD_TARGET_SYSTEM},
            {"target_system_version", stringOrNull(BUILD_TARGET_SYSTEM_VERSION)},
            {"target_processor", stringOrNull(BUILD_TARGET_PROCESSOR)},
            {"osx_architectures", architectures},
            {"osx_deployment_target", stringOrNull(BUILD_OSX_DEPLOYMENT_TARGET)},
            {"options", {
                {"build_client", boolOrNull(BUILD_OPTION_BUILD_CLIENT)},
                {"build_ui", boolOrNull(BUILD_OPTION_BUILD_UI)},
                {"build_webview", boolOrNull(BUILD_OPTION_BUILD_WEBVIEW)},
                {"build_testing", boolOrNull(BUILD_OPTION_BUILD_TESTING)},
                {"use_gamecontrollerdb", boolOrNull(BUILD_OPTION_USE_GAMECONTROLLERDB)},
                {"use_own_curl", boolOrNull(BUILD_OPTION_USE_OWN_CURL)},
                {"use_sdl3_audio", boolOrNull(BUILD_OPTION_USE_SDL3_AUDIO)},
                {"no_openssl", boolOrNull(BUILD_OPTION_NO_OPENSSL)},
                {"gamewindow_system", stringOrNull(BUILD_OPTION_GAMEWINDOW_SYSTEM)}
            }}
        }}
    };
}

json collectVersionData() {
    return {
        {"source_baseline", {
            {"root", sourceIdentity(SOURCE_BASELINE_ROOT_VERSION, SOURCE_BASELINE_ROOT_GIT_DESCRIBE,
                                    SOURCE_BASELINE_ROOT_GIT_COMMIT_HASH, SOURCE_BASELINE_ROOT_GIT_WORKTREE_DIRTY)}
        }},
        {"launcher_manifest", sourceIdentity(MANIFEST_VERSION, MANIFEST_GIT_DESCRIBE,
                                             MANIFEST_GIT_COMMIT_HASH_FULL, MANIFEST_GIT_WORKTREE_DIRTY)},
        {"client", sourceIdentity(CLIENT_VERSION, CLIENT_GIT_DESCRIBE,
                                  CLIENT_GIT_COMMIT_HASH_FULL, CLIENT_GIT_WORKTREE_DIRTY)},
        {"dependencies", collectDependencies()},
        {"minecraft", {
            {"manifest_loaded", false},
            {"package", nullptr},
            {"version_name", nullptr},
            {"version_code", nullptr},
            {"derived_version", nullptr},
            {"selected_library_abi", PathHelper::getAbiDir()}
        }},
        {"host", collectHostIdentity()},
        {"toolchain", collectToolchainIdentity()}
    };
}

json collectEnvironment() {
    const char* angleValue = getenv("ANGLE_DEFAULT_PLATFORM");
    bool anglePresent = angleValue != nullptr;
    bool angleRecognized = anglePresent &&
        (strcmp(angleValue, "vulkan") == 0 || strcmp(angleValue, "metal") == 0 || strcmp(angleValue, "gl") == 0);

    const char* icdValue = getenv("VK_ICD_FILENAMES");
    bool icdPresent = icdValue != nullptr;
    std::size_t icdEntryCount = 0;
    if(icdPresent && icdValue[0] != '\0') {
        icdEntryCount = 1;
        for(const char* cursor = icdValue; *cursor != '\0'; ++cursor) {
            if(*cursor == ':') {
                ++icdEntryCount;
            }
        }
    }

    return {
        {"angle_default_platform", {
            {"present", anglePresent},
            {"recognized", angleRecognized},
            {"value", angleRecognized ? json(angleValue) : json(nullptr)}
        }},
        {"vk_icd_filenames", {
            {"present", icdPresent},
            {"entry_count", icdEntryCount}
        }}
    };
}

bool writeAll(int descriptor, const std::string& payload, std::string& error) {
    std::size_t offset = 0;
    while(offset < payload.size()) {
        auto written = write(descriptor, payload.data() + offset, payload.size() - offset);
        if(written < 0) {
            if(errno == EINTR) {
                continue;
            }
            error = std::string("Could not write the temporary report file: ") + strerror(errno);
            return false;
        }
        if(written == 0) {
            error = "Could not write the temporary report file: write returned zero bytes";
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool atomicWrite(const std::string& outputPath, const std::string& payload, std::string& error) {
    std::string temporaryTemplate = outputPath + ".tmp.XXXXXX";
    std::vector<char> temporaryPath(temporaryTemplate.begin(), temporaryTemplate.end());
    temporaryPath.push_back('\0');

    int descriptor = mkstemp(temporaryPath.data());
    if(descriptor < 0) {
        error = std::string("Could not create a temporary report file: ") + strerror(errno);
        return false;
    }

    bool success = true;
    if(fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
        error = std::string("Could not set report file permissions: ") + strerror(errno);
        success = false;
    }
    if(success) {
        success = writeAll(descriptor, payload, error);
    }
    if(success && fsync(descriptor) != 0) {
        error = std::string("Could not flush the temporary report file: ") + strerror(errno);
        success = false;
    }
    if(close(descriptor) != 0 && success) {
        error = std::string("Could not close the temporary report file: ") + strerror(errno);
        success = false;
    }

    if(!success) {
        unlink(temporaryPath.data());
        return false;
    }
    if(rename(temporaryPath.data(), outputPath.c_str()) != 0) {
        error = std::string("Could not replace the graphics report: ") + strerror(errno);
        unlink(temporaryPath.data());
        return false;
    }

    auto slash = outputPath.find_last_of('/');
    std::string parent = slash == std::string::npos ? "." : (slash == 0 ? "/" : outputPath.substr(0, slash));
    int parentDescriptor = open(parent.c_str(), O_RDONLY);
    if(parentDescriptor >= 0) {
        fsync(parentDescriptor);
        close(parentDescriptor);
    }
    return true;
}

} // namespace

struct GraphicsCapabilityReport::Impl {
    std::string outputPath;
    json document;
    std::mutex mutex;
};

GraphicsCapabilityReport::GraphicsCapabilityReport(std::string outputPath, std::string experimentId,
                                                   const RunConfiguration& runConfiguration)
    : impl(std::make_unique<Impl>()) {
    if(outputPath.empty()) {
        throw std::invalid_argument("The graphics report path must not be empty");
    }

    auto startedAt = utcTimestamp();
    if(experimentId.empty()) {
        experimentId = defaultExperimentId(startedAt);
    }
    if(!validExperimentId(experimentId)) {
        throw std::invalid_argument("The graphics report ID contains unsupported characters");
    }

    impl->outputPath = std::move(outputPath);
    impl->document = {
        {"format", {
            {"name", FORMAT_NAME},
            {"version", FORMAT_VERSION}
        }},
        {"capture", {
            {"experiment_id", std::move(experimentId)},
            {"started_at_utc", startedAt},
            {"updated_at_utc", startedAt},
            {"sequence", 0},
            {"status", "in_progress"}
        }},
        {"sections", {
            {"versions", makeSection("partial", "run_identity", collectVersionData())},
            {"run_configuration", makeSection("collected", "launcher", {
                {"options", {
                    {"requested_graphics_api", runConfiguration.requestedGraphicsApi},
                    {"window_width", runConfiguration.windowWidth},
                    {"window_height", runConfiguration.windowHeight},
                    {"disable_fmod", runConfiguration.disableFmod},
                    {"texture_patch", runConfiguration.texturePatch},
                    {"webrtc_debug", runConfiguration.webrtcDebug},
                    {"stdin_import", runConfiguration.stdinImport},
                    {"reset_settings", runConfiguration.resetSettings},
                    {"free_only", runConfiguration.freeOnly},
                    {"emulate_touch", runConfiguration.emulateTouch},
                    {"custom_game_directory", runConfiguration.customGameDirectory},
                    {"custom_data_directory", runConfiguration.customDataDirectory},
                    {"custom_cache_directory", runConfiguration.customCacheDirectory},
                    {"import_file_requested", runConfiguration.importFileRequested},
                    {"uri_requested", runConfiguration.uriRequested},
                    {"additional_mod_directory_count", runConfiguration.additionalModDirectoryCount}
                }},
                {"environment", collectEnvironment()}
            })},
            {"graphics_context", makeSection("not_collected", "host_context")},
            {"guest_egl", makeSection("not_collected", "minecraft_guest_egl")},
            {"guest_gl", makeSection("not_collected", "minecraft_guest_gl")},
            {"host_egl", makeSection("not_collected", "host_egl")},
            {"angle", makeSection("not_collected", "angle")},
            {"vulkan", makeSection("not_collected", "vulkan")},
            {"moltenvk", makeSection("not_collected", "moltenvk")},
            {"shaders", makeSection("not_collected", "minecraft_guest_gl")},
            {"validation", makeSection("not_collected", "vulkan_validation")}
        }}
    };
}

GraphicsCapabilityReport::~GraphicsCapabilityReport() = default;

void GraphicsCapabilityReport::recordMinecraftIdentity(const MinecraftIdentity& identity) {
    std::lock_guard<std::mutex> lock(impl->mutex);
    auto& versions = impl->document["sections"]["versions"];
    versions["data"]["minecraft"] = {
        {"manifest_loaded", true},
        {"package", identity.package},
        {"version_name", identity.versionName},
        {"version_code", identity.versionCode},
        {"derived_version", identity.derivedVersion},
        {"selected_library_abi", PathHelper::getAbiDir()}
    };
    versions["status"] = "collected";
    versions["errors"] = json::array();
}

void GraphicsCapabilityReport::recordMinecraftManifestNotFound() {
    recordMinecraftUnavailable("minecraft_manifest_not_found", "The Minecraft manifest was not found", false);
}

void GraphicsCapabilityReport::recordMinecraftManifestParseFailed() {
    recordMinecraftUnavailable("minecraft_manifest_parse_failed", "The Minecraft manifest could not be parsed", true);
}

void GraphicsCapabilityReport::recordMinecraftUnavailable(const char* code, const char* message, bool fatal) {
    std::lock_guard<std::mutex> lock(impl->mutex);
    auto& versions = impl->document["sections"]["versions"];
    versions["data"]["minecraft"] = {
        {"manifest_loaded", false},
        {"package", nullptr},
        {"version_name", nullptr},
        {"version_code", nullptr},
        {"derived_version", nullptr},
        {"selected_library_abi", PathHelper::getAbiDir()}
    };
    versions["status"] = "partial";
    versions["errors"].push_back({{"code", code}, {"message", message}});
    if(fatal) {
        impl->document["capture"]["status"] = "failed";
    }
}

void GraphicsCapabilityReport::refreshEnvironment() {
    std::lock_guard<std::mutex> lock(impl->mutex);
    impl->document["sections"]["run_configuration"]["data"]["environment"] = collectEnvironment();
}

void GraphicsCapabilityReport::finishPartial() {
    std::lock_guard<std::mutex> lock(impl->mutex);
    if(impl->document["capture"]["status"] == "in_progress") {
        impl->document["capture"]["status"] = "partial";
    }
}

bool GraphicsCapabilityReport::writeSnapshot(std::string& error) noexcept {
    try {
        std::lock_guard<std::mutex> lock(impl->mutex);
        json snapshot = impl->document;
        snapshot["capture"]["sequence"] = impl->document["capture"]["sequence"].get<std::uint64_t>() + 1;
        snapshot["capture"]["updated_at_utc"] = utcTimestamp();
        auto payload = snapshot.dump(2) + "\n";
        if(!atomicWrite(impl->outputPath, payload, error)) {
            return false;
        }
        impl->document["capture"] = std::move(snapshot["capture"]);
        return true;
    } catch(const std::exception& exception) {
        error = std::string("Could not serialize the graphics report: ") + exception.what();
        return false;
    } catch(...) {
        error = "Could not serialize the graphics report";
        return false;
    }
}
