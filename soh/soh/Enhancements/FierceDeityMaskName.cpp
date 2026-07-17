// FD (2026-07-11): Draw the Fierce Deity's Mask item-name banner in the kaleido pause menu.
//
// The vanilla name-draw in z_kaleido_scope_PAL.c indexes iconNameTextures[] by (namedItem % 123), which
// would resolve to the wrong texture for the custom ITEM_MASK_DEITY (0x9F). We override it here via the
// VB_DRAW_CUSTOM_ITEM_NAME hook (same mechanism Roc's Feather uses) and memcpy the correct name texture.
// This is a new file on purpose (FierceDeityMask.cpp is off-limits) and is CVar-gated for normal gameplay.

#include <soh/OTRGlobals.h>
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include <soh_assets.h>

extern "C" {
#include <z64.h>
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
}

void RegisterFierceDeityMaskName() {
    bool shouldRegister = CVarGetInteger(CVAR_ENHANCEMENT("TransformationMasks.Enabled"), 1);

    COND_VB_SHOULD(VB_DRAW_CUSTOM_ITEM_NAME, shouldRegister, {
        u32 namedItem = va_arg(args, u32);
        if (namedItem == ITEM_MASK_DEITY) {
            *should = true;
            const char* textureName = gFierceDeityMaskItemNameENGTex;
            memcpy(gPlayState->pauseCtx.nameSegment, textureName, strlen(textureName) + 1);
        }
    });
}

static RegisterShipInitFunc registerFierceDeityMaskName(RegisterFierceDeityMaskName,
                                                        { CVAR_ENHANCEMENT("TransformationMasks.Enabled") });
