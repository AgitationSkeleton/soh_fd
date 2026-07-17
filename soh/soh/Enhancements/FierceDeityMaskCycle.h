#ifndef FIERCE_DEITY_MASK_CYCLE_H
#define FIERCE_DEITY_MASK_CYCLE_H

// FD (2026-07-11): kaleido cycle helpers that let the first bottle slot toggle between its bottle
// content and the Fierce Deity's Mask (ITEM_MASK_DEITY). Mirrors RocsFeatherCycle.h but is gated on
// gSaveContext.ship.hasFierceDeityMask instead of a RandomizerInf flag (this is a non-rando feature).

#include <z64.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t Enhancement_GetPrevBottleDeityItem(void);
uint8_t Enhancement_GetNextBottleDeityItem(void);

// FD (2026-07-13) save-audit: the displaced-bottle memory that the save path uses to write the real bottle
// (never the fake mask id) into a save's SLOT_BOTTLE_1.
uint8_t Enhancement_GetDeityMaskStoredBottle(void);

// FD (2026-07-13) save-audit: scrub the fake ITEM_MASK_DEITY out of an inventory items array, restoring the
// displaced bottle. Called on the SAVE COPY (SaveManager) so a pause-menu save -- where the gameplay guard is
// frozen and can't have reverted the slot -- can never persist the mask over the bottle it shares the slot with.
void Enhancement_ScrubDeityMaskFromItems(uint8_t* items);

// FD (2026-07-13) save-audit: reset the runtime cycle statics (displaced bottle + selected face) when a save file
// is loaded, so the shared-slot state never leaks between files. (Roc's Feather / trade slots don't need this --
// their "selection" is the real slot value and is inherently per-file.)
void Enhancement_ResetDeityMaskCycleState(void);

// Clears the mask out of the real SLOT_BOTTLE_1 slot during gameplay so a save never persists it over the
// bottle it shares the slot with. Called every frame from Player_UpdateCommon.
void Enhancement_RestoreDeityMaskBottleSlot(void);

// FD (2026-07-12) #12: remember whether the mask (vs the bottle) was the A-scrolled face of the shared slot
// across pause/unpause. The gameplay guard above resets the slot to the bottle between menu sessions (for save
// safety), so unlike Roc's Feather the slot can't be its own memory -- track the choice in a runtime flag.
// TrackDeityMaskMenuSelection: called each pause-menu frame to record the current face.
// ApplyDeityMaskMenuSelection: called when the pause menu opens to re-display the mask if it was last selected.
void Enhancement_TrackDeityMaskMenuSelection(void);
void Enhancement_ApplyDeityMaskMenuSelection(void);

#ifdef __cplusplus
}
#endif

#endif
