#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class GraphicsCapabilityReport {
public:
    struct MinecraftIdentity {
        std::string package;
        std::string versionName;
        std::int64_t versionCode;
        std::string derivedVersion;
    };

    struct RunConfiguration {
        std::string requestedGraphicsApi;
        int windowWidth;
        int windowHeight;
        bool disableFmod;
        bool texturePatch;
        bool webrtcDebug;
        bool stdinImport;
        bool resetSettings;
        bool freeOnly;
        bool emulateTouch;
        bool customGameDirectory;
        bool customDataDirectory;
        bool customCacheDirectory;
        bool importFileRequested;
        bool uriRequested;
        std::size_t additionalModDirectoryCount;
    };

    GraphicsCapabilityReport(std::string outputPath, std::string experimentId,
                             const RunConfiguration& runConfiguration);
    ~GraphicsCapabilityReport();

    GraphicsCapabilityReport(const GraphicsCapabilityReport&) = delete;
    GraphicsCapabilityReport& operator=(const GraphicsCapabilityReport&) = delete;

    void recordMinecraftIdentity(const MinecraftIdentity& identity);
    void recordMinecraftManifestNotFound();
    void recordMinecraftManifestParseFailed();
    void refreshEnvironment();
    void finishPartial();

    // Writes a complete replacement snapshot. On failure, the prior report is
    // preserved and a path-free diagnostic is returned in error.
    bool writeSnapshot(std::string& error) noexcept;

private:
    void recordMinecraftUnavailable(const char* code, const char* message, bool fatal);

    struct Impl;
    std::unique_ptr<Impl> impl;
};
