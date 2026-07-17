#include "soh/Enhancements/FierceDeityMaskCycle.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"

// FD (2026-07-11): Host the Fierce Deity's Mask on the FIRST bottle slot (SLOT_BOTTLE_1) in the kaleido
// item grid, cyclable with the engine's press-A-then-stick mechanism (see z_kaleido_item.c). This mirrors
// RocsFeatherCycle.c, but instead of toggling two fixed items it toggles the slot between whatever bottle
// content is there and the mask. Because bottle contents vary (empty bottle .. big poe), we remember the
// displaced bottle in a static so cycling back restores it.
//
// NOTE: the remembered bottle is runtime-only. If the player saves with the mask sitting in the slot, the
// underlying bottle is not preserved across a reload (the save slot holds the mask). In normal play the mask
// lives on a C-button and the bottle rests in the slot, so this is an edge case.

// Bottle content item ids are contiguous: ITEM_BOTTLE (0x14) .. ITEM_POE (0x20).
static uint8_t IsBottleContent(uint8_t item) {
    return item >= ITEM_BOTTLE && item <= ITEM_POE;
}

static uint8_t sStoredBottleItem = ITEM_NONE;

// Returns the item the first bottle slot should hold when cycled. Both prev and next are the same toggle:
// bottle -> mask, mask -> stored bottle. Gated on having obtained the mask.
static uint8_t GetOtherBottleDeityItem(void) {
    if (!gSaveContext.ship.hasFierceDeityMask) {
        return ITEM_NONE;
    }

    uint8_t slotItem = gSaveContext.inventory.items[SLOT_BOTTLE_1];

    if (slotItem == ITEM_MASK_DEITY) {
        // Currently showing the mask -> go back to the stored bottle content (ITEM_NONE if none).
        return sStoredBottleItem;
    }

    // Currently showing a bottle (or an empty slot) -> remember it and swap to the mask.
    sStoredBottleItem = IsBottleContent(slotItem) ? slotItem : ITEM_NONE;
    return ITEM_MASK_DEITY;
}

uint8_t Enhancement_GetPrevBottleDeityItem(void) {
    return GetOtherBottleDeityItem();
}

uint8_t Enhancement_GetNextBottleDeityItem(void) {
    return GetOtherBottleDeityItem();
}

// FD (2026-07-12) BUG FIX: the kaleido cycle writes ITEM_MASK_DEITY into the REAL SLOT_BOTTLE_1 inventory slot
// while it's displayed. If the player saves with the mask sitting in the slot, the save persists the mask and
// the underlying bottle is lost on reload (the mask overwrites the bottle it shares the slot with). The mask is
// NOT a real inventory item -- it's tracked by gSaveContext.ship.hasFierceDeityMask and used from a C-button --
// so it must never be the persisted slot value. Restore the displaced bottle whenever we're in gameplay (called
// every frame from Player_UpdateCommon): the moment the menu closes, the slot reverts to the bottle, so a save
// can never capture the mask. Recovery: on a save already corrupted this way, sStoredBottleItem is ITEM_NONE, so
// the phantom mask is cleared to an empty slot (the original bottle content of that pre-corrupted save is gone).
void Enhancement_RestoreDeityMaskBottleSlot(void) {
    if (gSaveContext.inventory.items[SLOT_BOTTLE_1] == ITEM_MASK_DEITY) {
        gSaveContext.inventory.items[SLOT_BOTTLE_1] = sStoredBottleItem;
    }
}

// FD (2026-07-13) save-audit: expose the displaced bottle so the save path can restore it into the copy.
uint8_t Enhancement_GetDeityMaskStoredBottle(void) {
    return sStoredBottleItem;
}

// FD (2026-07-13) save-audit: scrub the fake mask id out of a given items array (the SAVE COPY), restoring the
// displaced bottle. The gameplay guard above keeps the LIVE slot clean during gameplay (so autosave is safe), but
// a Save & Quit from the PAUSE menu freezes that guard while the mask is A-scrolled into the slot -- so the save
// copy can still hold ITEM_MASK_DEITY. Restoring the bottle here means the persisted SLOT_BOTTLE_1 is ALWAYS the
// real bottle, never the mask, so the bottle can never be lost on reload.
void Enhancement_ScrubDeityMaskFromItems(uint8_t* items) {
    if (items != NULL && items[SLOT_BOTTLE_1] == ITEM_MASK_DEITY) {
        items[SLOT_BOTTLE_1] = sStoredBottleItem;
    }
}

// FD (2026-07-12) #12: runtime memory of whether the shared slot's A-scroll last showed the mask or the bottle.
// Roc's Feather/trade slots "remember" simply by leaving the chosen item in the real inventory slot, but the FD
// mask can't (the gameplay guard above wipes it back to the bottle for save safety), so track the choice here.
static uint8_t sDeityMaskSelected = 0;

// Called every pause-menu frame. During the menu the slot faithfully reflects the player's A-scroll choice
// (the gameplay guard doesn't run while paused), so latch which face is showing. It survives into the next pause
// session even after the guard resets the slot on the way back to gameplay.
void Enhancement_TrackDeityMaskMenuSelection(void) {
    if (gSaveContext.ship.hasFierceDeityMask) {
        sDeityMaskSelected = (gSaveContext.inventory.items[SLOT_BOTTLE_1] == ITEM_MASK_DEITY);
    }
}

// Called when the pause menu opens. If the mask was the last-selected face, re-display it in the slot (the
// gameplay guard reset the slot to the bottle on the previous menu close). Stash the displaced bottle so the
// cycle-back path (GetOtherBottleDeityItem) restores the right content.
void Enhancement_ApplyDeityMaskMenuSelection(void) {
    if (!gSaveContext.ship.hasFierceDeityMask) {
        return;
    }
    uint8_t slotItem = gSaveContext.inventory.items[SLOT_BOTTLE_1];
    if (slotItem == ITEM_MASK_DEITY) {
        return; // already showing the mask
    }
    // FD (2026-07-13) save-audit: put the mask into the slot on menu-open when EITHER it was the last-selected face,
    // OR there is no bottle to occupy the slot. The kaleido cursor cannot land on an EMPTY slot (it skips it), so
    // with no bottle the mask must occupy the slot by default to stay reachable -- exactly like Roc's Feather
    // sitting in the Nayru's Love slot when only the feather is owned. When a bottle IS present the bottle stays the
    // default (and the mask is reached by A-scrolling). The per-frame gameplay guard clears the mask back out of the
    // slot on the way to gameplay, so this never persists over a real bottle.
    if (sDeityMaskSelected || !IsBottleContent(slotItem)) {
        sStoredBottleItem = IsBottleContent(slotItem) ? slotItem : ITEM_NONE;
        gSaveContext.inventory.items[SLOT_BOTTLE_1] = ITEM_MASK_DEITY;
    }
}

// FD (2026-07-13) save-audit: reset per-file cycle state when a save file loads, so the displaced-bottle memory
// and the selected-face flag never leak from one file into another (which could show the mask on a file that
// wasn't using it, or restore the wrong bottle). Called from SaveManager's base-load path.
void Enhancement_ResetDeityMaskCycleState(void) {
    sStoredBottleItem = ITEM_NONE;
    sDeityMaskSelected = 0;
}
