#ifndef Z_KALEIDO_SCOPE_H
#define Z_KALEIDO_SCOPE_H

#include <libultraship/libultra.h>
#include "global.h"

extern u8 gAmmoItems[];
extern s16 D_8082AAEC[];
extern s16 D_8082AB2C[];
extern u8 gEquipAgeReqs[][4];
extern u8 gSlotAgeReqs[];
extern u8 gItemAgeReqs[];
extern u8 gAreaGsFlags[];

#define MAP_48x85_TEX_WIDTH 48
#define MAP_48x85_TEX_HEIGHT 85
#define MAP_48x85_TEX_SIZE ((MAP_48x85_TEX_WIDTH * MAP_48x85_TEX_HEIGHT) / 2) // 48x85 CI4 texture

#define AGE_REQ_ADULT LINK_AGE_ADULT
#define AGE_REQ_CHILD LINK_AGE_CHILD
#define AGE_REQ_NONE 9

// FD (2026-07-12): while Fierce Deity, item/equipment usability is governed entirely by Parameter_CanUseItem
// (the deity allowlist), NOT the age tables -- linkAge==LINK_AGE_DEITY matches no age-req entry, so an AND
// would wrongly gray FD-usable age-locked trade items. FD can't change ANY equipment (all swords/shields/
// tunics/boots are 0 in the deity table), so CHECK_AGE_REQ_EQUIP is a flat deny for FD (#10). Slot->item via
// the inventory. Non-FD keeps vanilla behavior unchanged. Parameter_CanUseItem decl: functions.h.
// FD (2026-07-15): the "Unrestrict Items for FD" cheat at level 2 ("All except Swords + Shields") lets FD equip
// TUNICS and BOOTS (rows EQUIP_TYPE_TUNIC/BOOTS) -- both un-grayed AND equippable, since this macro drives the
// name/icon gray-out and the equip action alike. Swords + shields (rows 0/1) stay denied. Level 0/1 keep the
// flat deny. (Kept independent of Parameter_CanUseItem because that oracle takes ITEM_* ids, not equip rows.)
#define CHECK_AGE_REQ_EQUIP(i, j) (LINK_IS_DEITY ? (((CVarGetInteger(CVAR_CHEAT("TransformationMasks.FdUnrestrictItems"), 0) >= 2) && (((i) == EQUIP_TYPE_TUNIC) || ((i) == EQUIP_TYPE_BOOTS))) ? 1 : 0) : (CVarGetInteger(CVAR_CHEAT("TimelessEquipment"), 0) || (gEquipAgeReqs[i][j] == AGE_REQ_NONE) || (gEquipAgeReqs[i][j] == ((void)0, gSaveContext.linkAge))))
#define CHECK_AGE_REQ_SLOT(slotIndex) (LINK_IS_DEITY ? Parameter_CanUseItem(gSaveContext.inventory.items[slotIndex]) : (CVarGetInteger(CVAR_CHEAT("TimelessEquipment"), 0) || (gSlotAgeReqs[slotIndex] == AGE_REQ_NONE) || gSlotAgeReqs[slotIndex] == ((void)0, gSaveContext.linkAge)))
#define CHECK_AGE_REQ_ITEM(itemIndex) (LINK_IS_DEITY ? Parameter_CanUseItem(itemIndex) : (CVarGetInteger(CVAR_CHEAT("TimelessEquipment"), 0) || (gItemAgeReqs[itemIndex] == AGE_REQ_NONE) || (gItemAgeReqs[itemIndex] == gSaveContext.linkAge)))

void KaleidoScope_DrawQuestStatus(PlayState* play, GraphicsContext* gfxCtx);
s32 KaleidoScope_UpdateQuestStatusPoint(PauseContext* pauseCtx, s32 point);
void KaleidoScope_DrawDebugEditor(PlayState* play);
void KaleidoScope_DrawPlayerWork(PlayState* play);
void KaleidoScope_DrawEquipment(PlayState* play);
void KaleidoScope_SetCursorVtx(PauseContext* pauseCtx, u16 index, Vtx* vtx);
void KaleidoScope_DrawItemSelect(PlayState* play);
void KaleidoScope_SetupItemEquip(PlayState* play, u16 item, u16 slot, s16 animX, s16 animY);
void KaleidoScope_UpdateItemEquip(PlayState* play);
void KaleidoScope_DrawDungeonMap(PlayState* play, GraphicsContext* gfxCtx);
void KaleidoScope_DrawWorldMap(PlayState* play, GraphicsContext* gfxCtx);
void KaleidoScope_UpdatePrompt(PlayState* play);
Gfx* KaleidoScope_QuadTextureIA4(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
Gfx* KaleidoScope_QuadTextureIA8(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
void KaleidoScope_MoveCursorToSpecialPos(PlayState* play, u16 specialPos);
void KaleidoScope_DrawQuadTextureRGBA32(GraphicsContext* gfxCtx, void* texture, u16 width, u16 height, u16 point);
void KaleidoScope_ProcessPlayerPreRender();
void KaleidoScope_SetupPlayerPreRender(PlayState* play);
void KaleidoScope_DrawCursor(PlayState* play, u16 pageIndex);
void KaleidoScope_UpdateDungeonMap(PlayState* play);

void PauseMapMark_Draw(PlayState* play);

void KaleidoScope_UpdateCursorSize(PauseContext* pauseCtx);

void KaleidoScope_ResetItemCycling();

#endif
