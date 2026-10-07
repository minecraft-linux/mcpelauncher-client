#pragma once

// Workaround for Google PairIP DRM protection of libPlayFabMultiplayer.so on
// Android x86_64 builds (deliberately scrambles PLT/.got.plt entries and
// encrypts the .data section). This is launcher-side fixup applied at runtime
// after the library has been loaded as a transitive dependency of
// libminecraftpe.so.
//
// On non-x86_64 architectures this is a no-op stub so the call site can be
// unconditional.

namespace mcpelauncher {

// Apply the PairIP PLT/.data fixups to libPlayFabMultiplayer.so.
//
// Must be called AFTER libminecraftpe.so has been loaded (which transitively
// loads libPlayFabMultiplayer.so) and BEFORE the game's main thread runs.
// If libPlayFabMultiplayer.so is not loaded, this is a no-op.
void apply_pairip_plt_workaround();

} // namespace mcpelauncher
