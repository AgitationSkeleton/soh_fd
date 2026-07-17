#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/JsonConversions.hpp"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h" // FD (2026-07-12): Player_GetFdTransformAnimId (Anchor transform-mask networking)
extern PlayState* gPlayState;
// FD (2026-07-14): the FD mask's get-item uses this custom draw func (its gid is the Goron-mask flow-gate id, so
// remotes can't tell it apart by gid alone); we sync a bit when the local player is holding it up.
void FierceDeity_DrawGiMask(PlayState* play, GetItemEntry* getItemEntry);
}

/**
 * PLAYER_UPDATE
 *
 * Contains real-time data necessary to update other clients in the same scene as the player
 *
 * Sent every frame to other clients within the same scene
 *
 * Note: This packet is sent _a lot_, so please do not include any unnecessary data in it
 */

void Anchor::SendPacket_PlayerUpdate() {
    if (!IsSaveLoaded()) {
        return;
    }

    uint32_t currentPlayerCount = 0;
    for (auto& [clientId, client] : clients) {
        if (client.sceneNum == gPlayState->sceneNum && client.online && client.isSaveLoaded && !client.self) {
            currentPlayerCount++;
        }
    }
    if (currentPlayerCount == 0) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);
    nlohmann::json payload;

    payload["type"] = PLAYER_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["entranceIndex"] = gSaveContext.entranceIndex;
    payload["linkAge"] = gSaveContext.linkAge;
    payload["posRot"]["pos"] = player->actor.world.pos;
    payload["posRot"]["rot"] = player->actor.shape.rot;
    std::vector<int> jointArray;
    for (size_t i = 0; i < 24; i++) {
        Vec3s joint = player->skelAnime.jointTable[i];
        jointArray.push_back(joint.x);
        jointArray.push_back(joint.y);
        jointArray.push_back(joint.z);
    }
    payload["prevTransl"] = player->skelAnime.prevTransl;
    payload["movementFlags"] = player->skelAnime.movementFlags;
    payload["jointTable"] = jointArray;
    payload["upperLimbRot"] = player->upperLimbRot;
    payload["currentBoots"] = player->currentBoots;
    payload["currentShield"] = player->currentShield;
    payload["currentTunic"] = player->currentTunic;
    payload["stateFlags1"] = player->stateFlags1;
    payload["stateFlags2"] = player->stateFlags2 & ~PLAYER_STATE2_DISABLE_DRAW;
    payload["buttonItem0"] = gSaveContext.equips.buttonItems[0];
    payload["itemAction"] = player->itemAction;
    payload["heldItemAction"] = player->heldItemAction;
    payload["modelGroup"] = player->modelGroup;
    payload["invincibilityTimer"] = player->invincibilityTimer;
    payload["unk_862"] = player->unk_862;
    payload["unk_85C"] = player->unk_85C;
    payload["actionVar1"] = player->av1.actionVar1;
    // FD (2026-07-12): Fierce Deity transform-cutscene state so remotes see the mask donning/scream + morph.
    payload["fdStateFlags3"] = player->stateFlags3;
    payload["fdTransformTargetForm"] = player->transformTargetForm;
    payload["fdTransformPrevForm"] = player->transformPreviousForm;
    payload["fdTransformAnim"] = Player_GetFdTransformAnimId(player);
    payload["fdTransformCurFrame"] = player->skelAnime.curFrame;
    payload["fdTransformMod2"] = player->transformMatrixModifiers[2];
    payload["fdTransformMod3"] = player->transformMatrixModifiers[3];
    payload["fdTransformTimer2"] = player->transformEventTimer2;
    // FD (2026-07-14): true while this player is holding up the FD mask get-item, so remotes draw the FD mask model
    // (via its custom draw func) instead of the raw Goron mask its gid maps to.
    payload["fdMaskGi"] = (player->unk_862 > 0) && (player->getItemEntry.drawFunc == FierceDeity_DrawGiMask);
    payload["quiet"] = true;

    for (auto& [clientId, client] : clients) {
        if (client.sceneNum == gPlayState->sceneNum && client.online && client.isSaveLoaded && !client.self) {
            payload["targetClientId"] = clientId;
            SendJsonToRemote(payload);
        }
    }
}

void Anchor::HandlePacket_PlayerUpdate(nlohmann::json payload) {
    uint32_t clientId = payload["clientId"].get<uint32_t>();

    if (clients.contains(clientId)) {
        auto& client = clients[clientId];

        if (client.linkAge != payload.value("linkAge", (s32)LINK_AGE_ADULT)) {
            shouldRefreshActors = true;
        }

        client.sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
        client.entranceIndex = payload.value("entranceIndex", (s32)0);
        client.linkAge = payload.value("linkAge", (s32)LINK_AGE_ADULT);
        client.posRot = payload.value("posRot", PosRot{ 0 });
        std::vector<int> jointArray = payload.value("jointTable", std::vector<int>{});
        jointArray.resize(24 * 3); // Ensure it has enough elements, in case of missing data
        for (int i = 0; i < 24; i++) {
            client.jointTable[i].x = jointArray[i * 3];
            client.jointTable[i].y = jointArray[i * 3 + 1];
            client.jointTable[i].z = jointArray[i * 3 + 2];
        }
        client.movementFlags = payload.value("movementFlags", (u8)0);
        client.prevTransl = payload.value("prevTransl", Vec3s{ 0 });
        client.upperLimbRot = payload.value("upperLimbRot", Vec3s{ 0 });
        client.currentBoots = payload.value("currentBoots", (s8)0);
        client.currentShield = payload.value("currentShield", (s8)0);
        client.currentTunic = payload.value("currentTunic", (s8)0);
        client.stateFlags1 = payload.value("stateFlags1", (u32)0);
        client.stateFlags2 = payload.value("stateFlags2", (u32)0);
        client.buttonItem0 = payload.value("buttonItem0", (u8)0);
        client.itemAction = payload.value("itemAction", (s8)0);
        client.heldItemAction = payload.value("heldItemAction", (s8)0);
        client.modelGroup = payload.value("modelGroup", (u8)0);
        client.invincibilityTimer = payload.value("invincibilityTimer", (s8)0);
        client.unk_862 = payload.value("unk_862", (s16)0);
        client.unk_85C = payload.value("unk_85C", (f32)0);
        client.actionVar1 = payload.value("actionVar1", (s8)0);
        // FD (2026-07-12): Fierce Deity transform-cutscene state (defaults keep pre-FD clients / non-FD frames inert).
        client.fdStateFlags3 = payload.value("fdStateFlags3", (u16)0);
        client.fdTransformTargetForm = payload.value("fdTransformTargetForm", (u8)0);
        client.fdTransformPrevForm = payload.value("fdTransformPrevForm", (u8)0);
        client.fdTransformAnim = payload.value("fdTransformAnim", (u8)0);
        client.fdTransformCurFrame = payload.value("fdTransformCurFrame", (f32)0);
        client.fdTransformMod2 = payload.value("fdTransformMod2", (f32)0);
        client.fdTransformMod3 = payload.value("fdTransformMod3", (f32)0);
        client.fdTransformTimer2 = payload.value("fdTransformTimer2", (s16)0);
        client.fdMaskGi = payload.value("fdMaskGi", false);
    }
}
