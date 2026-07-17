#ifndef FD_O2R_GEN_H
#define FD_O2R_GEN_H

#include <string>

// FD (2026-07-16): boot-time generation of fd.o2r from the user's own Majora's Mask ROM, mirroring the way
// Ship of Harkinian generates oot.o2r from an Ocarina of Time ROM. No Nintendo-derived assets ship in this
// repository; the Fierce Deity form's MM resources are extracted locally on first launch. The only bundled
// resources are original works (Aegiker's authored transform sounds, the swirl effect, the MM_Jumps community
// animations, and hand-patched display lists) shipped under assets/fd_bundled/.
namespace FdO2rGen {

// True if fd.o2r is not present in any app directory (so the Fierce Deity form would be unavailable).
bool NeedsGeneration(const std::string& appShortName);

// Prompt for and generate fd.o2r from a Majora's Mask ROM, writing it next to oot.o2r in the app directory.
// Returns true on success. Returns false (without aborting boot) if the user declines or no valid MM ROM is
// available — the game then launches as normal SoH without the Fierce Deity form.
bool Generate(const std::string& installPath, const std::string& dataPath, const std::string& appShortName);

} // namespace FdO2rGen

#endif // FD_O2R_GEN_H
