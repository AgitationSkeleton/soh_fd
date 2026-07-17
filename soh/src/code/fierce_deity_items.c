/*
 * fierce_deity_items.c
 *
 * FD (2026-07-11): Acquisition helpers for the custom Fierce Deity mask (ITEM_MASK_DEITY, 0x9F).
 *
 * ITEM_MASK_DEITY is a custom item id that lives well outside the vanilla item/slot tables
 * (gItemSlots[] only has 0x36 entries). It therefore has no inventory-grid slot and cannot be
 * stored/equipped through the normal vanilla machinery. Instead we equip it directly to a
 * C-button: SoH draws C-button icons from gItemIcons[buttonItems[n]] (already wired to
 * gFierceDeityMaskTex), and pressing that C-button routes ITEM_MASK_DEITY through
 * sItemActions[ITEM_MASK_DEITY] == PLAYER_IA_MASK_DEITY, which triggers the transform branch in
 * z_player.c. So a C-button is exactly the "usable" state we want the mask to land in.
 */
#include "global.h"
#include "objects/object_link_deity/object_link_deity.h" // FD mask GI DL resource-path symbols

// FD (2026-07-11): custom get-item draw for the Fierce Deity mask -- draws the real MM FD mask model
// (object_gi_mask03: face + hair/hat) via ResourceMgr_LoadGfxByName, replacing the Goron-mask fallback.
// Wired onto the GetItemEntry.drawFunc; the get-item system calls it (z_draw.c:411 / z_player.c:14951)
// with the hold-up matrix already applied.
void FierceDeity_DrawGiMask(PlayState* play, GetItemEntry* getItemEntry) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    // FD (2026-07-11): pass the OTR resource-path char[] DIRECTLY to gSPDisplayList (LUS resolves the
    // __OTR__ path), exactly like Randomizer_DrawRocsFeather. Wrapping in ResourceMgr_LoadGfxByName
    // returned a bad pointer and crashed the GFX interpreter.
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gGiFierceDeityMaskFaceDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gGiFierceDeityMaskHairAndHatDL);
    CLOSE_DISPS(play->state.gfxCtx);
}

// FD (2026-07-11): custom textId for the FD-mask acquisition textbox. It is served at runtime by
// the OnOpenText hook + CustomMessageManager registration in soh/Enhancements/FierceDeityMask.cpp.
// Keep this value in sync with TEXT_FIERCE_DEITY_MASK there.
#define FD_MASK_TEXT_ID 0x9300

// FD (2026-07-11): Equip ITEM_MASK_DEITY to the first free C-button (falling back to C-Right if all
// three are occupied) so it can be used to transform into Fierce Deity. No DMA icon load is needed
// because SoH resolves the C-button icon through gItemIcons[]. Also called from Item_Give() as the
// safe storage path for this custom item (see z_parameter.c).
void FierceDeity_EquipMaskToCButton(void) {
    s32 i;
    s32 button = -1;

    // Already equipped to a C-button? Nothing to do.
    for (i = 1; i <= 3; i++) {
        if (gSaveContext.equips.buttonItems[i] == ITEM_MASK_DEITY) {
            return;
        }
    }

    // Prefer the first empty C-button: C-Left = 1, C-Down = 2, C-Right = 3.
    for (i = 1; i <= 3; i++) {
        if (gSaveContext.equips.buttonItems[i] == ITEM_NONE) {
            button = i;
            break;
        }
    }
    if (button < 0) {
        button = 3; // all C-buttons full: overwrite C-Right.
    }

    gSaveContext.equips.buttonItems[button] = ITEM_MASK_DEITY;
    gSaveContext.equips.cButtonSlots[button - 1] = SLOT_NONE; // custom item: no inventory-grid slot.
    gSaveContext.buttonStatus[button] = BTN_ENABLED;
}

// FD (2026-07-11): Grant the FD mask through the full get-item acquisition sequence (Link holds the
// item overhead, custom textbox, item-get fanfare). The grant itself is completed by the player
// code, which calls Item_Give(play, ITEM_MASK_DEITY); Item_Give routes that custom id to
// FierceDeity_EquipMaskToCButton() (see z_parameter.c).
void GiveFierceDeityMask(void) {
    if (gPlayState == NULL) {
        return;
    }

    // The GET_ITEM object/gid/gi still reference a valid vanilla mask so the get-item FLOW gates pass
    // (objectId != INVALID, getItemId != NONE), but drawFunc below OVERRIDES the held model with the
    // real Fierce Deity mask -- so the Goron model is never actually drawn.
    GetItemEntry fdMask = GET_ITEM(ITEM_MASK_DEITY,       // itemId  -> what Item_Give() receives
                                   OBJECT_GI_GOLONMASK,   // objectId (flow gate only; model overridden)
                                   GID_MASK_GORON,        // drawId  (flow gate only; model overridden)
                                   FD_MASK_TEXT_ID,       // textId  (custom acquisition message)
                                   0x80,                  // field   (matches the vanilla masks)
                                   CHEST_ANIM_LONG, ITEM_CATEGORY_MAJOR, MOD_NONE,
                                   GI_MASK_GORON);         // getItemId (must be a valid GI for the draw)
    fdMask.drawFunc = FierceDeity_DrawGiMask; // FD (2026-07-11): draw the real FD mask model, not the Goron fallback

    // Show the overhead-hold acquisition animation + textbox + fanfare. Item_Give() (called by the
    // player code when the animation completes) performs the actual grant via the guard above.
    if (!GiveItemEntryWithoutActor(gPlayState, fdMask)) {
        // Player was in a state that can't receive an item (mid-air, carrying, cutscene, etc.).
        // Fall back to granting silently so the button still does something useful.
        Item_Give(gPlayState, ITEM_MASK_DEITY);
    }
}
