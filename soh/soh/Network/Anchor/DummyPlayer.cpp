#include "Anchor.h"
#include "soh/Enhancements/nametag.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;

void Player_UseItem(PlayState* play, Player* player, s32 item);
void Player_Draw(Actor* actor, PlayState* play);
s32 Object_Spawn(ObjectContext* objectCtx, s16 objectId); // FD (2026-07-14): not in functions.h; used for FD object residency
void FierceDeity_DrawGiMask(PlayState* play, GetItemEntry* getItemEntry); // FD (2026-07-14): custom FD-mask get-item draw
}

static DamageTable DummyPlayerDamageTable = {
    /* Deku nut      */ DMG_ENTRY(0, DUMMY_PLAYER_HIT_RESPONSE_STUN),
    /* Deku stick    */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Slingshot     */ DMG_ENTRY(1, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Explosive     */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Boomerang     */ DMG_ENTRY(0, DUMMY_PLAYER_HIT_RESPONSE_STUN),
    /* Normal arrow  */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Hammer swing  */ DMG_ENTRY(2, PLAYER_HIT_RESPONSE_KNOCKBACK_LARGE),
    /* Hookshot      */ DMG_ENTRY(0, DUMMY_PLAYER_HIT_RESPONSE_STUN),
    /* Kokiri sword  */ DMG_ENTRY(1, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Master sword  */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Giant's Knife */ DMG_ENTRY(4, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Fire arrow    */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_FIRE),
    /* Ice arrow     */ DMG_ENTRY(4, PLAYER_HIT_RESPONSE_ICE_TRAP),
    /* Light arrow   */ DMG_ENTRY(2, PLAYER_HIT_RESPONSE_ELECTRIC_SHOCK),
    /* Unk arrow 1   */ DMG_ENTRY(2, PLAYER_HIT_RESPONSE_NONE),
    /* Unk arrow 2   */ DMG_ENTRY(2, PLAYER_HIT_RESPONSE_NONE),
    /* Unk arrow 3   */ DMG_ENTRY(2, PLAYER_HIT_RESPONSE_NONE),
    /* Fire magic    */ DMG_ENTRY(0, DUMMY_PLAYER_HIT_RESPONSE_FIRE),
    /* Ice magic     */ DMG_ENTRY(3, PLAYER_HIT_RESPONSE_ICE_TRAP),
    /* Light magic   */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_ELECTRIC_SHOCK),
    /* Shield        */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_NONE),
    /* Mirror Ray    */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_NONE),
    /* Kokiri spin   */ DMG_ENTRY(1, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Giant spin    */ DMG_ENTRY(4, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Master spin   */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Kokiri jump   */ DMG_ENTRY(2, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Giant jump    */ DMG_ENTRY(8, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Master jump   */ DMG_ENTRY(4, DUMMY_PLAYER_HIT_RESPONSE_NORMAL),
    /* Unknown 1     */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_NONE),
    /* Unblockable   */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_NONE),
    /* Hammer jump   */ DMG_ENTRY(4, PLAYER_HIT_RESPONSE_KNOCKBACK_LARGE),
    /* Unknown 2     */ DMG_ENTRY(0, PLAYER_HIT_RESPONSE_NONE),
};

void DummyPlayer_Init(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;

    uint32_t clientId = Anchor::Instance->GetDummyPlayerClientId(actor);

    if (!Anchor::Instance->clients.contains(clientId)) {
        Actor_Kill(actor);
        return;
    }

    AnchorClient& client = Anchor::Instance->clients[clientId];

    // Hack to account for usage of gSaveContext in Player_Init
    s32 originalAge = gSaveContext.linkAge;
    gSaveContext.linkAge = client.linkAge;

    // #region modeled after EnTorch2_Init and Player_Init
    actor->room = -1;
    player->itemAction = player->heldItemAction = -1;
    player->heldItemId = ITEM_NONE;
    Player_UseItem(play, player, ITEM_NONE);
    Player_SetModelGroup(player, Player_ActionToModelGroup(player, player->heldItemAction));
    play->playerInit(player, play, gPlayerSkelHeaders[client.linkAge]);

    // Prevent dummy players from holding a weapon trail effect slot, as they don't use it anyway
    Effect_Delete(play, player->meleeWeaponEffectIndex);
    player->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;

    play->func_11D54(player, play);
    // #endregion

    player->cylinder.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    player->cylinder.base.ocFlags2 = OC2_TYPE_1;
    player->cylinder.info.bumperFlags = BUMP_ON | BUMP_HOOKABLE | BUMP_NO_HITMARK;
    player->actor.flags |= ACTOR_FLAG_HOOKSHOT_PULLS_PLAYER;
    player->cylinder.dim.radius = 30;
    player->actor.colChkInfo.damageTable = &DummyPlayerDamageTable;

    gSaveContext.linkAge = originalAge;

    bool isGlobalRoom = (std::string("soh-global") == CVarGetString(CVAR_REMOTE_ANCHOR("RoomId"), ""));

    if (!isGlobalRoom) {
        NameTag_RegisterForActorWithOptions(actor, client.name.c_str(), {});
    }
}

void Math_Vec3s_Copy(Vec3s* dest, Vec3s* src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
}

// Update the actor with new data from the client
void DummyPlayer_Update(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;

    uint32_t clientId = Anchor::Instance->GetDummyPlayerClientId(actor);

    if (!Anchor::Instance->clients.contains(clientId)) {
        Actor_Kill(actor);
        return;
    }

    AnchorClient& client = Anchor::Instance->clients[clientId];

    if (client.sceneNum != gPlayState->sceneNum || !client.online || !client.isSaveLoaded) {
        actor->world.pos.x = -9999.0f;
        actor->world.pos.y = -9999.0f;
        actor->world.pos.z = -9999.0f;
        actor->shape.shadowAlpha = 0;
        // FD (2026-07-13): keep the transform-anim edge-detector current even while this remote is out of our scene,
        // so walking into our scene already mid-transform doesn't replay the transform-start sfx (anim stayed non-0).
        client.fdTransformAnimPrev = client.fdTransformAnim;
        client.fdTransformCurFramePrev = client.fdTransformCurFrame; // FD (2026-07-14): same for the sfx-beat detector
        return;
    }

    actor->shape.shadowAlpha = 255;

    // FD (2026-07-14): a remote playing as Fierce Deity must draw at FD's 0.015 scale, exactly like the local
    // player's per-frame size block (z_player.c ~13775/13813). The dummy never runs Player_UpdateCommon, so
    // without this an FD remote keeps the 0.01 adult/child init scale and the (smaller-per-unit) FD model renders
    // at Young-Link height. Only override for DEITY; adult/child dummies keep their normal init scale.
    if (client.linkAge == LINK_AGE_DEITY) {
        actor->scale.x = actor->scale.y = actor->scale.z = 0.015f;
    }

    // FD (2026-07-14): a DEITY remote's body limb DLs live inside OBJECT_LINK_BOY, but this dummy inherited the
    // shared ACTOR_PLAYER object slot -- which is OBJECT_LINK_CHILD when the LOCAL player is a child. Drawing the
    // FD skeleton with segment 6 bound to the child object yields "FD skeleton + young-Link skin" (the reported
    // bug -- and it's the REMOTE's dummy, not your own actor, wearing your child object). Make OBJECT_LINK_BOY
    // resident (spawn as soon as the transform anim starts so the DMA finishes by the apex) and, once loaded,
    // point this dummy's object slot at it so Actor_SetObjectDependency binds segment 6 to the boy object. If not
    // yet loaded we leave the slot as-is (brief child-skinned frame). Object_GetIndex avoids duplicate bank spawns.
    if (client.linkAge == LINK_AGE_DEITY || client.fdTransformAnim != 0) {
        s32 boyIdx = Object_GetIndex(&play->objectCtx, OBJECT_LINK_BOY);
        if (boyIdx < 0) {
            boyIdx = Object_Spawn(&play->objectCtx, OBJECT_LINK_BOY);
        }
        if (client.linkAge == LINK_AGE_DEITY && boyIdx >= 0 && Object_IsLoaded(&play->objectCtx, boyIdx)) {
            actor->objBankIndex = boyIdx;
        }
    }

    Math_Vec3s_Copy(&player->upperLimbRot, &client.upperLimbRot);
    Math_Vec3s_Copy(&actor->shape.rot, &client.posRot.rot);
    Math_Vec3f_Copy(&actor->world.pos, &client.posRot.pos);
    player->skelAnime.jointTable = client.jointTable;
    player->skelAnime.movementFlags = client.movementFlags;
    Math_Vec3s_Copy(&player->skelAnime.prevTransl, &client.prevTransl);
    player->currentBoots = client.currentBoots;
    player->currentShield = client.currentShield;
    player->currentTunic = client.currentTunic;
    player->stateFlags1 = client.stateFlags1;
    player->stateFlags2 = client.stateFlags2;
    player->itemAction = client.itemAction;
    player->heldItemAction = client.heldItemAction;
    player->invincibilityTimer = client.invincibilityTimer;
    player->unk_862 =
        (client.unk_862 > (s16)GID_MAXIMUM) ? (s16)GID_STONE_OF_AGONY : client.unk_862; // prevent OOB, show SoA if OOB
    player->unk_85C = client.unk_85C;
    player->av1.actionVar1 = client.actionVar1;

    // FD (2026-07-13): apply the networked Fierce Deity transform-cutscene state so this remote player visibly dons
    // the mask (held -> on-face -> scream) and morphs. Player_PostLimbDrawGameplay reads these when drawing. The
    // anim id + curFrame reproduce the mask-gate animation identity (the pose itself already comes from jointTable).
    // ★GATED on the remote actually being mid-transform (anim id non-zero): previously this ran EVERY frame for EVERY
    // remote and overwrote the whole dummy's stateFlags3 + skelAnime.curFrame, mutating non-FD (adult/child) remotes'
    // draw state that vanilla Anchor never touched. Now non-FD remotes keep their normal draw state; we only OR-in the
    // on-face mask bit + cutscene params during the transform, and clear that one bit otherwise so it can't stick.
    if (client.fdTransformAnim != 0) {
        player->stateFlags3 |= (client.fdStateFlags3 & PLAYER_STATE3_TRANSFORMATION_MASK);
        player->transformTargetForm = client.fdTransformTargetForm;
        player->transformPreviousForm = client.fdTransformPrevForm;
        player->transformMatrixModifiers[2] = client.fdTransformMod2;
        player->transformMatrixModifiers[3] = client.fdTransformMod3;
        player->transformEventTimer2 = client.fdTransformTimer2;
        Player_SetFdTransformAnimById(player, client.fdTransformAnim, client.fdTransformCurFrame);

        // AUDIO (FD 2026-07-14): reproduce the transforming player's mask-cutscene sfx POSITIONALLY at the remote,
        // sequenced by the synced anim frame -- so nearby players hear the beats in the right ORDER/TIMING/PLACE
        // instead of one scream fired at cutscene start (the old bug). The faithful custom-WAV cues (Adult/Child
        // scream, Mask_Attach, face-change) are 2D-only on the transforming client (FdAudio_PlayOneShot) and can't
        // be positioned, so remotes get the RE's positional N64 substitutes at the SAME frames the local timeline
        // (z_player.c Player_UpdateTransformationAnim) uses:
        //   put-on  (cl_setmask, id 1): f4 NA_SE_PL_CHANGE_ARMS click, f20 NA_SE_PL_FREEZE_S mask-snap,
        //                               f30 NA_SE_VO_LI_MAGIC_ATTACK[_KID] scream (voice keyed by the pre-form)
        //   revert  (cl_maskoff id 3 / pz_maskoffstart id 4): f12 NA_SE_PL_PUT_OUT_ITEM take-off,
        //                               f15 NA_SE_PL_CHANGE_ARMS face-change (== FD_SFX_FACE_CHANGE)
        // A beat fires once when the synced curFrame CROSSES its threshold. On a fresh anim (id changed) prev is
        // reset to -1 so a beat below the first synced frame still lands.
        {
            f32 fPrev = (client.fdTransformAnimPrev == client.fdTransformAnim) ? client.fdTransformCurFramePrev
                                                                              : -1.0f;
            f32 fCur = client.fdTransformCurFrame;
#define FD_XFORM_BEAT(T) ((fPrev < (T)) && (fCur >= (T)))
            if (client.fdTransformAnim == 1) { // cl_setmask (put-on the FD mask)
                if (FD_XFORM_BEAT(4.0f)) {
                    Audio_PlayActorSound2(&player->actor, NA_SE_PL_CHANGE_ARMS);
                }
                if (FD_XFORM_BEAT(20.0f)) {
                    Audio_PlayActorSound2(&player->actor, NA_SE_PL_FREEZE_S);
                }
                if (FD_XFORM_BEAT(30.0f)) {
                    Audio_PlayActorSound2(&player->actor, (client.fdTransformPrevForm == LINK_AGE_CHILD)
                                                              ? NA_SE_VO_LI_MAGIC_ATTACK_KID
                                                              : NA_SE_VO_LI_MAGIC_ATTACK);
                }
            } else if (client.fdTransformAnim == 3 || client.fdTransformAnim == 4) { // revert (take mask off)
                if (FD_XFORM_BEAT(12.0f)) {
                    Audio_PlayActorSound2(&player->actor, NA_SE_PL_PUT_OUT_ITEM);
                }
                if (FD_XFORM_BEAT(15.0f)) {
                    Audio_PlayActorSound2(&player->actor, NA_SE_PL_CHANGE_ARMS);
                }
            }
#undef FD_XFORM_BEAT
        }
    } else {
        player->stateFlags3 &= ~PLAYER_STATE3_TRANSFORMATION_MASK;
    }
    client.fdTransformAnimPrev = client.fdTransformAnim;
    client.fdTransformCurFramePrev = client.fdTransformCurFrame;

    // Apply animation movement (Copied from Player_ApplyAnimMovementScaledByAge)
    Vec3f diff;
    SkelAnime_UpdateTranslation(&player->skelAnime, &diff, player->actor.shape.rot.y);

    if (player->skelAnime.movementFlags & 1) {
        if (!LINK_IS_ADULT) {
            diff.x *= 0.64f;
            diff.z *= 0.64f;
        }

        player->actor.world.pos.x += diff.x * player->actor.scale.x;
        player->actor.world.pos.z += diff.z * player->actor.scale.z;
    }

    if (player->skelAnime.movementFlags & 2) {
        if (!(player->skelAnime.movementFlags & 4)) {
            diff.y *= player->ageProperties->unk_08;
        }

        player->actor.world.pos.y += diff.y * player->actor.scale.y;
    }

    if (player->modelGroup != client.modelGroup) {
        // Hack to account for usage of gSaveContext
        s32 originalAge = gSaveContext.linkAge;
        gSaveContext.linkAge = client.linkAge;
        u8 originalButtonItem0 = gSaveContext.equips.buttonItems[0];
        gSaveContext.equips.buttonItems[0] = client.buttonItem0;
        Player_SetModelGroup(player, client.modelGroup);
        gSaveContext.linkAge = originalAge;
        gSaveContext.equips.buttonItems[0] = originalButtonItem0;
    }

    if (Anchor::Instance->roomState.pvpMode == 0 ||
        (Anchor::Instance->roomState.pvpMode == 1 &&
         client.teamId == CVarGetString(CVAR_REMOTE_ANCHOR("TeamId"), "default"))) {
        actor->flags |= ACTOR_FLAG_LOCK_ON_DISABLED;
        return;
    }

    actor->flags &= ~ACTOR_FLAG_LOCK_ON_DISABLED;

    if (player->cylinder.base.acFlags & AC_HIT && player->invincibilityTimer == 0) {
        Anchor::Instance->SendPacket_DamagePlayer(client.clientId, player->actor.colChkInfo.damageEffect,
                                                  player->actor.colChkInfo.damage);
        if (player->actor.colChkInfo.damageEffect == DUMMY_PLAYER_HIT_RESPONSE_STUN) {
            Actor_SetColorFilter(&player->actor, 0, 0xFF, 0, 24);
        } else {
            player->invincibilityTimer = 20;
        }
    }

    Collider_UpdateCylinder(&player->actor, &player->cylinder);

    if (!(player->stateFlags2 & PLAYER_STATE2_FROZEN)) {
        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_HANGING_OFF_LEDGE |
                                     PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_ON_HORSE))) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &player->cylinder.base);
        }

        if (!(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_DAMAGED)) &&
            (player->invincibilityTimer <= 0)) {
            CollisionCheck_SetAC(play, &play->colChkCtx, &player->cylinder.base);

            if (player->invincibilityTimer < 0) {
                CollisionCheck_SetAT(play, &play->colChkCtx, &player->cylinder.base);
            }
        }
    }

    if (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE)) {
        player->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        player->actor.colChkInfo.mass = 50;
    }

    Collider_ResetCylinderAC(play, &player->cylinder.base);
}

void DummyPlayer_Draw(Actor* actor, PlayState* play) {
    Player* player = (Player*)actor;

    uint32_t clientId = Anchor::Instance->GetDummyPlayerClientId(actor);

    if (!Anchor::Instance->clients.contains(clientId)) {
        Actor_Kill(actor);
        return;
    }

    AnchorClient& client = Anchor::Instance->clients[clientId];

    if (client.sceneNum != gPlayState->sceneNum || !client.online || !client.isSaveLoaded) {
        return;
    }

    // Hack to account for usage of gSaveContext in Player_Draw
    s32 originalAge = gSaveContext.linkAge;
    gSaveContext.linkAge = client.linkAge;
    u8 originalButtonItem0 = gSaveContext.equips.buttonItems[0];
    gSaveContext.equips.buttonItems[0] = client.buttonItem0;

    // FD (2026-07-14): when this remote is holding up the FD-mask get-item, its gid (Goron-mask flow-gate id) would
    // make Player_DrawGetItem draw the raw GORON mask -- the real FD-mask model comes from a local-only custom draw
    // func on the get-item entry, which the dummy never has. Point the dummy's get-item draw func at it so the held
    // model is the Fierce Deity mask (the func draws fixed resource-path DLs -- no giObjectSegment dependency).
    // Cleared to NULL otherwise so any OTHER get-item the dummy replicates still draws normally by its gid.
    player->getItemEntry.drawFunc = client.fdMaskGi ? FierceDeity_DrawGiMask : NULL;

    Player_Draw((Actor*)player, play);
    gSaveContext.linkAge = originalAge;
    gSaveContext.equips.buttonItems[0] = originalButtonItem0;
}

void DummyPlayer_Destroy(Actor* actor, PlayState* play) {
    // DummyPlayer Actors are initially spawned as ACTOR_PLAYER, but change their
    // ID shortly afterwards to ACTOR_EN_OE2. This would cause ACTOR_PLAYER's
    // ActorDB Entry's `numLoaded` to leak, which is mostly harmless but hits debug
    // asserts. Set the id back to ACTOR_PLAYER so that `numLoaded` will be decremented
    // correctly.
    actor->id = ACTOR_PLAYER;
}
