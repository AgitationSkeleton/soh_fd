// FD (2026-07-26): Fierce Deity Tunic Color
// -----------------------------------------------------------------------------
// Recolors ONLY the Fierce Deity undershirt/tunic cloth by tinting its dedicated
// palette (TLUT). The CI4 undershirt texture
//   objects/object_link_boy/object_link_boy_Tex_008C88
// samples the dedicated 16-entry RGBA16 TLUT
//   objects/object_link_boy/object_link_boy_TLUT_008128
// which is used ONLY by that cloth (hat + tunic shoulders/skirt/collar). Tinting
// the palette color values recolors exactly the cloth and leaves armor,
// gauntlets, black sleeves/pants, boots and face untouched. Because we only touch
// palette color values (not the combiner or env color), the model's lighting /
// shading is preserved.
//
// Gated by a toggle that defaults OFF. When OFF (or when not Fierce Deity) the
// original palette bytes are written back, so the result is byte-identical to
// vanilla FD. If the TLUT resource can't be found (e.g. fd.o2r not generated /
// FD unavailable) every path no-ops silently -- it must never crash.

#include <cstring>

#include <libultraship/bridge.h>
#include <libultraship/libultraship.h>
#include <ship/resource/ResourceManager.h>
#include <fast/resource/type/Texture.h>

#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64.h"
#include "variables.h"
#include "macros.h"
extern SaveContext gSaveContext;
}

// Fast3D texture cache. Cleared after we rewrite the palette so the CI4 texture
// is re-decoded against the new TLUT on the next draw.
extern "C" void gfx_texture_cache_clear();

#define CVAR_FD_TUNIC_COLOR_ENABLED CVAR_CHEAT("TransformationMasks.FdTunicColorEnabled")
// Color lives in the Cosmetics namespace so it appears in the Cosmetics Editor (Link group) with
// rainbow/lock/randomizer support. The Transformation Masks color picker binds to this same CVar, so both
// menus stay in sync automatically (one shared value). Default white == identity (vanilla).
#define CVAR_FD_TUNIC_COLOR CVAR_COSMETIC("Link.FierceDeityTunic")

namespace {

constexpr const char* kTlutPath = "objects/object_link_boy/object_link_boy_TLUT_008128";
constexpr size_t kTlutBytes = 32; // 16 entries * RGBA16 (2 bytes)

bool sOriginalCached = false;
uint8_t sOriginalTlut[kTlutBytes];

// Currently-APPLIED palette state so we only touch the resource / clear the
// texture cache when the desired state actually changes.
bool sTintApplied = false;
Color_RGBA8 sAppliedColor = { 255, 255, 255, 255 };

// Scale a 5-bit channel by an 8-bit factor with rounding (factor 255 == identity).
inline uint32_t ScaleChannel5(uint32_t v5, uint8_t factor) {
    uint32_t r = (v5 * (uint32_t)factor + 127u) / 255u;
    return r > 0x1F ? 0x1F : r;
}

void OnGameFrameUpdateFdTunicColor() {
    bool enabled = CVarGetInteger(CVAR_FD_TUNIC_COLOR_ENABLED, 0) != 0;
    // The color-picker widget stores its value at "<cvar>.Value" (UIWidgets.cpp CVarColorPicker),
    // so read that, not the bare cvar (which is always the default -> white -> no visible tint).
    Color_RGBA8 color = CVarGetColor(CVAR_FD_TUNIC_COLOR ".Value", Color_RGBA8{ 255, 255, 255, 255 });
    bool wantTint = enabled && LINK_IS_DEITY;

    // Skip when the desired state already matches what's applied (cheap common case).
    if (wantTint) {
        if (sTintApplied && sAppliedColor.r == color.r && sAppliedColor.g == color.g &&
            sAppliedColor.b == color.b) {
            return;
        }
    } else if (!sTintApplied) {
        return;
    }

    auto res = ResourceMgr_GetResourceByNameHandlingMQ(kTlutPath);
    if (res == nullptr) {
        return; // FD assets not present -- no-op silently.
    }
    auto tex = std::static_pointer_cast<Fast::Texture>(res);
    if (tex == nullptr || tex->ImageData == nullptr || tex->ImageDataSize < kTlutBytes) {
        return;
    }
    uint8_t* data = tex->ImageData;

    // Cache the pristine palette exactly once so restore is byte-perfect.
    if (!sOriginalCached) {
        memcpy(sOriginalTlut, data, kTlutBytes);
        sOriginalCached = true;
    }

    if (wantTint) {
        // Always scale from the ORIGINAL palette so repeated re-tints don't compound.
        for (int i = 0; i < 16; i++) {
            uint8_t hi = sOriginalTlut[i * 2];
            uint8_t lo = sOriginalTlut[i * 2 + 1];
            uint16_t v = (uint16_t)((hi << 8) | lo); // RGBA16 (5551) big-endian on N64 assets
            uint32_t r = (v >> 11) & 0x1F;
            uint32_t g = (v >> 6) & 0x1F;
            uint32_t b = (v >> 1) & 0x1F;
            uint32_t a = v & 0x1;
            r = ScaleChannel5(r, color.r);
            g = ScaleChannel5(g, color.g);
            b = ScaleChannel5(b, color.b);
            uint16_t nv = (uint16_t)((r << 11) | (g << 6) | (b << 1) | a);
            data[i * 2] = (uint8_t)((nv >> 8) & 0xFF);
            data[i * 2 + 1] = (uint8_t)(nv & 0xFF);
        }
        sTintApplied = true;
        sAppliedColor = color;
    } else {
        memcpy(data, sOriginalTlut, kTlutBytes);
        sTintApplied = false;
    }

    gfx_texture_cache_clear();
}

} // namespace

void RegisterFierceDeityTunicColor() {
    // Registered unconditionally (idempotent via COND_HOOK's static hookId); the
    // logic gates itself so it can also RESTORE the palette when disabled or when
    // the player is no longer Fierce Deity.
    COND_HOOK(OnGameFrameUpdate, true, OnGameFrameUpdateFdTunicColor);
}

static RegisterShipInitFunc initFuncFierceDeityTunicColor(RegisterFierceDeityTunicColor);
