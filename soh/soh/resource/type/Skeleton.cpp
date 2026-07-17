#include <ship/resource/ResourceManager.h>
#include "Skeleton.h"
#include "soh/OTRGlobals.h"
#include "libultraship/libultraship.h"
#include <soh_assets.h>
#include <objects/object_link_child/object_link_child.h>
#include <objects/object_link_boy/object_link_boy.h>

extern "C" {
#include "variables.h"
#include "z64.h"
#include "macros.h"
#include "z64player.h"
extern PlayState* gPlayState;
}

extern "C" SaveContext gSaveContext;
extern "C" u16 gEquipMasks[4];
extern "C" u8 gEquipShifts[4];

namespace SOH {
SkeletonData* Skeleton::GetPointer() {
    return &skeletonData;
}

size_t Skeleton::GetPointerSize() {
    switch (type) {
        case SkeletonType::Normal:
            return sizeof(skeletonData.skeletonHeader);
        case SkeletonType::Flex:
            return sizeof(skeletonData.flexSkeletonHeader);
        case SkeletonType::Curve:
            return sizeof(skeletonData.skelCurveLimbList);
        default:
            return 0;
    }
}

std::vector<SkeletonPatchInfo> SkeletonPatcher::skeletons;

bool SkeletonPatcher::IsLinkSkeletonPath(const std::string& path) {
    return (sOtr + path == std::string(gLinkAdultSkel)) || (sOtr + path == std::string(gLinkChildSkel));
}

bool SkeletonPatcher::IsLocalPlayerSkelAnime(SkelAnime* skelAnime) {
    if (gPlayState == nullptr) {
        return false;
    }

    Player* player = GET_PLAYER(gPlayState);

    if (player == nullptr) {
        return false;
    }

    PauseContext* pauseCtx = &gPlayState->pauseCtx;

    return (skelAnime == &player->skelAnime) || (skelAnime == &player->upperSkelAnime) ||
           (skelAnime == &pauseCtx->playerSkelAnime);
}

void SkeletonPatcher::RegisterSkeleton(std::string& path, SkelAnime* skelAnime) {
    SkeletonPatchInfo info;

    info.skelAnime = skelAnime;
    info.isLocalPlayer = false;

    if (path.starts_with(sOtr)) {
        path = path.substr(sOtr.length());
    }

    // Determine if we're using an alternate skeleton
    if (path.starts_with(Ship::IResource::gAltAssetPrefix)) {
        info.vanillaSkeletonPath = path.substr(Ship::IResource::gAltAssetPrefix.length(),
                                               path.size() - Ship::IResource::gAltAssetPrefix.length());
    } else {
        info.vanillaSkeletonPath = path;
    }

    if (IsLinkSkeletonPath(info.vanillaSkeletonPath)) {
        info.isLocalPlayer = IsLocalPlayerSkelAnime(skelAnime);

        // Skip registering skeletons that do not belong to the local player (e.g. Anchor dummy actors)
        if (!info.isLocalPlayer) {
            return;
        }
    }

    skeletons.push_back(info);
}

void SkeletonPatcher::UnregisterSkeleton(SkelAnime* skelAnime) {

    // TODO: Should probably just use a dictionary here...
    for (size_t i = 0; i < skeletons.size(); i++) {
        auto skel = skeletons[i];

        if (skel.skelAnime == skelAnime) {
            skeletons.erase(skeletons.begin() + i);
            break;
        }
    }
}
void SkeletonPatcher::ClearSkeletons() {
    skeletons.clear();
}

void SkeletonPatcher::UpdateSkeletons() {
    auto resourceMgr = Ship::Context::GetRawInstance()->GetResourceManager();
    bool isAlt = resourceMgr->IsAltAssetsEnabled();
    // FD (2026-07-15): while the local player is Fierce Deity, his skelAnime is registered under the stale base-age
    // path; reloading that here on the alt-assets toggle (OTRGlobals.cpp) would repoint the FD body back to base-age
    // Link (the "giant Young Link + FD skeleton" corruption). FD assets live at base paths (no alt/ variant), so the
    // FD skeleton never needs an alt swap -- skip the local player's skels while transformed. Other (non-player)
    // skeletons still swap normally. See the matching guard in UpdateCustomSkeletons.
    bool isDeity = (gPlayState != nullptr) && (gSaveContext.linkAge == LINK_AGE_DEITY);
    for (auto skel : skeletons) {
        if (skel.isLocalPlayer && isDeity) {
            continue;
        }
        Skeleton* newSkel =
            (Skeleton*)resourceMgr
                ->LoadResource((isAlt ? Ship::IResource::gAltAssetPrefix : "") + skel.vanillaSkeletonPath, true)
                .get();

        if (newSkel != nullptr) {
            skel.skelAnime->skeleton = newSkel->skeletonData.skeletonHeader.segment;
            uintptr_t skelPtr = (uintptr_t)newSkel->GetPointer();
            memcpy(&skel.skelAnime->skeletonHeader, &skelPtr,
                   sizeof(uintptr_t)); // Dumb thing that needs to be done because cast is not cooperating
        }
    }
}

void SkeletonPatcher::UpdateCustomSkeletons() {
    // FD (2026-07-15): while the local player is Fierce Deity his skelAnime is still REGISTERED under the stale
    // base-age path (gLinkAdultSkel/gLinkChildSkel from Player_InitCommon), even though its skeleton pointer already
    // points at the FD skeleton. Tunic-patching it here -- notably on the OnAssetAltChange hook when the player Tabs
    // the mods/alt-assets toggle -- repoints the FD body back to young/adult Link, producing the reported "giant
    // Young Link with FD skeleton + broken face" corruption. FD is never tunic-patched (its path is excluded from
    // IsLinkSkeletonPath), so skip the whole pass while transformed; the FD model is a vanilla-of-this-fork asset
    // and must be left alone. Reverting FD->human re-registers the base age via Player_ChangeAge, restoring normal
    // tunic patching. (Same root cause as the pause-menu FD BUG 5 workaround in z_player_lib.c.)
    if (gPlayState != nullptr && gSaveContext.linkAge == LINK_AGE_DEITY) {
        return;
    }
    for (auto skel : skeletons) {
        if (!skel.isLocalPlayer) {
            continue;
        }

        UpdateTunicSkeletons(skel);
    }
}

void SkeletonPatcher::UpdateTunicSkeletons(SkeletonPatchInfo& skel) {
    std::string skeletonPath = "";

    // Check if this is one of Link's skeletons
    if (sOtr + skel.vanillaSkeletonPath == std::string(gLinkAdultSkel)) {
        // Check what Link's current tunic is
        switch (TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC))) {
            case PLAYER_TUNIC_KOKIRI:
                skeletonPath = std::string(gLinkAdultKokiriTunicSkel).substr(sOtr.length());
                break;
            case PLAYER_TUNIC_GORON:
                skeletonPath = std::string(gLinkAdultGoronTunicSkel).substr(sOtr.length());
                break;
            case PLAYER_TUNIC_ZORA:
                skeletonPath = std::string(gLinkAdultZoraTunicSkel).substr(sOtr.length());
                break;
            default:
                return;
        }

        UpdateCustomSkeletonFromPath(skeletonPath, skel);
    } else if (sOtr + skel.vanillaSkeletonPath == std::string(gLinkChildSkel)) {
        // Check what Link's current tunic is
        switch (TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC))) {
            case PLAYER_TUNIC_KOKIRI:
                skeletonPath = std::string(gLinkChildKokiriTunicSkel).substr(sOtr.length());
                break;
            case PLAYER_TUNIC_GORON:
                skeletonPath = std::string(gLinkChildGoronTunicSkel).substr(sOtr.length());
                break;
            case PLAYER_TUNIC_ZORA:
                skeletonPath = std::string(gLinkChildZoraTunicSkel).substr(sOtr.length());
                break;
            default:
                return;
        }

        UpdateCustomSkeletonFromPath(skeletonPath, skel);
    }
}

void SkeletonPatcher::UpdateCustomSkeletonFromPath(const std::string& skeletonPath, SkeletonPatchInfo& skel) {
    Skeleton* newSkel = nullptr;
    Skeleton* altSkel = nullptr;
    auto resourceMgr = Ship::Context::GetRawInstance()->GetResourceManager();
    bool isAlt = resourceMgr->IsAltAssetsEnabled();

    // If alt assets are on, look for alt tagged skeletons
    if (isAlt) {
        altSkel = (Skeleton*)Ship::Context::GetRawInstance()
                      ->GetResourceManager()
                      ->LoadResource(Ship::IResource::gAltAssetPrefix + skeletonPath, true)
                      .get();

        // Override non-alt skeleton if necessary
        if (altSkel != nullptr) {
            newSkel = altSkel;
        }
    }

    // Load new skeleton based on the custom model if it exists
    if (altSkel == nullptr) {
        newSkel =
            (Skeleton*)Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(skeletonPath, true).get();
    }

    // Change back to the original skeleton if no skeleton's were found
    if (newSkel == nullptr && skeletonPath != skel.vanillaSkeletonPath) {
        UpdateCustomSkeletonFromPath(skel.vanillaSkeletonPath, skel);
        return;
    }

    if (newSkel != nullptr) {
        skel.skelAnime->skeleton = newSkel->skeletonData.skeletonHeader.segment;
        uintptr_t skelPtr = (uintptr_t)newSkel->GetPointer();
        memcpy(&skel.skelAnime->skeletonHeader, &skelPtr, sizeof(uintptr_t));
    }
}
} // namespace SOH
