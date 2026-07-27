#include "global.h"
#include <string.h> // FD (2026-07-12): strcmp for Player_AnimIsByName (mask-cutscene anim detection by path content)
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "objects/object_link_boy/object_link_boy.h"
#include "objects/object_link_child/object_link_child.h"
#include "objects/object_link_deity/object_link_deity.h" // Fierce Deity (aegiker RE->SoH port 2026-07-11)
#include "overlays/actors/ovl_Demo_Effect/z_demo_effect.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/randomizer/draw.h"
#include "soh/ResourceManagerHelpers.h"

#include <stdlib.h>

typedef struct {
    /* 0x00 */ u8 flag;
    /* 0x02 */ u16 textId;
} TextTriggerEntry; // size = 0x04

typedef struct {
    /* 0x00 */ void* dList;
    /* 0x04 */ Vec3f pos;
} BowStringData; // size = 0x10

// Index by gSaveContext.linkAge (LINK_AGE_ADULT/CHILD/DEITY). DEITY resolves to the FD skeleton
// resource in fd.o2r (objects/object_link_boy/gLinkFierceDeitySkel). aegiker RE->SoH port 2026-07-11.
FlexSkeletonHeader* gPlayerSkelHeaders[] = { &gLinkAdultSkel, &gLinkChildSkel, &gLinkFierceDeitySkel };

s16 sBootData[PLAYER_BOOTS_MAX][17] = {
    { 200, 1000, 300, 700, 550, 270, 600, 350, 800, 600, -100, 600, 590, 750, 125, 200, 130 },
    { 200, 1000, 300, 700, 550, 270, 1000, 0, 800, 300, -160, 600, 590, 750, 125, 200, 130 },
    { 200, 1000, 300, 700, 550, 270, 600, 600, 800, 550, -100, 600, 540, 270, 25, 0, 130 },
    { 200, 1000, 300, 700, 380, 400, 0, 300, 800, 500, -100, 600, 590, 750, 125, 200, 130 },
    { 80, 800, 150, 700, 480, 270, 600, 50, 800, 550, -40, 400, 540, 270, 25, 0, 80 },
    { 200, 1000, 300, 800, 500, 400, 800, 400, 800, 550, -100, 600, 540, 750, 125, 400, 200 },
};

// Used to map action params to model groups
u8 sActionModelGroups[] = {
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_NONE
    PLAYER_MODELGROUP_SWORD,            // PLAYER_IA_SWORD_CS
    PLAYER_MODELGROUP_10,               // PLAYER_IA_FISHING_POLE
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_IA_SWORD_MASTER
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_IA_SWORD_KOKIRI
    PLAYER_MODELGROUP_BGS,              // PLAYER_IA_SWORD_BIGGORON
    PLAYER_MODELGROUP_10,               // PLAYER_IA_DEKU_STICK
    PLAYER_MODELGROUP_HAMMER,           // PLAYER_IA_HAMMER
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_FIRE
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_ICE
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_LIGHT
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0C
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0D
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0E
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_SLINGSHOT
    PLAYER_MODELGROUP_HOOKSHOT,         // PLAYER_IA_HOOKSHOT
    PLAYER_MODELGROUP_HOOKSHOT,         // PLAYER_IA_LONGSHOT
    PLAYER_MODELGROUP_EXPLOSIVES,       // PLAYER_IA_BOMB
    PLAYER_MODELGROUP_EXPLOSIVES,       // PLAYER_IA_BOMBCHU
    PLAYER_MODELGROUP_BOOMERANG,        // PLAYER_IA_BOOMERANG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_SPELL_15
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_SPELL_16
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_MAGIC_SPELL_17
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_FARORES_WIND
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_NAYRUS_LOVE
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_DINS_FIRE
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_DEKU_NUT
    PLAYER_MODELGROUP_OCARINA,          // PLAYER_IA_OCARINA_FAIRY
    PLAYER_MODELGROUP_OOT,              // PLAYER_IA_OCARINA_OF_TIME
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FISH
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FIRE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_BUG
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_BIG_POE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_RUTOS_LETTER
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_RED
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_BLUE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_GREEN
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_MILK_FULL
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_MILK_HALF
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FAIRY
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ZELDAS_LETTER
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_WEIRD_EGG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_CHICKEN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_BEAN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POCKET_EGG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POCKET_CUCCO
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_COJIRO
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ODD_MUSHROOM
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ODD_POTION
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POACHERS_SAW
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_BROKEN_GORONS_SWORD
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_PRESCRIPTION
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_FROG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_EYEDROPS
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_CLAIM_CHECK
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_KEATON
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_SKULL
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_SPOOKY
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_BUNNY_HOOD
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_GORON
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_ZORA
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_GERUDO
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_TRUTH
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_LENS_OF_TRUTH
};

TextTriggerEntry sTextTriggers[] = {
    { 1, 0x3040 },
    { 2, 0x401D },
    { 0, 0x0000 },
    { 2, 0x401D },
};

// Used to map model groups to model types for [animation, left hand, right hand, sheath, waist]
u8 gPlayerModelTypes[PLAYER_MODELGROUP_MAX][PLAYER_MODELGROUPENTRY_MAX] = {
    /* PLAYER_MODELGROUP_0 */
    { PLAYER_ANIMTYPE_2, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_SHIELD, PLAYER_MODELTYPE_SHEATH_16,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_CHILD_HYLIAN_SHIELD */
    { PLAYER_ANIMTYPE_1, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_SHEATH_19,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_SWORD_AND_SHIELD */
    { PLAYER_ANIMTYPE_1, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_SHIELD, PLAYER_MODELTYPE_SHEATH_17,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_DEFAULT */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_4 */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BGS */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_BGS, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_SHEATH_19,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOW_SLINGSHOT */
    { PLAYER_ANIMTYPE_4, PLAYER_MODELTYPE_LH_CLOSED, PLAYER_MODELTYPE_RH_BOW_SLINGSHOT, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_EXPLOSIVES */
    { PLAYER_ANIMTYPE_5, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOOMERANG */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_BOOMERANG, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_HOOKSHOT */
    { PLAYER_ANIMTYPE_4, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_HOOKSHOT, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_10 */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_CLOSED, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_HAMMER */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_HAMMER, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_OCARINA */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OCARINA, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_OOT */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OOT, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOTTLE */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_BOTTLE, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_18,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_SWORD */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_SHEATH_19,
      PLAYER_MODELTYPE_WAIST },
};

Gfx* sPlayerRightHandShieldDLs[PLAYER_SHIELD_MAX * (NUM_DL_FORMS * 2)] = { // FD (2026-07-11): widened 4->6 stride
    // PLAYER_SHIELD_NONE
    gLinkAdultRightHandClosedNearDL,
    gLinkChildRightHandClosedNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandClosedFarDL,
    gLinkChildRightHandClosedFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    // PLAYER_SHIELD_DEKU
    gLinkAdultRightHandClosedNearDL,
    gLinkChildRightFistAndDekuShieldNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandClosedFarDL,
    gLinkChildRightFistAndDekuShieldFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    // PLAYER_SHIELD_HYLIAN
    gLinkAdultRightHandHoldingHylianShieldNearDL,
    gLinkChildRightHandClosedNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandHoldingHylianShieldFarDL,
    gLinkChildRightHandClosedFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    // PLAYER_SHIELD_MIRROR
    gLinkAdultRightHandHoldingMirrorShieldNearDL,
    gLinkChildRightHandClosedNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandHoldingMirrorShieldFarDL,
    gLinkChildRightHandClosedFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
};

Gfx* sSheathWithSwordDLs[(PLAYER_SHIELD_MAX + 2) * (NUM_DL_FORMS * 2)] = { // FD (2026-07-11): widened 4->6 stride
    // PLAYER_SHIELD_NONE
    gLinkAdultMasterSwordAndSheathNearDL,
    gLinkChildSwordAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD has no sheath
    gLinkAdultMasterSwordAndSheathFarDL,
    gLinkChildSwordAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_DEKU
    gLinkAdultMasterSwordAndSheathNearDL,
    gLinkChildDekuShieldSwordAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultMasterSwordAndSheathFarDL,
    gLinkChildDekuShieldSwordAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_HYLIAN
    gLinkAdultHylianShieldSwordAndSheathNearDL,
    gLinkChildHylianShieldSwordAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultHylianShieldSwordAndSheathFarDL,
    gLinkChildHylianShieldSwordAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_MIRROR
    gLinkAdultMirrorShieldSwordAndSheathNearDL,
    gLinkChildSwordAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultMirrorShieldSwordAndSheathFarDL,
    gLinkChildSwordAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_NONE (child, no sword)
    NULL,
    NULL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    NULL,
    NULL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_DEKU (child, no sword)
    NULL,
    gLinkChildDekuShieldWithMatrixDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    NULL,
    gLinkChildDekuShieldWithMatrixDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sSheathWithoutSwordDLs[(PLAYER_SHIELD_MAX + 2) * (NUM_DL_FORMS * 2)] = { // FD (2026-07-11): widened 4->6 stride
    // PLAYER_SHIELD_NONE
    gLinkAdultSheathNearDL,
    gLinkChildSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD has no sheath
    gLinkAdultSheathFarDL,
    gLinkChildSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_DEKU
    gLinkAdultSheathNearDL,
    gLinkChildDekuShieldAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultSheathFarDL,
    gLinkChildDekuShieldAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_HYLIAN
    gLinkAdultHylianShieldAndSheathNearDL,
    gLinkChildHylianShieldAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultHylianShieldAndSheathFarDL,
    gLinkChildHylianShieldAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_MIRROR
    gLinkAdultMirrorShieldAndSheathNearDL,
    gLinkChildSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultMirrorShieldAndSheathFarDL,
    gLinkChildSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_NONE (child, no sword)
    NULL,
    NULL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    NULL,
    NULL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // PLAYER_SHIELD_DEKU (child, no sword)
    gLinkAdultSheathNearDL,
    gLinkChildDekuShieldWithMatrixDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultSheathNearDL,
    gLinkChildDekuShieldWithMatrixDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* gPlayerLeftHandBgsDLs[] = { // FD (2026-07-11): widened 4->6 stride + FD-sword sub-block
    // Biggoron Sword
    gLinkAdultLeftHandHoldingBgsNearDL,
    gLinkChildLeftHandHoldingMasterSwordDL,
    gLinkFierceDeityLeftHandHoldingSwordDL, // FD (2026-07-11): blade baked in, no separate sword DL
    gLinkAdultLeftHandHoldingBgsFarDL,
    gLinkChildLeftHandHoldingMasterSwordDL,
    gLinkFierceDeityLeftHandHoldingSwordDL, // FD (2026-07-11)
    // Broken Giant's Knife
    gLinkAdultHandHoldingBrokenGiantsKnifeDL,
    gLinkChildLeftHandHoldingMasterSwordDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD sword never breaks
    gLinkAdultHandHoldingBrokenGiantsKnifeFarDL,
    gLinkChildLeftHandHoldingMasterSwordDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    // Fierce Deity sword (selected by 3g offset when holding the deity sword). FD-form-only: the
    // adult/child "FD-sword-in-a-human-hand" DLs are absent from object_link_deity.h, so they are NULL;
    // only the deity slot is ever read at linkAge==LINK_AGE_DEITY.
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): gLinkAdultFierceDeityLeftHandHoldingSwordDL absent
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): gLinkChildFierceDeityLeftHandHoldingSwordDL absent
    gLinkFierceDeityLeftHandHoldingSwordDL, // FD (2026-07-11)
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkFierceDeityLeftHandHoldingSwordDL, // FD (2026-07-11)
};

// FD (2026-07-11): all DL-group arrays below widened adult,child -> adult,child,DEITY (near then far),
// stride NUM_DL_FORMS (3). FD-form entries per FD_PORT_SPEC.md Group 3b.
Gfx* gPlayerLeftHandOpenDLs[] = {
    gLinkAdultLeftHandNearDL,
    gLinkChildLeftHandNearDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11)
    gLinkAdultLeftHandFarDL,
    gLinkChildLeftHandFarDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11)
};

Gfx* gPlayerLeftHandClosedDLs[] = {
    gLinkAdultLeftHandClosedNearDL,
    gLinkChildLeftFistNearDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11)
    gLinkAdultLeftHandClosedFarDL,
    gLinkChildLeftFistFarDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11)
};

Gfx* sPlayerLeftHandSwordDLs2[] = {
    gLinkAdultLeftHandHoldingMasterSwordNearDL,
    gLinkChildLeftFistAndKokiriSwordNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD sword baked into holding-sword DL, no separate left-hand-sword DL
    gLinkAdultLeftHandHoldingMasterSwordFarDL,
    gLinkChildLeftFistAndKokiriSwordFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerLeftHandSwordDLs[] = {
    gLinkAdultLeftHandHoldingMasterSwordNearDL,
    gLinkChildLeftFistAndKokiriSwordNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultLeftHandHoldingMasterSwordFarDL,
    gLinkChildLeftFistAndKokiriSwordFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandOpenDLs[] = {
    gLinkAdultRightHandNearDL,
    gLinkChildRightHandNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandFarDL,
    gLinkChildRightHandFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandClosedDLs[] = {
    gLinkAdultRightHandClosedNearDL,
    gLinkChildRightHandClosedNearDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
    gLinkAdultRightHandClosedFarDL,
    gLinkChildRightHandClosedFarDL,
    gLinkFierceDeityRightHandDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandBowSlingshotDLs[] = {
    gLinkAdultRightHandHoldingBowNearDL,
    gLinkChildRightHandHoldingSlingshotNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD has no bow/slingshot
    gLinkAdultRightHandHoldingBowFarDL,
    gLinkChildRightHandHoldingSlingshotFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sSwordAndSheathDLs[] = {
    gLinkAdultMasterSwordAndSheathNearDL,
    gLinkChildSwordAndSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD has no sheath
    gLinkAdultMasterSwordAndSheathFarDL,
    gLinkChildSwordAndSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sSheathDLs[] = {
    gLinkAdultSheathNearDL,
    gLinkChildSheathNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11): FD has no sheath
    gLinkAdultSheathFarDL,
    gLinkChildSheathFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerWaistDLs[] = {
    gLinkAdultWaistNearDL,
    gLinkChildWaistNearDL,
    gLinkFierceDeityWaistDL, // FD (2026-07-11)
    gLinkAdultWaistFarDL,
    gLinkChildWaistFarDL,
    gLinkFierceDeityWaistDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandBowSlingshotDLs2[] = {
    gLinkAdultRightHandHoldingBowNearDL,
    gLinkChildRightHandHoldingSlingshotNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultRightHandHoldingBowFarDL,
    gLinkChildRightHandHoldingSlingshotFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandOcarinaDLs[] = {
    gLinkAdultRightHandHoldingOotNearDL,
    gLinkChildRightHandHoldingFairyOcarinaNearDL,
    // FD (2026-07-13) SELF-CONTAINED ocarina: the deity slots draw FD's OWN right hand (gLinkFierceDeityRightHandDL,
    // part of the always-loaded FD model, so it has FD's real pale hand texture -- the earlier patched-adult-hand DL
    // rendered adult/peachy skin because it kept adult hand tex/TLUT); the correct ocarina mesh -- Fairy OR OoT,
    // extracted from BASE geometry into fd.o2r -- is drawn on top in Player_PostLimbDrawGameplay based on which
    // ocarina is equipped. So FD holds the right ocarina with his own hand out of the gate.
    gLinkFierceDeityRightHandDL, // [2] DEITY near -- FD's own hand (correct texture)
    gLinkAdultRightHandHoldingOotFarDL,        // [3] adult far
    gLinkChildRightHandHoldingFairyOcarinaFarDL, // [4] child far
    gLinkFierceDeityRightHandDL,  // [5] DEITY far -- FD's own hand (correct texture)
};

Gfx* sPlayerRightHandOotDLs[] = {
    gLinkAdultRightHandHoldingOotNearDL,
    gLinkChildRightHandAndOotNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultRightHandHoldingOotFarDL,
    gLinkChildRightHandHoldingOOTFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerRightHandHookshotDLs[] = {
    gLinkAdultRightHandHoldingHookshotNearDL,
    gLinkChildRightHandNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultRightHandHoldingHookshotNearDL, // The 'far' display list exists but is not used
    gLinkChildRightHandFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerLeftHandHammerDLs[] = {
    gLinkAdultLeftHandHoldingHammerNearDL,
    gLinkChildLeftHandNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultLeftHandHoldingHammerFarDL,
    gLinkChildLeftHandFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* gPlayerLeftHandBoomerangDLs[] = {
    gLinkAdultLeftHandNearDL,
    gLinkChildLeftFistAndBoomerangNearDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
    gLinkAdultLeftHandFarDL,
    gLinkChildLeftFistAndBoomerangFarDL,
    gLinkFierceDeityEmptyDL, // FD (2026-07-11)
};

Gfx* sPlayerLeftHandBottleDLs[] = {
    gLinkAdultLeftHandOutNearDL,
    gLinkChildLeftHandUpNearDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11): FD holds bottle with plain left hand (hold-bottle DL unused)
    gLinkAdultLeftHandOutNearDL,
    gLinkChildLeftHandUpNearDL,
    gLinkFierceDeityLeftHandDL, // FD (2026-07-11)
};

// FD (2026-07-11): first-person limb arrays are indexed directly by gSaveContext.linkAge (per-form),
// so they need one DEITY entry (index 2). FD is never in first-person weapon-aim in the FD-only port;
// DEITY = copy of adult per FD_PORT_SPEC.md 3c (SOURCE ships NULL here -- see report note).
Gfx* sFirstPersonLeftForearmDLs[] = {
    gLinkAdultRightArmOutNearDL,
    NULL,
    gLinkAdultRightArmOutNearDL, // FD (2026-07-11): DEITY = adult
};

Gfx* sFirstPersonLeftHandDLs[] = {
    gLinkAdultRightHandOutNearDL,
    NULL,
    gLinkAdultRightHandOutNearDL, // FD (2026-07-11): DEITY = adult
};

Gfx* sFirstPersonRightShoulderDLs[] = {
    gLinkAdultRightShoulderNearDL,
    gLinkChildRightShoulderNearDL,
    gLinkAdultRightShoulderNearDL, // FD (2026-07-11): DEITY = adult
};

Gfx* sFirstPersonForearmDLs[] = {
    gLinkAdultLeftArmOutNearDL,
    NULL,
    gLinkAdultLeftArmOutNearDL, // FD (2026-07-11): DEITY = adult
};

Gfx* sFirstPersonRightHandHoldingWeaponDLs[] = {
    gLinkAdultRightHandHoldingBowFirstPersonDL,
    gLinkChildRightArmStretchedSlingshotDL,
    gLinkAdultRightHandHoldingBowFirstPersonDL, // FD (2026-07-11): DEITY = adult
};

// Indexed by model types (left hand, right hand, sheath or waist)
Gfx** sPlayerDListGroups[PLAYER_MODELTYPE_MAX] = {
    gPlayerLeftHandOpenDLs,           // PLAYER_MODELTYPE_LH_OPEN
    gPlayerLeftHandClosedDLs,         // PLAYER_MODELTYPE_LH_CLOSED
    sPlayerLeftHandSwordDLs,          // PLAYER_MODELTYPE_LH_SWORD
    sPlayerLeftHandSwordDLs2,         // PLAYER_MODELTYPE_LH_SWORD_2
    gPlayerLeftHandBgsDLs,            // PLAYER_MODELTYPE_LH_BGS
    sPlayerLeftHandHammerDLs,         // PLAYER_MODELTYPE_LH_HAMMER
    gPlayerLeftHandBoomerangDLs,      // PLAYER_MODELTYPE_LH_BOOMERANG
    sPlayerLeftHandBottleDLs,         // PLAYER_MODELTYPE_LH_BOTTLE
    sPlayerRightHandOpenDLs,          // PLAYER_MODELTYPE_RH_OPEN
    sPlayerRightHandClosedDLs,        // PLAYER_MODELTYPE_RH_CLOSED
    sPlayerRightHandShieldDLs,        // PLAYER_MODELTYPE_RH_SHIELD
    sPlayerRightHandBowSlingshotDLs,  // PLAYER_MODELTYPE_RH_BOW_SLINGSHOT
    sPlayerRightHandBowSlingshotDLs2, // PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2
    sPlayerRightHandOcarinaDLs,       // PLAYER_MODELTYPE_RH_OCARINA
    sPlayerRightHandOotDLs,           // PLAYER_MODELTYPE_RH_OOT
    sPlayerRightHandHookshotDLs,      // PLAYER_MODELTYPE_RH_HOOKSHOT
    sSwordAndSheathDLs,               // PLAYER_MODELTYPE_SHEATH_16
    sSheathDLs,                       // PLAYER_MODELTYPE_SHEATH_17
    sSheathWithSwordDLs,              // PLAYER_MODELTYPE_SHEATH_18
    sSheathWithoutSwordDLs,           // PLAYER_MODELTYPE_SHEATH_19
    sPlayerWaistDLs,                  // PLAYER_MODELTYPE_WAIST
};

Gfx gCullBackDList[] = {
    gsSPSetGeometryMode(G_CULL_BACK),
    gsSPEndDisplayList(),
};

Gfx gCullFrontDList[] = {
    gsSPSetGeometryMode(G_CULL_FRONT),
    gsSPEndDisplayList(),
};

Vec3f* D_80160000;
s32 sDListsLodOffset;
Vec3f sGetItemRefPos;
s32 sLeftHandType;
s32 sRightHandType;

void Player_SetBootData(PlayState* play, Player* this) {
    s32 currentBoots;
    s16* bootRegs;

    REG(27) = 2000;
    REG(48) = 370;

    currentBoots = this->currentBoots;
    if (currentBoots == PLAYER_BOOTS_KOKIRI) {
        if (!LINK_IS_ADULT) {
            currentBoots = PLAYER_BOOTS_KOKIRI_CHILD;
        }
    } else if (currentBoots == PLAYER_BOOTS_IRON) {
        if (this->stateFlags1 & PLAYER_STATE1_IN_WATER) {
            currentBoots = PLAYER_BOOTS_IRON_UNDERWATER;
        }
        REG(27) = 500;
        REG(48) = 100;
    }

    bootRegs = sBootData[currentBoots];
    REG(19) = bootRegs[0];
    REG(30) = bootRegs[1];
    REG(32) = bootRegs[2];
    REG(34) = bootRegs[3];
    REG(35) = bootRegs[4];
    REG(36) = bootRegs[5];
    REG(37) = bootRegs[6];
    REG(38) = bootRegs[7];
    REG(43) = bootRegs[8];
    REG(45) = bootRegs[9];
    REG(68) = bootRegs[10];
    REG(69) = bootRegs[11];
    IREG(66) = bootRegs[12];
    IREG(67) = bootRegs[13];
    IREG(68) = bootRegs[14];
    IREG(69) = bootRegs[15];
    MREG(95) = bootRegs[16];

    if (play->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_2) {
        REG(45) = 500;
    }

    // FD (2026-07-13): Fierce Deity walk/gait -- matches 2ship's dedicated FD boot registers
    // (mm/src/code/z_player_lib.c D_801BFE14 FD row): a HIGHER run-speed cap (10.0 vs human ~5.5) paired with
    // ~HALVED leg-cadence multipliers, so FD's 1.5x body takes long strides at a calm leg cycle instead of the
    // frantic adult-cadence gait. Same skeleton/animations as before -- only these REGs change. Now always on (the
    // previous compromise + its toggle were retired). FD-ONLY: this whole block only runs for LINK_IS_DEITY; adult/
    // child keep the boot-table REGs loaded above. The paired run-cadence BASE (1.2 -> 0.6) lives at the two
    // func_8084029C run sites in z_player.c -- the run cadence base is hardcoded there rather than a boot REG.
    if (LINK_IS_DEITY) {
        REG(45) = 1000; // R_RUN_SPEED_LIMIT: 10.0 run cap (2ship FD)
        REG(35) = 366;  // walk cadence base       (2ship FD)
        REG(36) = 200;  // walk cadence x speed     (2ship FD; ~half of human)
        REG(38) = 175;  // run cadence x speed      (2ship FD; ~half of human)
        REG(30) = 666;  // sidestep cadence base    (2ship FD)
        REG(32) = 200;  // sidestep cadence x speed (2ship FD)
        MREG(95) = 65;  // bow / side-walk anim playSpeed (2ship FD)
        REG(27) = 1200; // turn-rate step: FD turns a touch slower (2ship FD)
        if (play->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_2) {
            REG(45) = 500; // indoor cap, same as human (2ship)
        }
        // FALLBACK (retired): the previous behavior was a single compromise cap with adult cadence REGs untouched:
        //     REG(45) = 700;
    }
}

// Custom method used to determine if we're using a custom model for link
uint8_t Player_IsCustomLinkModel() {
    return (LINK_IS_ADULT && ResourceGetIsCustomByName(gLinkAdultSkel)) ||
           (LINK_IS_CHILD && ResourceGetIsCustomByName(gLinkChildSkel));
}

s32 Player_InBlockingCsMode(PlayState* play, Player* this) {
    return (this->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_CUTSCENE)) || (this->csAction != 0) ||
           (play->transitionTrigger == TRANS_TRIGGER_START) || (this->stateFlags1 & PLAYER_STATE1_LOADING) ||
           (this->stateFlags3 & PLAYER_STATE3_FLYING_WITH_HOOKSHOT) ||
           ((gSaveContext.magicState != MAGIC_STATE_IDLE) && (Player_ActionToMagicSpell(this, this->itemAction) >= 0));
}

s32 Player_InCsMode(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return Player_InBlockingCsMode(play, this) || (this->unk_6AD == 4);
}

/**
 * Checks if Player is currently locked onto a hostile actor.
 * `PLAYER_STATE1_HOSTILE_LOCK_ON` controls Player's "battle" response to hostile actors.
 *
 * Note that within Player, `Player_UpdateHostileLockOn` exists, which updates the flag and also returns the check.
 * Player can use this function instead if the flag should be checked, but not updated.
 */
s32 Player_CheckHostileLockOn(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_HOSTILE_LOCK_ON);
}

s32 Player_IsChildWithHylianShield(Player* this) {
    return gSaveContext.linkAge != 0 && (this->currentShield == PLAYER_SHIELD_HYLIAN);
}

s32 Player_ActionToModelGroup(Player* this, s32 actionParam) {
    s32 modelGroup = sActionModelGroups[actionParam];

    if ((modelGroup == PLAYER_MODELGROUP_SWORD_AND_SHIELD) && Player_IsChildWithHylianShield(this)) {
        // child, using kokiri sword with hylian shield equipped
        return PLAYER_MODELGROUP_CHILD_HYLIAN_SHIELD;
    } else {
        return modelGroup;
    }
}

void Player_SetModelsForHoldingShield(Player* this) {
    if ((this->stateFlags1 & PLAYER_STATE1_SHIELDING) &&
        ((this->itemAction < 0) || (this->itemAction == this->heldItemAction))) {
        if ((CVarGetInteger(CVAR_CHEAT("ShieldTwoHanded"), 0) && (this->heldItemAction != PLAYER_IA_DEKU_STICK) ||
             !Player_HoldsTwoHandedWeapon(this)) &&
            !Player_IsChildWithHylianShield(this)) {
            this->rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
            if (LINK_IS_CHILD && (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) &&
                (this->currentShield == PLAYER_SHIELD_MIRROR)) {
                this->rightHandDLists = &sPlayerDListGroups[PLAYER_MODELTYPE_RH_SHIELD][0];
            } else if (LINK_IS_ADULT && (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) &&
                       (this->currentShield == PLAYER_SHIELD_DEKU)) {
                this->rightHandDLists = &sPlayerDListGroups[PLAYER_MODELTYPE_RH_SHIELD][1];
            } else {
                this->rightHandDLists = &sPlayerDListGroups[PLAYER_MODELTYPE_RH_SHIELD][gSaveContext.linkAge];
            }
            if (this->sheathType == PLAYER_MODELTYPE_SHEATH_18) {
                this->sheathType = PLAYER_MODELTYPE_SHEATH_16;
            } else if (this->sheathType == PLAYER_MODELTYPE_SHEATH_19) {
                this->sheathType = PLAYER_MODELTYPE_SHEATH_17;
            }
            this->sheathDLists = &sPlayerDListGroups[this->sheathType][gSaveContext.linkAge];
            if ((CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) && LINK_IS_CHILD &&
                gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI) {
                this->sheathDLists = &sPlayerDListGroups[this->sheathType][0];
            } else if ((CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) && LINK_IS_ADULT &&
                       gSaveContext.equips.buttonItems[0] == ITEM_SWORD_KOKIRI) {
                this->sheathDLists = &sPlayerDListGroups[this->sheathType][1];
            }
            this->modelAnimType = PLAYER_ANIMTYPE_2;
            this->itemAction = -1;
        }
    }
}

void Player_SetModels(Player* this, s32 modelGroup) {
    // Left hand
    this->leftHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_LEFT_HAND];
    this->leftHandDLists = &sPlayerDListGroups[this->leftHandType][gSaveContext.linkAge];

    if (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) {
        if (LINK_IS_CHILD &&
            (this->leftHandType == PLAYER_MODELTYPE_LH_HAMMER ||
             ((this->leftHandType == PLAYER_MODELTYPE_LH_SWORD || this->leftHandType == PLAYER_MODELTYPE_LH_BGS) &&
              (gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI)))) {
            this->leftHandDLists = &sPlayerDListGroups[this->leftHandType][0];
        }

        if (LINK_IS_ADULT && (this->leftHandType == PLAYER_MODELTYPE_LH_BOOMERANG ||
                              (this->leftHandType == PLAYER_MODELTYPE_LH_SWORD &&
                               gSaveContext.equips.buttonItems[0] == ITEM_SWORD_KOKIRI))) {
            this->leftHandDLists = &sPlayerDListGroups[this->leftHandType][1];
        }
    }

    // Right hand
    this->rightHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND];
    this->rightHandDLists = &sPlayerDListGroups[this->rightHandType][gSaveContext.linkAge];

    this->rightHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND];
    this->rightHandDLists = &sPlayerDListGroups[this->rightHandType][gSaveContext.linkAge];

    if (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) {
        if (LINK_IS_CHILD &&
            (this->rightHandType == PLAYER_MODELTYPE_RH_HOOKSHOT ||
             (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD && this->currentShield == PLAYER_SHIELD_MIRROR))) {
            this->rightHandDLists = &sPlayerDListGroups[this->rightHandType][0];
        }
        if (LINK_IS_ADULT &&
            (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD && this->currentShield == PLAYER_SHIELD_DEKU)) {
            this->rightHandDLists = &sPlayerDListGroups[this->rightHandType][1];
        }
    }
    if ((CVarGetInteger(CVAR_ENHANCEMENT("BowSlingshotAmmoFix"), 0) ||
         CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) &&
        this->rightHandType == 11) { // If holding Bow/Slingshot
        this->rightHandDLists = &sPlayerDListGroups[this->rightHandType][Player_HoldsSlingshot(this)];
    }

    // Sheath
    this->sheathType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_SHEATH];
    this->sheathDLists = &sPlayerDListGroups[this->sheathType][gSaveContext.linkAge];

    if (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) {
        if (LINK_IS_CHILD && (this->currentShield == PLAYER_SHIELD_HYLIAN &&
                                  ((gSaveContext.equips.buttonItems[0] == ITEM_SWORD_MASTER) ||
                                   (gSaveContext.equips.buttonItems[0] == ITEM_SWORD_BGS)) ||
                              (this->currentShield == PLAYER_SHIELD_MIRROR) &&
                                  (gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI))) {
            this->sheathDLists = &sPlayerDListGroups[this->sheathType][0];
        } else if (LINK_IS_CHILD && this->currentShield == PLAYER_SHIELD_MIRROR &&
                   gSaveContext.equips.buttonItems[0] == ITEM_SWORD_KOKIRI &&
                   this->sheathType == PLAYER_MODELTYPE_SHEATH_18) {
            this->sheathDLists = &sPlayerDListGroups[this->sheathType][0];
        } else if (LINK_IS_ADULT && (this->currentShield == PLAYER_SHIELD_DEKU &&
                                     gSaveContext.equips.buttonItems[0] != ITEM_SWORD_MASTER) ||
                   (gSaveContext.equips.buttonItems[0] == ITEM_SWORD_MASTER &&
                    this->sheathType == PLAYER_MODELTYPE_SHEATH_18 && this->currentShield == PLAYER_SHIELD_DEKU)) {
            this->sheathDLists = &sPlayerDListGroups[this->sheathType][1];
        } else if (LINK_IS_CHILD && this->sheathType == PLAYER_MODELTYPE_SHEATH_17 &&
                   ((gSaveContext.equips.buttonItems[0] == ITEM_SWORD_MASTER) ||
                    (gSaveContext.equips.buttonItems[0] == ITEM_SWORD_BGS))) {
            this->sheathDLists = &sPlayerDListGroups[this->sheathType][0];
        }
    }

    // Waist
    this->waistDLists = &sPlayerDListGroups[gPlayerModelTypes[modelGroup][4]][gSaveContext.linkAge];

    Player_SetModelsForHoldingShield(this);
    GameInteractor_ExecuteOnPlayerSetModels(this, modelGroup);
}

void Player_SetModelGroup(Player* this, s32 modelGroup) {
    this->modelGroup = modelGroup;

    if (modelGroup == PLAYER_MODELGROUP_CHILD_HYLIAN_SHIELD) {
        this->modelAnimType = PLAYER_ANIMTYPE_0;
    } else {
        this->modelAnimType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    }

    if ((this->modelAnimType < PLAYER_ANIMTYPE_3) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        this->modelAnimType = PLAYER_ANIMTYPE_0;
    }

    Player_SetModels(this, modelGroup);
}

void func_8008EC70(Player* this) {
    this->itemAction = this->heldItemAction;
    Player_SetModelGroup(this, Player_ActionToModelGroup(this, this->heldItemAction));
    this->unk_6AD = 0;
}

void Player_SetEquipmentData(PlayState* play, Player* this) {
    if (this->csAction != 0x56) {
        this->currentShield = SHIELD_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD));
        // FD (2026-07-11) bug 4: the Fierce Deity has NO shield -- ignore whatever shield the underlying age has
        // equipped so no shield model is ever drawn and the crouch-guard (which requires currentShield != NONE)
        // can never engage; FD uses its standing brace instead. In the RE this falls out of the deity model asset
        // (object_link_deity carries no shield geometry); forcing PLAYER_SHIELD_NONE is the code-side equivalent
        // for SoH, which loads the FD skeleton from mm.o2r. currentShield is recomputed from equips here on every
        // equip change, so reverting to human restores the real shield automatically.
        // TODO 2ship setting: a future toggle could let FD keep the underlying age's shield.
        if (LINK_IS_DEITY) {
            this->currentShield = PLAYER_SHIELD_NONE;
        }
        this->currentTunic = TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC));
        this->currentBoots = BOOTS_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS));
        this->currentSwordItemId = B_BTN_ITEM;
        Player_SetModelGroup(this, Player_ActionToModelGroup(this, this->heldItemAction));
        Player_SetBootData(play, this);
    }
}

void Player_UpdateBottleHeld(PlayState* play, Player* this, s32 item, s32 actionParam) {
    Inventory_UpdateBottleItem(play, item, this->heldItemButton);

    if (item != ITEM_BOTTLE) {
        this->heldItemId = item;
        this->heldItemAction = actionParam;
    }

    this->itemAction = actionParam;
}

void Player_ReleaseLockOn(Player* this) {
    this->focusActor = NULL;
    this->stateFlags2 &= ~PLAYER_STATE2_LOCK_ON_WITH_SWITCH;
}

/**
 * This function aims to clear Z-Target related state when it isn't in use.
 * It also handles setting a specific free fall related state that is interntwined with Z-Targeting.
 * TODO: Learn more about this and give a name to PLAYER_STATE1_19
 */
void Player_ClearZTargeting(Player* this) {
    if ((this->actor.bgCheckFlags & 1) ||
        (this->stateFlags1 & (PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER)) ||
        (!(this->stateFlags1 & (PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALL)) &&
         ((this->actor.world.pos.y - this->actor.floorHeight) < 100.0f))) {
        this->stateFlags1 &=
            ~(PLAYER_STATE1_Z_TARGETING | PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL |
              PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALL | PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE);
    } else if (!(this->stateFlags1 &
                 (PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALL | PLAYER_STATE1_CLIMBING_LADDER))) {
        this->stateFlags1 |= PLAYER_STATE1_FREEFALL;
    }

    Player_ReleaseLockOn(this);
}

/**
 * Sets the "auto lock-on actor" to lock onto an actor without Player's input.
 * This function will first release any existing lock-on or (try to) release parallel.
 *
 * When using Switch Targeting, it is not possible to carry an auto lock-on actor into a normal
 * lock-on when the auto lock-on is finished.
 * This is because the `PLAYER_STATE2_LOCK_ON_WITH_SWITCH` flag is never set with an auto lock-on.
 * With Hold Targeting it is possible to keep the auto lock-on going by keeping the Z button held down.
 *
 * The auto lock-on is considered "friendly" even if the actor is actually hostile. If the auto lock-on is hostile,
 * Player's battle response will not occur (if he is actionable) and the camera behaves differently.
 * When transitioning from auto lock-on to normal lock-on (with Hold Targeting) there will be a noticeable change
 * when it switches from "friendly" mode to "hostile" mode.
 */
void Player_SetAutoLockOnActor(PlayState* play, Actor* actor) {
    Player* this = GET_PLAYER(play);

    Player_ClearZTargeting(this);
    this->focusActor = actor;
    this->autoLockOnActor = actor;
    this->stateFlags1 |= PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS;
    Camera_SetParam(Play_GetCamera(play, 0), 8, actor);
    Camera_ChangeMode(Play_GetCamera(play, 0), 2);
}

s32 func_8008EF30(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->stateFlags1 & PLAYER_STATE1_ON_HORSE);
}

s32 func_8008EF44(PlayState* play, s32 ammo) {
    play->shootingGalleryStatus = ammo + 1;
    return 1;
}

s32 Player_IsBurningStickInRange(PlayState* play, Vec3f* pos, f32 xzRange, f32 yRange) {
    Player* this = GET_PLAYER(play);
    Vec3f diff;
    s32 pad;

    if ((this->heldItemAction == PLAYER_IA_DEKU_STICK) && (this->unk_860 != 0)) {
        Math_Vec3f_Diff(&this->meleeWeaponInfo[0].tip, pos, &diff);
        return ((SQ(diff.x) + SQ(diff.z)) <= SQ(xzRange)) && (0.0f <= diff.y) && (diff.y <= yRange);
    } else {
        return false;
    }
}

s32 Player_GetStrength(void) {
    s32 strengthUpgrade = CUR_UPG_VALUE(UPG_STRENGTH);

    // FD (2026-07-12) #9b: Fierce Deity has GOLD-gauntlet lifting strength (RE fd_build z_player_lib.c:963-965
    // returns PLAYER_STR_GOLD_G). OFF by default behind the cheat; when off, FD falls through to the normal
    // age/upgrade path below (no strength buff).
    if (LINK_IS_DEITY && CVarGetInteger(CVAR_CHEAT("TransformationMasks.FdIncreasedStrength"), 0)) {
        return PLAYER_STR_GOLD_G;
    }

    if (CVarGetInteger(CVAR_ENHANCEMENT("ToggleStrength"), 0) &&
        CVarGetInteger(CVAR_ENHANCEMENT("StrengthDisabled"), 0)) {
        return PLAYER_STR_NONE;
    }

    if (CVarGetInteger(CVAR_CHEAT("TimelessEquipment"), 0) || LINK_IS_ADULT) {
        return strengthUpgrade;
    } else if (strengthUpgrade != 0) {
        return PLAYER_STR_BRACELET;
    } else {
        return PLAYER_STR_NONE;
    }
}

u8 Player_GetMask(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return this->currentMask;
}

Player* Player_UnsetMask(PlayState* play) {
    Player* this = GET_PLAYER(play);

    this->currentMask = PLAYER_MASK_NONE;

    return this;
}

s32 Player_HasMirrorShieldEquipped(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->currentShield == PLAYER_SHIELD_MIRROR);
}

s32 Player_HasMirrorShieldSetToDraw(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) && (this->currentShield == PLAYER_SHIELD_MIRROR);
}

s32 Player_ActionToMagicSpell(Player* this, s32 actionParam) {
    s32 magicSpell = actionParam - PLAYER_IA_MAGIC_SPELL_15;

    if ((magicSpell >= 0) && (magicSpell < 6)) {
        return magicSpell;
    } else {
        return -1;
    }
}

s32 Player_HoldsHookshot(Player* this) {
    return (this->heldItemAction == PLAYER_IA_HOOKSHOT) || (this->heldItemAction == PLAYER_IA_LONGSHOT);
}

s32 Player_HoldsBow(Player* this) {
    switch (this->heldItemAction) {
        case PLAYER_IA_BOW:
        case PLAYER_IA_BOW_FIRE:
        case PLAYER_IA_BOW_ICE:
        case PLAYER_IA_BOW_LIGHT:
            return true;
        default:
            return false;
    }
}

s32 Player_HoldsSlingshot(Player* this) {
    return this->heldItemAction == PLAYER_IA_SLINGSHOT;
}

s32 func_8008F128(Player* this) {
    return Player_HoldsHookshot(this) && (this->heldActor == NULL);
}

s32 Player_ActionToMeleeWeapon(s32 actionParam) {
    s32 sword = actionParam - PLAYER_IA_FISHING_POLE;

    if ((sword > 0) && (sword < 6)) {
        return sword;
    } else {
        return 0;
    }
}

s32 Player_GetMeleeWeaponHeld(Player* this) {
    return Player_ActionToMeleeWeapon(this->heldItemAction);
}

s32 Player_HoldsTwoHandedWeapon(Player* this) {
    if ((this->heldItemAction >= PLAYER_IA_SWORD_BIGGORON) && (this->heldItemAction <= PLAYER_IA_HAMMER)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_HoldsBrokenKnife(Player* this) {
    if (this->heldItemId == ITEM_SWORD_DEITY) { // FD (2026-07-11): the deity sword never counts as broken
        return false;
    }
    return (this->heldItemAction == PLAYER_IA_SWORD_BIGGORON) && (gSaveContext.swordHealth <= 0.0f);
}

s32 Player_ActionToBottle(Player* this, s32 actionParam) {
    s32 bottle = actionParam - PLAYER_IA_BOTTLE;

    if ((bottle >= 0) && (bottle < 13)) {
        return bottle;
    } else {
        return -1;
    }
}

s32 Player_GetBottleHeld(Player* this) {
    return Player_ActionToBottle(this, this->heldItemAction);
}

s32 Player_ActionToExplosive(Player* this, s32 actionParam) {
    s32 explosive = actionParam - PLAYER_IA_BOMB;

    if ((explosive >= 0) && (explosive < 2)) {
        return explosive;
    } else {
        return -1;
    }
}

s32 Player_GetExplosiveHeld(Player* this) {
    return Player_ActionToExplosive(this, this->heldItemAction);
}

s32 func_8008F2BC(Player* this, s32 actionParam) {
    s32 sword = 0;

    if (actionParam != PLAYER_IA_SWORD_CS) {
        sword = actionParam - PLAYER_IA_SWORD_MASTER;
        if ((sword < 0) || (sword >= 3)) {
            goto return_neg;
        }
    }

    return sword;

return_neg:
    return -1;
}

s32 Player_GetEnvironmentalHazard(PlayState* play) {
    Player* this = GET_PLAYER(play);
    TextTriggerEntry* triggerEntry;
    s32 envHazard;

    if (play->roomCtx.curRoom.behaviorType2 == ROOM_BEHAVIOR_TYPE2_3) { // Room is hot
        envHazard = PLAYER_ENV_HAZARD_HOTROOM - 1;
    } else if ((this->underwaterTimer > 80) &&
               ((this->currentBoots == PLAYER_BOOTS_IRON) || (this->underwaterTimer >= 300))) { // Deep underwater
        envHazard = ((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & 1))
                        ? (PLAYER_ENV_HAZARD_UNDERWATER_FLOOR - 1)
                        : (PLAYER_ENV_HAZARD_UNDERWATER_FREE - 1);
    } else if (this->stateFlags1 & PLAYER_STATE1_IN_WATER) { // Swimming
        envHazard = PLAYER_ENV_HAZARD_SWIMMING - 1;
    } else {
        return PLAYER_ENV_HAZARD_NONE;
    }

    // Trigger general textboxes under certain conditions, like "It's so hot in here!"
    if (!Player_InCsMode(play)) {
        triggerEntry = &sTextTriggers[envHazard];

        if ((triggerEntry->flag != 0) && !(gSaveContext.textTriggerFlags & triggerEntry->flag) &&
            (((envHazard == (PLAYER_ENV_HAZARD_HOTROOM - 1)) &&
              (this->currentTunic != PLAYER_TUNIC_GORON && CVarGetInteger(CVAR_CHEAT("SuperTunic"), 0) == 0 &&
               CVarGetInteger(CVAR_ENHANCEMENT("DisableTunicWarningText"), 0) == 0)) ||
             (((envHazard == (PLAYER_ENV_HAZARD_UNDERWATER_FLOOR - 1)) ||
               (envHazard == (PLAYER_ENV_HAZARD_UNDERWATER_FREE - 1))) &&
              (this->currentBoots == PLAYER_BOOTS_IRON) &&
              (this->currentTunic != PLAYER_TUNIC_ZORA && CVarGetInteger(CVAR_CHEAT("SuperTunic"), 0) == 0 &&
               CVarGetInteger(CVAR_ENHANCEMENT("DisableTunicWarningText"), 0) == 0)))) {
            Message_StartTextbox(play, triggerEntry->textId, NULL);
            gSaveContext.textTriggerFlags |= triggerEntry->flag;
        }
    }

    return envHazard + 1;
}

u8 sEyeMouthIndexes[][2] = {
    { 0, 0 }, { 1, 0 }, { 2, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 }, { 4, 0 }, { 5, 1 },
    { 7, 2 }, { 0, 2 }, { 3, 0 }, { 4, 0 }, { 2, 2 }, { 1, 1 }, { 0, 2 }, { 0, 0 },
};

/**
 * Link's eye and mouth textures are placed at the exact same place in adult and child Link's respective object files.
 * This allows the array to only contain the symbols for one file and have it apply to both. This is a problem for
 * shiftability, and changes will need to be made in the code to account for this in a modding scenario. The symbols
 * from adult Link's object are used here.
 */

#if defined(MODDING) || defined(_MSC_VER) || defined(__GNUC__)
// TODO: Formatting
void* sEyeTextures[3][8] = { // FD (2026-07-11): +DEITY row
    { gLinkAdultEyesOpenTex, gLinkAdultEyesHalfTex, gLinkAdultEyesClosedfTex, gLinkAdultEyesRollLeftTex,
      gLinkAdultEyesRollRightTex, gLinkAdultEyesShockTex, gLinkAdultEyesUnk1Tex, gLinkAdultEyesUnk2Tex },
    { gLinkChildEyesOpenTex, gLinkChildEyesHalfTex, gLinkChildEyesClosedfTex, gLinkChildEyesRollLeftTex,
      gLinkChildEyesRollRightTex, gLinkChildEyesShockTex, gLinkChildEyesUnk1Tex, gLinkChildEyesUnk2Tex },
    // FD (2026-07-11): deity face DL does not sample seg-8 (Aegiker ships all NULL)
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

#else
void* sEyeTextures[] = {
    gLinkAdultEyesOpenTex,      gLinkAdultEyesHalfTex,  gLinkAdultEyesClosedfTex, gLinkAdultEyesRollLeftTex,
    gLinkAdultEyesRollRightTex, gLinkAdultEyesShockTex, gLinkAdultEyesUnk1Tex,    gLinkAdultEyesUnk2Tex,
};
#endif

#if defined(MODDING) || defined(_MSC_VER) || defined(__GNUC__)
void* sMouthTextures[3][4] = { // FD (2026-07-11): +DEITY row
    {
        gLinkAdultMouth1Tex,
        gLinkAdultMouth2Tex,
        gLinkAdultMouth3Tex,
        gLinkAdultMouth4Tex,
    },
    {
        gLinkChildMouth1Tex,
        gLinkChildMouth2Tex,
        gLinkChildMouth3Tex,
        gLinkChildMouth4Tex,
    },
    // FD (2026-07-11): deity face DL does not sample seg-9 (Aegiker ships all NULL)
    {
        NULL,
        NULL,
        NULL,
        NULL,
    },
};
#else
void* sMouthTextures[] = {
    gLinkAdultMouth1Tex,
    gLinkAdultMouth2Tex,
    gLinkAdultMouth3Tex,
    gLinkAdultMouth4Tex,
};
#endif

Color_RGB8 sTunicColors[] = {
    { 30, 105, 27 },
    { 100, 20, 0 },
    { 0, 60, 100 },
};

Color_RGB8 sGauntletColors[] = {
    { 255, 255, 255 },
    { 254, 207, 15 },
    // #region SOH [RBA] values matching OOB reads on N64
    { 0, 0, 6 },
    { 2, 89, 24 },
    { 6, 2, 90 },
    { 96, 6, 2 },
};

Gfx* sBootDListGroups[][2] = {
    { gLinkAdultLeftIronBootDL, gLinkAdultRightIronBootDL },   // PLAYER_BOOTS_IRON
    { gLinkAdultLeftHoverBootDL, gLinkAdultRightHoverBootDL }, // PLAYER_BOOTS_HOVER
};

void Player_DrawImpl(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount, s32 lod, s32 tunic, s32 boots,
                     s32 face, OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* data) {
    Color_RGB8* color;
    s32 eyeIndex = (jointTable[22].x & 0xF) - 1;
    s32 mouthIndex = (jointTable[22].x >> 4) - 1;

    OPEN_DISPS(play->state.gfxCtx);

    if (eyeIndex < 0) {
        eyeIndex = sEyeMouthIndexes[face][0];
    }

    if (eyeIndex > 7)
        eyeIndex = 7;

#if defined(MODDING) || defined(_MSC_VER) || defined(__GNUC__)
    gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sEyeTextures[gSaveContext.linkAge][eyeIndex]));
#else
    gSPSegment(POLY_OPA_DISP++, 0x08, SEGMENTED_TO_VIRTUAL(sEyeTextures[eyeIndex]));
#endif
    if (mouthIndex < 0) {
        mouthIndex = sEyeMouthIndexes[face][1];
    }

    if (mouthIndex > 3)
        mouthIndex = 3;

#if defined(MODDING) || defined(_MSC_VER) || defined(__GNUC__)
    gSPSegment(POLY_OPA_DISP++, 0x09, SEGMENTED_TO_VIRTUAL(sMouthTextures[gSaveContext.linkAge][mouthIndex]));
#else
    gSPSegment(POLY_OPA_DISP++, 0x09, SEGMENTED_TO_VIRTUAL(sMouthTextures[eyeIndex]));
#endif

    Color_RGB8 sTemp;
    color = &sTunicColors[tunic];
    if (tunic == PLAYER_TUNIC_KOKIRI && CVarGetInteger(CVAR_COSMETIC("Link.KokiriTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.KokiriTunic.Value"), sTunicColors[PLAYER_TUNIC_KOKIRI]);
        color = &sTemp;
    } else if (tunic == PLAYER_TUNIC_GORON && CVarGetInteger(CVAR_COSMETIC("Link.GoronTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.GoronTunic.Value"), sTunicColors[PLAYER_TUNIC_GORON]);
        color = &sTemp;
    } else if (tunic == PLAYER_TUNIC_ZORA && CVarGetInteger(CVAR_COSMETIC("Link.ZoraTunic.Changed"), 0)) {
        sTemp = CVarGetColor24(CVAR_COSMETIC("Link.ZoraTunic.Value"), sTunicColors[PLAYER_TUNIC_ZORA]);
        color = &sTemp;
    }

    if (GameInteractor_Should(VB_APPLY_TUNIC_COLOR, true, data, color)) {
        gDPSetEnvColor(POLY_OPA_DISP++, color->r, color->g, color->b, 0);
    }

    // If we have a custom link model, always use the most detailed LOD
    if (Player_IsCustomLinkModel()) {
        lod = 0;
    }

    sDListsLodOffset = lod * NUM_DL_FORMS; // FD (2026-07-11): DL groups now stride 3 forms per LOD

    SkelAnime_DrawFlexLod(play, skeleton, jointTable, dListCount, overrideLimbDraw, postLimbDraw, data, lod);

    if (((CVarGetInteger(CVAR_ENHANCEMENT("FirstPersonGauntlets"), 0) && LINK_IS_ADULT) ||
         (overrideLimbDraw != Player_OverrideLimbDrawGameplayFirstPerson)) &&
        (overrideLimbDraw != Player_OverrideLimbDrawGameplayCrawling) &&
        (gSaveContext.gameMode != GAMEMODE_END_CREDITS)) {
        if (LINK_IS_ADULT) {
            s32 strengthUpgrade = CUR_UPG_VALUE(UPG_STRENGTH);

            if (strengthUpgrade >= 2) { // silver or gold gauntlets
                gDPPipeSync(POLY_OPA_DISP++);

                color = &sGauntletColors[strengthUpgrade - 2];
                if (strengthUpgrade == PLAYER_STR_SILVER_G &&
                    CVarGetInteger(CVAR_COSMETIC("Gloves.SilverGauntlets.Changed"), 0)) {
                    sTemp = CVarGetColor24(CVAR_COSMETIC("Gloves.SilverGauntlets.Value"), *color);
                    color = &sTemp;
                } else if (strengthUpgrade == PLAYER_STR_GOLD_G &&
                           CVarGetInteger(CVAR_COSMETIC("Gloves.GoldenGauntlets.Changed"), 0)) {
                    sTemp = CVarGetColor24(CVAR_COSMETIC("Gloves.GoldenGauntlets.Value"), *color);
                    color = &sTemp;
                }
                gDPSetEnvColor(POLY_OPA_DISP++, color->r, color->g, color->b, 0);

                gSPDisplayList(POLY_OPA_DISP++, gLinkAdultLeftGauntletPlate1DL);
                gSPDisplayList(POLY_OPA_DISP++, gLinkAdultRightGauntletPlate1DL);
                gSPDisplayList(POLY_OPA_DISP++, (sLeftHandType == PLAYER_MODELTYPE_LH_OPEN)
                                                    ? gLinkAdultLeftGauntletPlate2DL
                                                    : gLinkAdultLeftGauntletPlate3DL);
                gSPDisplayList(POLY_OPA_DISP++, (sRightHandType == PLAYER_MODELTYPE_RH_OPEN)
                                                    ? gLinkAdultRightGauntletPlate2DL
                                                    : gLinkAdultRightGauntletPlate3DL);
            }

            if (boots != 0) {
                Gfx** bootDLists = sBootDListGroups[boots - 1];

                gSPDisplayList(POLY_OPA_DISP++, bootDLists[0]);
                gSPDisplayList(POLY_OPA_DISP++, bootDLists[1]);
            }
        } else if (LINK_IS_CHILD) { // FD (2026-07-11): was `else`; guard so DEITY skips child-bracelet seg-6 DL
            if (Player_GetStrength() > PLAYER_STR_NONE) {
                gSPDisplayList(POLY_OPA_DISP++, gLinkChildGoronBraceletDL);
            }
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

// FD (2026-07-11): leg-IK tables widened +DEITY (= adult values; FD is adult-proportioned).
Vec3f D_80126038[] = {
    { 1304.0f, 0.0f, 0.0f },
    { 695.0f, 0.0f, 0.0f },
    { 1304.0f, 0.0f, 0.0f }, // FD (2026-07-11): deity = adult
};

f32 D_80126050[] = { 1265.0f, 826.0f, 1265.0f }; // FD (2026-07-11): deity = adult
f32 D_80126058[] = { SQ(13.04f), SQ(6.95f), SQ(13.04f) }; // FD (2026-07-11): deity = adult
f32 D_80126060[] = { 10.019104f, -19.925102f, 10.019104f }; // FD (2026-07-11): deity = adult
f32 D_80126068[] = { 5.0f, 3.0f, 5.0f }; // FD (2026-07-11): deity = adult

Vec3f D_80126070 = { 0.0f, -300.0f, 0.0f };

void func_8008F87C(PlayState* play, Player* this, SkelAnime* skelAnime, Vec3f* pos, Vec3s* rot, s32 thighLimbIndex,
                   s32 shinLimbIndex, s32 footLimbIndex) {
    Vec3f spA4;
    Vec3f sp98;
    Vec3f footprintPos;
    CollisionPoly* sp88;
    s32 sp84;
    f32 sp80;
    f32 sp7C;
    f32 sp78;
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    s16 temp1;
    s16 temp2;
    s32 temp3;

    if ((this->actor.scale.y >= 0.0f) && !(this->stateFlags1 & PLAYER_STATE1_DEAD) &&
        (Player_ActionToMagicSpell(this, this->itemAction) < 0)) {
        s32 pad;

        sp7C = D_80126058[gSaveContext.linkAge];
        sp78 = D_80126060[gSaveContext.linkAge];
        sp74 = D_80126068[gSaveContext.linkAge] - this->unk_6C4;

        Matrix_Push();
        Matrix_TranslateRotateZYX(pos, rot);
        Matrix_MultVec3f(&sZeroVec, &spA4);
        Matrix_TranslateRotateZYX(&D_80126038[gSaveContext.linkAge], &skelAnime->jointTable[shinLimbIndex]);
        Matrix_Translate(D_80126050[gSaveContext.linkAge], 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_MultVec3f(&sZeroVec, &sp98);
        Matrix_MultVec3f(&D_80126070, &footprintPos);
        Matrix_Pop();

        footprintPos.y += 15.0f;

        sp80 = BgCheck_EntityRaycastFloor4(&play->colCtx, &sp88, &sp84, &this->actor, &footprintPos) + sp74;

        if (sp98.y < sp80) {
            sp70 = sp98.x - spA4.x;
            sp6C = sp98.y - spA4.y;
            sp68 = sp98.z - spA4.z;

            sp64 = sqrtf(SQ(sp70) + SQ(sp6C) + SQ(sp68));
            sp60 = (SQ(sp64) + sp78) / (2.0f * sp64);

            sp58 = sp7C - SQ(sp60);
            sp58 = (sp7C < SQ(sp60)) ? 0.0f : sqrtf(sp58);

            sp54 = Math_FAtan2F(sp58, sp60);

            sp6C = sp80 - spA4.y;

            sp64 = sqrtf(SQ(sp70) + SQ(sp6C) + SQ(sp68));
            sp60 = (SQ(sp64) + sp78) / (2.0f * sp64);
            sp5C = sp64 - sp60;

            sp58 = sp7C - SQ(sp60);
            sp58 = (sp7C < SQ(sp60)) ? 0.0f : sqrtf(sp58);

            sp50 = Math_FAtan2F(sp58, sp60);

            temp1 = (M_PI - (Math_FAtan2F(sp5C, sp58) + ((M_PI / 2) - sp50))) * (0x8000 / M_PI);
            temp1 = temp1 - skelAnime->jointTable[shinLimbIndex].z;

            if ((s16)(ABS(skelAnime->jointTable[shinLimbIndex].x) + ABS(skelAnime->jointTable[shinLimbIndex].y)) < 0) {
                temp1 += 0x8000;
            }

            temp2 = (sp50 - sp54) * (0x8000 / M_PI);
            rot->z -= temp2;

            skelAnime->jointTable[thighLimbIndex].z = skelAnime->jointTable[thighLimbIndex].z - temp2;
            skelAnime->jointTable[shinLimbIndex].z = skelAnime->jointTable[shinLimbIndex].z + temp1;
            skelAnime->jointTable[footLimbIndex].z = skelAnime->jointTable[footLimbIndex].z + temp2 - temp1;

            temp3 = func_80041D4C(&play->colCtx, sp88, sp84);

            if ((temp3 >= 2) && (temp3 < 4) && !SurfaceType_IsWallDamage(&play->colCtx, sp88, sp84)) {
                footprintPos.y = sp80;
                EffectSsGFire_Spawn(play, &footprintPos);
            }
        }
    }
}

s32 Player_OverrideLimbDrawGameplayCommon(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                          void* thisx) {
    Player* this = (Player*)thisx;

    if (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0) &&
        CVarGetInteger(CVAR_ENHANCEMENT("ScaleAdultEquipmentAsChild"), 0) && LINK_IS_CHILD) {
        if (limbIndex == PLAYER_LIMB_L_HAND) {
            if ((gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI &&
                 sLeftHandType == PLAYER_MODELTYPE_LH_SWORD) ||
                (sLeftHandType == PLAYER_MODELTYPE_LH_BGS) || (sLeftHandType == PLAYER_MODELTYPE_LH_HAMMER)) {
                Matrix_Scale(0.8, 0.8, 0.8, MTXMODE_APPLY);
            }
        }
        if (limbIndex == PLAYER_LIMB_R_HAND) {
            if ((this->currentShield == PLAYER_SHIELD_MIRROR && sRightHandType == PLAYER_MODELTYPE_RH_SHIELD) ||
                sRightHandType == PLAYER_MODELTYPE_RH_HOOKSHOT ||
                (sRightHandType == PLAYER_MODELTYPE_RH_BOW_SLINGSHOT && Player_HoldsBow(this))) {
                Matrix_Scale(0.8, 0.8, 0.8, MTXMODE_APPLY);
            }
        }
        if (limbIndex == PLAYER_LIMB_SHEATH) {
            if ((this->currentShield == PLAYER_SHIELD_MIRROR ||
                 (this->currentShield == PLAYER_SHIELD_HYLIAN &&
                  (gSaveContext.equips.buttonItems[0] == ITEM_SWORD_MASTER ||
                   gSaveContext.equips.buttonItems[0] == ITEM_SWORD_BGS))) &&
                ((this->sheathType == PLAYER_MODELTYPE_SHEATH_16) || (this->sheathType == PLAYER_MODELTYPE_SHEATH_17) ||
                 (this->sheathType == PLAYER_MODELTYPE_SHEATH_18) ||
                 (this->sheathType == PLAYER_MODELTYPE_SHEATH_19))) {
                Matrix_Translate(218, -100, 62, MTXMODE_APPLY);
                Matrix_Scale(0.8, 0.8, 0.8, MTXMODE_APPLY);
            }
            if ((this->currentShield == PLAYER_SHIELD_DEKU && gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI &&
                 (this->sheathType == PLAYER_MODELTYPE_SHEATH_16 || this->sheathType == PLAYER_MODELTYPE_SHEATH_17))) {
                Matrix_Translate(218, -100, 62, MTXMODE_APPLY);
                Matrix_Scale(0.8, 0.8, 0.8, MTXMODE_APPLY);
            }
        }
    }

    if (limbIndex == PLAYER_LIMB_ROOT) {
        sLeftHandType = this->leftHandType;
        sRightHandType = this->rightHandType;
        D_80160000 = &this->meleeWeaponInfo[2].base;

        // FD (2026-07-12): faithful port of the RE root-limb scale gate (fd_build z_player_lib.c:1432).
        // Vanilla shrinks the root (pelvis) translation by 0.64 for every non-adult, which WRONGLY
        // includes Fierce Deity (LINK_AGE_DEITY=2 satisfies !LINK_IS_ADULT) -> pelvis dropped ~18 world
        // units -> crouched legs + sunken feet. The RE excludes DEITY entirely and picks a form-aware
        // scale (>= GORON uses ageProperties->unk_08, else child's 0.64). This grounds FD at yOffset=0
        // and replaces the old shape.yOffset=900 compensation hack (removed in z_player.c).
        // SoH has only ADULT/CHILD/DEITY (no goron/zora/deku forms), so the RE's form-aware rootScale
        // (>= GORON -> ageProperties->unk_08) collapses to child's 0.64. The critical fix is excluding DEITY.
        if (!LINK_IS_ADULT && !LINK_IS_DEITY) {
            if (!(this->skelAnime.movementFlags & 4) || (this->skelAnime.movementFlags & 1)) {
                pos->x *= 0.64f;
                pos->z *= 0.64f;
            }

            if (!(this->skelAnime.movementFlags & 4) || (this->skelAnime.movementFlags & 2)) {
                pos->y *= 0.64f;
            }
        }

        pos->y -= this->unk_6C4;

        if (this->unk_6C2 != 0) {
            Matrix_Translate(pos->x, ((Math_CosS(this->unk_6C2) - 1.0f) * 200.0f) + pos->y, pos->z, MTXMODE_APPLY);
            Matrix_RotateX(this->unk_6C2 * (M_PI / 0x8000), MTXMODE_APPLY);
            Matrix_RotateZYX(rot->x, rot->y, rot->z, MTXMODE_APPLY);
            pos->x = pos->y = pos->z = 0.0f;
            rot->x = rot->y = rot->z = 0;
        }
    } else {
        if (*dList != NULL) {
            D_80160000++;
        }

        if (limbIndex == PLAYER_LIMB_HEAD) {
            if (CVarGetInteger(CVAR_COSMETIC("Link.HeadScale.Changed"), 0)) {
                f32 scale = CVarGetFloat(CVAR_COSMETIC("Link.HeadScale.Value"), 1.0f);
                if (scale != 1.0f) {
                    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
                    if (scale > 1.2f) {
                        Matrix_Translate(-((LINK_IS_ADULT ? 320.0f : 200.0f) * scale), 0.0f, 0.0f, MTXMODE_APPLY);
                    } else if (scale < 1.0f) {
                        Matrix_Translate((LINK_IS_ADULT ? 3600.0f : 2900.0f) * ABS(scale - 1.0f), 0.0f, 0.0f,
                                         MTXMODE_APPLY);
                    }
                }
            }
            rot->x += this->headLimbRot.z;
            rot->y -= this->headLimbRot.y;
            rot->z += this->headLimbRot.x;
        } else if (limbIndex == PLAYER_LIMB_L_HAND) {
            if (CVarGetInteger(CVAR_COSMETIC("Link.SwordScale.Changed"), 0)) {
                f32 scale = CVarGetFloat(CVAR_COSMETIC("Link.SwordScale.Value"), 1.0f);
                if (scale != 1.0f) {
                    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
                    Matrix_Translate(-((LINK_IS_ADULT ? 320.0f : 200.0f) * (scale - 1.0f)), 0.0f, 0.0f, MTXMODE_APPLY);
                }
            }
        } else if (limbIndex == PLAYER_LIMB_UPPER) {
            if (this->upperLimbYawSecondary != 0) {
                Matrix_RotateZ(0x44C * (M_PI / 0x8000), MTXMODE_APPLY);
                Matrix_RotateY(this->upperLimbYawSecondary * (M_PI / 0x8000), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.y != 0) {
                Matrix_RotateY(this->upperLimbRot.y * (M_PI / 0x8000), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.x != 0) {
                Matrix_RotateX(this->upperLimbRot.x * (M_PI / 0x8000), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.z != 0) {
                Matrix_RotateZ(this->upperLimbRot.z * (M_PI / 0x8000), MTXMODE_APPLY);
            }
        } else if (limbIndex == PLAYER_LIMB_L_THIGH) {
            func_8008F87C(play, this, &this->skelAnime, pos, rot, PLAYER_LIMB_L_THIGH, PLAYER_LIMB_L_SHIN,
                          PLAYER_LIMB_L_FOOT);
        } else if (limbIndex == PLAYER_LIMB_R_THIGH) {
            func_8008F87C(play, this, &this->skelAnime, pos, rot, PLAYER_LIMB_R_THIGH, PLAYER_LIMB_R_SHIN,
                          PLAYER_LIMB_R_FOOT);
            return false;
        } else {
            return false;
        }
    }

    return false;
}

s32 Player_OverrideLimbDrawGameplayDefault(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                           void* thisx) {
    Player* this = (Player*)thisx;

    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        if (limbIndex == PLAYER_LIMB_L_HAND) {
            Gfx** dLists = this->leftHandDLists;

            if ((sLeftHandType == PLAYER_MODELTYPE_LH_BGS) &&
                (this->lastItem == ITEM_SWORD_DEITY || LINK_IS_DEITY)) {
                dLists += ((NUM_DL_FORMS * 2) * 2); // FD (2026-07-11): select FD-sword sub-block
            } else if ((sLeftHandType == PLAYER_MODELTYPE_LH_BGS) && (gSaveContext.swordHealth <= 0.0f)) {
                dLists += (NUM_DL_FORMS * 2); // FD (2026-07-11): was 4 (broken-knife sub-block)
            } else if ((sLeftHandType == PLAYER_MODELTYPE_LH_BOOMERANG) &&
                       (this->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN)) {
                dLists = &gPlayerLeftHandOpenDLs[gSaveContext.linkAge];
                sLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
            } else if ((this->leftHandType == PLAYER_MODELTYPE_LH_OPEN) && (this->actor.speedXZ > 2.0f) &&
                       !(this->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
                dLists = &gPlayerLeftHandClosedDLs[gSaveContext.linkAge];
                sLeftHandType = PLAYER_MODELTYPE_LH_CLOSED;
            }

            *dList = ResourceMgr_LoadGfxByName(dLists[sDListsLodOffset]);
        } else if (limbIndex == PLAYER_LIMB_R_HAND) {
            Gfx** dLists = this->rightHandDLists;

            if (sRightHandType == PLAYER_MODELTYPE_RH_SHIELD) {
                dLists += this->currentShield * (NUM_DL_FORMS * 2); // FD (2026-07-11): was *4
            } else if ((this->rightHandType == PLAYER_MODELTYPE_RH_OPEN) && (this->actor.speedXZ > 2.0f) &&
                       !(this->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
                dLists = &sPlayerRightHandClosedDLs[gSaveContext.linkAge];
                sRightHandType = PLAYER_MODELTYPE_RH_CLOSED;
            }

            *dList = ResourceMgr_LoadGfxByName(dLists[sDListsLodOffset]);
        } else if (limbIndex == PLAYER_LIMB_SHEATH) {
            Gfx** dLists = this->sheathDLists;

            if ((this->sheathType == PLAYER_MODELTYPE_SHEATH_18) || (this->sheathType == PLAYER_MODELTYPE_SHEATH_19)) {
                dLists += this->currentShield * (NUM_DL_FORMS * 2); // FD (2026-07-11): was *4
                if (!LINK_IS_ADULT && (this->currentShield < PLAYER_SHIELD_HYLIAN) &&
                    (gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI)) {
                    dLists += PLAYER_SHIELD_MAX * (NUM_DL_FORMS * 2); // FD (2026-07-11): was *4
                }
            } else if (!CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0) ||
                       (CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0) &&
                        ((gSaveContext.equips.buttonItems[0] != ITEM_SWORD_MASTER &&
                          gSaveContext.equips.buttonItems[0] != ITEM_SWORD_BGS) &&
                         this->currentShield == PLAYER_SHIELD_DEKU))) {
                if (!LINK_IS_ADULT &&
                    ((this->sheathType == PLAYER_MODELTYPE_SHEATH_16) ||
                     (this->sheathType == PLAYER_MODELTYPE_SHEATH_17)) &&
                    (gSaveContext.equips.buttonItems[0] != ITEM_SWORD_KOKIRI)) {
                    dLists = &sSheathWithSwordDLs[PLAYER_SHIELD_MAX * (NUM_DL_FORMS * 2)]; // FD (2026-07-11): was *4
                }
            }

            if (dLists[sDListsLodOffset] != NULL) {
                *dList = ResourceMgr_LoadGfxByName(dLists[sDListsLodOffset]);
            } else {
                *dList = NULL;
            }

        } else if (limbIndex == PLAYER_LIMB_WAIST) {

            if (!Player_IsCustomLinkModel()) {
                *dList = ResourceMgr_LoadGfxByName(
                    this->waistDLists[sDListsLodOffset]); // NOTE: This needs to be disabled when using custom
                                                          // characters - they're not going to have LODs anyways...
            }
        }
    }

    if (GameInteractor_InvisibleLinkActive()) {
        this->actor.shape.shadowDraw = NULL;
        *dList = NULL;
    }

    return false;
}

s32 Player_OverrideLimbDrawGameplayFirstPerson(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                               void* thisx) {
    Player* this = (Player*)thisx;

    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        if (this->unk_6AD != 2) {
            *dList = NULL;
        } else if (limbIndex == PLAYER_LIMB_L_FOREARM) {
            *dList = sFirstPersonLeftForearmDLs[gSaveContext.linkAge];
        } else if (limbIndex == PLAYER_LIMB_L_HAND) {
            s32 handOutDlIndex = gSaveContext.linkAge;
            if ((CVarGetInteger(CVAR_ENHANCEMENT("BowSlingshotAmmoFix"), 0) ||
                 CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) &&
                LINK_IS_ADULT && Player_HoldsSlingshot(this)) {
                handOutDlIndex = 1;
            }
            *dList = sFirstPersonLeftHandDLs[handOutDlIndex];
        } else if (limbIndex == PLAYER_LIMB_R_SHOULDER) {
            *dList = sFirstPersonRightShoulderDLs[gSaveContext.linkAge];
        } else if (limbIndex == PLAYER_LIMB_R_FOREARM) {
            *dList = sFirstPersonForearmDLs[gSaveContext.linkAge];
        } else if (limbIndex == PLAYER_LIMB_R_HAND) {
            s32 firstPersonWeaponIndex = gSaveContext.linkAge;
            if (CVarGetInteger(CVAR_ENHANCEMENT("BowSlingshotAmmoFix"), 0) ||
                CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) {
                if (Player_HoldsBow(this)) {
                    firstPersonWeaponIndex = 0;
                } else if (Player_HoldsSlingshot(this)) {
                    firstPersonWeaponIndex = 1;
                }
            }
            *dList = Player_HoldsHookshot(this) ? gLinkAdultRightHandHoldingHookshotFarDL
                                                : sFirstPersonRightHandHoldingWeaponDLs[firstPersonWeaponIndex];
        } else {
            *dList = NULL;
        }
    }
    return false;
}

s32 Player_OverrideLimbDrawGameplayCrawling(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                            void* thisx) {
    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        *dList = NULL;
    }

    return false;
}

u8 func_80090480(PlayState* play, ColliderQuad* collider, WeaponInfo* weaponInfo, Vec3f* newTip, Vec3f* newBase) {
    if (weaponInfo->active == 0) {
        if (collider != NULL) {
            Collider_ResetQuadAT(play, &collider->base);
        }
        Math_Vec3f_Copy(&weaponInfo->tip, newTip);
        Math_Vec3f_Copy(&weaponInfo->base, newBase);
        weaponInfo->active = 1;
        return 1;
    } else if ((weaponInfo->tip.x == newTip->x) && (weaponInfo->tip.y == newTip->y) &&
               (weaponInfo->tip.z == newTip->z) && (weaponInfo->base.x == newBase->x) &&
               (weaponInfo->base.y == newBase->y) && (weaponInfo->base.z == newBase->z)) {
        if (collider != NULL) {
            Collider_ResetQuadAT(play, &collider->base);
        }
        return 0;
    } else {
        if (collider != NULL) {
            Collider_SetQuadVertices(collider, newBase, newTip, &weaponInfo->base, &weaponInfo->tip);
            CollisionCheck_SetAT(play, &play->colChkCtx, &collider->base);
        }
        Math_Vec3f_Copy(&weaponInfo->base, newBase);
        Math_Vec3f_Copy(&weaponInfo->tip, newTip);
        weaponInfo->active = 1;
        return 1;
    }
}

void Player_UpdateShieldCollider(PlayState* play, Player* this, ColliderQuad* collider, Vec3f* quadSrc) {
    static u8 shieldColTypes[PLAYER_SHIELD_MAX] = {
        COLTYPE_METAL,
        COLTYPE_WOOD,
        COLTYPE_METAL,
        COLTYPE_METAL,
    };

    if (this->stateFlags1 & PLAYER_STATE1_SHIELDING) {
        Vec3f quadDest[4];

        this->shieldQuad.base.colType = shieldColTypes[this->currentShield];

        Matrix_MultVec3f(&quadSrc[0], &quadDest[0]);
        Matrix_MultVec3f(&quadSrc[1], &quadDest[1]);
        Matrix_MultVec3f(&quadSrc[2], &quadDest[2]);
        Matrix_MultVec3f(&quadSrc[3], &quadDest[3]);
        Collider_SetQuadVertices(collider, &quadDest[0], &quadDest[1], &quadDest[2], &quadDest[3]);

        CollisionCheck_SetAC(play, &play->colChkCtx, &collider->base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &collider->base);
    }
}

Vec3f D_80126080 = { 5000.0f, 400.0f, 0.0f };
Vec3f D_8012608C = { 5000.0f, -400.0f, 1000.0f };
Vec3f D_80126098 = { 5000.0f, 1400.0f, -1000.0f };

Vec3f D_801260A4[3] = {
    { 0.0f, 400.0f, 0.0f },
    { 0.0f, 1400.0f, -1000.0f },
    { 0.0f, -400.0f, 1000.0f },
};

void func_800906D4(PlayState* play, Player* this, Vec3f* newTipPos) {
    Vec3f newBasePos[3];

    Matrix_MultVec3f(&D_801260A4[0], &newBasePos[0]);
    Matrix_MultVec3f(&D_801260A4[1], &newBasePos[1]);
    Matrix_MultVec3f(&D_801260A4[2], &newBasePos[2]);

    if (func_80090480(play, NULL, &this->meleeWeaponInfo[0], &newTipPos[0], &newBasePos[0]) &&
        !(this->stateFlags1 & PLAYER_STATE1_SHIELDING) &&
        !CVarGetInteger(CVAR_ENHANCEMENT("DisableLinkSwordTrail"), 0)) {
        EffectBlure_AddVertex(Effect_GetByIndex(this->meleeWeaponEffectIndex), &this->meleeWeaponInfo[0].tip,
                              &this->meleeWeaponInfo[0].base);
    }

    if ((this->meleeWeaponState > 0) &&
        ((this->meleeWeaponAnimation < 0x18) || (this->stateFlags2 & PLAYER_STATE2_SPIN_ATTACKING))) {
        func_80090480(play, &this->meleeWeaponQuads[0], &this->meleeWeaponInfo[1], &newTipPos[1], &newBasePos[1]);
        func_80090480(play, &this->meleeWeaponQuads[1], &this->meleeWeaponInfo[2], &newTipPos[2], &newBasePos[2]);
    }
}

void Player_DrawGetItemIceTrap(PlayState* play, Player* this, Vec3f* refPos, s32 drawIdPlusOne, f32 height) {
    OPEN_DISPS(play->state.gfxCtx);

    if (CVarGetInteger(CVAR_GENERAL("LetItSnow"), 0)) {
        Gfx_SetupDL_25Opa(play->state.gfxCtx);

        Matrix_Scale(0.2f, 0.2f, 0.2f, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD);

        gDPSetGrayscaleColor(POLY_OPA_DISP++, 75, 75, 75, 255);
        gSPGrayscale(POLY_OPA_DISP++, true);

        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSilverRockDL);

        gSPGrayscale(POLY_OPA_DISP++, false);
    } else {
        if (iceTrapScale < 0.01) {
            iceTrapScale += 0.001f;
        } else if (iceTrapScale < 0.8f) {
            iceTrapScale += 0.2f;
        }

        // Draw the ice only after a bit so it doesn't spoil the fact that it's a trap
        if (iceTrapScale >= 0.01) {
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, (0 - play->gameplayFrames) % 128, 32, 32, 1, 0,
                                          (play->gameplayFrames * -2) % 128, 32, 32, 0, -1, 0, -2));

            Matrix_Translate(0.0f, -40.0f, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(iceTrapScale, iceTrapScale, iceTrapScale, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, 255);
            gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment3DL);

            // Reset matrix for the fake item model because we're animating the size of the ice block around it before
            // this.
            Matrix_Translate(refPos->x + (3.3f * Math_SinS(this->actor.shape.rot.y)), refPos->y + height,
                             refPos->z + ((3.3f + (IREG(90) / 10.0f)) * Math_CosS(this->actor.shape.rot.y)),
                             MTXMODE_NEW);
            Matrix_RotateZYX(0, play->gameplayFrames * 1000, 0, MTXMODE_APPLY);
            Matrix_Scale(0.2f, 0.2f, 0.2f, MTXMODE_APPLY);
        }

        // Draw fake item model.
        if (this->getItemEntry.drawFunc != NULL) {
            this->getItemEntry.drawFunc(play, &this->getItemEntry);
        } else {
            GetItem_Draw(play, drawIdPlusOne - 1);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void Player_DrawGetItemImpl(PlayState* play, Player* this, Vec3f* refPos, s32 drawIdPlusOne) {
    f32 height = (this->exchangeItemId != EXCH_ITEM_NONE) ? 6.0f : 14.0f;

    // FD (2026-07-12) #10: no item-height lift for FD. An earlier +30 (to reach the head-high camera aim) made the
    // item float too high above him. The real fix is lowering the get-item CAMERA aim for FD (Camera_KeepOn3 aims at
    // playerPos.y + Player_GetHeight() = 124 for FD, way above his held item), handled in z_camera.c so the shot
    // frames his hands + head like adult/child instead of a strip of ceiling. The item stays at its normal
    // hand-relative position.

    OPEN_DISPS(play->state.gfxCtx);

    gSegments[6] = VIRTUAL_TO_PHYSICAL(this->giObjectSegment);

    gSPSegment(POLY_OPA_DISP++, 0x06, this->giObjectSegment);
    gSPSegment(POLY_XLU_DISP++, 0x06, this->giObjectSegment);

    Matrix_Translate(refPos->x + (3.3f * Math_SinS(this->actor.shape.rot.y)), refPos->y + height,
                     refPos->z + ((3.3f + (IREG(90) / 10.0f)) * Math_CosS(this->actor.shape.rot.y)), MTXMODE_NEW);
    Matrix_RotateZYX(0, play->gameplayFrames * 1000, 0, MTXMODE_APPLY);
    {
        f32 giScale = (gSaveContext.linkAge == LINK_AGE_DEITY) ? 0.25f : 0.2f; // FD: bigger hands -> slightly larger GI
        Matrix_Scale(giScale, giScale, giScale, MTXMODE_APPLY);
    }

    if (this->getItemEntry.modIndex == MOD_RANDOMIZER && this->getItemEntry.getItemId == RG_ICE_TRAP) {
        Player_DrawGetItemIceTrap(play, this, refPos, drawIdPlusOne, height);
    } else if (this->getItemEntry.modIndex == MOD_RANDOMIZER && this->getItemEntry.getItemId == RG_TRIFORCE_PIECE) {
        Randomizer_DrawTriforcePieceGI(play, this->getItemEntry);
    } else if (this->getItemEntry.drawFunc != NULL) {
        this->getItemEntry.drawFunc(play, &this->getItemEntry);
    } else {
        GetItem_Draw(play, drawIdPlusOne - 1);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void Player_DrawGetItem(PlayState* play, Player* this) {
    // if (!this->giObjectLoading || !osRecvMesg(&this->giObjectLoadQueue, NULL, OS_MESG_NOBLOCK)) // OTRTODO: Do
    // something about osRecvMesg here...
    {
        this->giObjectLoading = false;
        Player_DrawGetItemImpl(play, this, &sGetItemRefPos, ABS(this->unk_862));
    }
}

void func_80090A28(Player* this, Vec3f* vecs) {
    D_8012608C.x = D_80126080.x;

    if (this->unk_845 >= 3) {
        this->unk_845 += 1;
        D_8012608C.x *= 1.0f + ((9 - this->unk_845) * 0.1f);
    }

    D_8012608C.x += 1200.0f;
    D_80126098.x = D_8012608C.x;

    Matrix_MultVec3f(&D_80126080, &vecs[0]);
    Matrix_MultVec3f(&D_8012608C, &vecs[1]);
    Matrix_MultVec3f(&D_80126098, &vecs[2]);
}

void Player_DrawHookshotReticle(PlayState* play, Player* this, f32 hookshotRange) {
    static Vec3f D_801260C8 = { -500.0f, -100.0f, 0.0f };
    CollisionPoly* colPoly;
    s32 bgId;
    Vec3f hookshotStart;
    Vec3f hookshotEnd;
    Vec3f firstHit;
    Vec3f sp68;
    f32 sp64;

    D_801260C8.z = 0.0f;
    Matrix_MultVec3f(&D_801260C8, &hookshotStart);
    D_801260C8.z = hookshotRange;
    Matrix_MultVec3f(&D_801260C8, &hookshotEnd);

    if (BgCheck_AnyLineTest3(&play->colCtx, &hookshotStart, &hookshotEnd, &firstHit, &colPoly, 1, 1, 1, 1, &bgId)) {
        OPEN_DISPS(play->state.gfxCtx);

        OVERLAY_DISP = Gfx_SetupDL(OVERLAY_DISP, 0x07);

        SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &firstHit, &sp68, &sp64);

        const f32 sp60 = (sp64 < 200.0f) ? 0.08f : (sp64 / 200.0f) * 0.08f;

        Matrix_Translate(firstHit.x, firstHit.y, firstHit.z, MTXMODE_NEW);
        Matrix_Scale(sp60, sp60, sp60, MTXMODE_APPLY);

        gSPMatrix(OVERLAY_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        if (GameInteractor_Should(VB_TARGETABLE_HOOKSHOT_RETICLE, true, colPoly, bgId)) {
            gSPSegment(OVERLAY_DISP++, 0x06, play->objectCtx.status[this->actor.objBankIndex].segment);
            gSPDisplayList(OVERLAY_DISP++, gLinkAdultHookshotReticleDL);
        }

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

Vec3f D_801260D4 = { 1100.0f, -700.0f, 0.0f };

f32 sMeleeWeaponLengths[] = {
    0.0f, 4000.0f, 3000.0f, 5500.0f, 0.0f, 2500.0f,
};

f32 sSwordTypes[] = {
    TRAIL_TYPE_REST,           TRAIL_TYPE_MASTER_SWORD, TRAIL_TYPE_KOKIRI_SWORD,
    TRAIL_TYPE_BIGGORON_SWORD, TRAIL_TYPE_REST,         TRAIL_TYPE_HAMMER,
};

Gfx* sBottleDLists[] = { gLinkAdultBottleDL, gLinkChildBottleDL,
                         gLinkFierceDeityBottleDL }; // FD (2026-07-11): +DEITY

Color_RGB8 sBottleColors[] = {
    { 255, 255, 255 }, { 80, 80, 255 },   { 255, 100, 255 }, { 0, 0, 255 }, { 255, 0, 255 },
    { 255, 0, 255 },   { 200, 200, 100 }, { 255, 0, 0 },     { 0, 0, 255 }, { 0, 255, 0 },
    { 255, 255, 255 }, { 255, 255, 255 }, { 80, 80, 255 },
};

Vec3f sLeftHandArrowVec3 = { 398.0f, 1419.0f, 244.0f };

BowStringData sBowStringData[] = {
    { gLinkAdultBowStringDL, { 0.0f, -360.4f, 0.0f } },        // bow
    { gLinkChildSlingshotStringDL, { 606.0f, 236.0f, 0.0f } }, // slingshot
    { gLinkAdultBowStringDL, { 0.0f, -360.4f, 0.0f } },        // FD (2026-07-11): deity = adult
};

Vec3f sRightHandLimbModelShieldQuadVertices[] = {
    { -4500.0f, -3000.0f, -600.0f },
    { 1500.0f, -3000.0f, -600.0f },
    { -4500.0f, 3000.0f, -600.0f },
    { 1500.0f, 3000.0f, -600.0f },
};

Vec3f D_80126184 = { 100.0f, 1500.0f, 0.0f };
Vec3f D_80126190 = { 100.0f, 1640.0f, 0.0f };

Vec3f sSheathLimbModelShieldQuadVertices[] = {
    { -3000.0f, -3000.0f, -900.0f },
    { 3000.0f, -3000.0f, -900.0f },
    { -3000.0f, 3000.0f, -900.0f },
    { 3000.0f, 3000.0f, -900.0f },
};

Vec3f sSheathLimbModelShieldOnBackPos = { 630.0f, 100.0f, -30.0f };
Vec3s sSheathLimbModelShieldOnBackZyxRot = { 0, 0, 0x7FFF };

Vec3f sLeftRightFootLimbModelFootPos[] = {
    { 200.0f, 300.0f, 0.0f },
    { 200.0f, 200.0f, 0.0f },
    { 200.0f, 300.0f, 0.0f }, // FD (2026-07-11): deity
};

// FD (2026-07-11): Fierce Deity sword-beam gate (RE z_player_lib.c:1712 Player_CanUseSwordBeams +
// Player_CheckZTargeting2). FD (or anyone holding the FD sword) may fire a sword beam whenever Z-targeting.
// SoH's func_8008E9C4 (the hostile-update Z-target check) is Player_IsZTargetingWithHostileUpdate here.
extern int Player_IsZTargetingWithHostileUpdate(Player* this);

s32 Player_CheckZTargeting2(Player* this) {
    // RE flags PLAYER_STATE1_16/17/30 -> SoH FRIENDLY_ACTOR_FOCUS / PARALLEL / LOCK_ON_FORCED_TO_RELEASE.
    if (this->stateFlags1 &
        (PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL | PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE)) {
        return 1;
    }
    return Player_IsZTargetingWithHostileUpdate(this);
}

u8 Player_CanUseSwordBeams(Player* this) {
    if ((LINK_IS_DEITY || this->heldItemId == ITEM_SWORD_DEITY) && (Player_CheckZTargeting2(this))) {
        return 1;
    }
    return 0;
}

// FD (2026-07-11) bug 7: draw the transform mask HELD IN LINK'S HAND during the put-on / take-off animation,
// before it snaps onto the face (RE z_player_lib.c Player_DrawMaskInHand ~661, called from the L_HAND limb in
// Player_PostLimbDrawGameplay). Donning (human, cl_setmask frames 8-11) shows the TARGET form's mask; removing
// (deity, pz_maskoffstart) shows the PREVIOUS form's mask. FD is the only transform form in this port, so both
// resolve to the resident gFierceDeityMaskDL (gameplay_keep / fd.o2r), fetched via ResourceMgr exactly like the
// on-face draw -- no seg-0xA object load. NULL-guarded so an absent resource can never crash.
// FD (2026-07-12) ★★ROOT-CAUSE FIX for the malformed transform mask: the mask-cutscene animation symbols
// (gPlayerAnim_cl_setmask/cl_setmaskend/cl_maskoff) are declared `static const char[]` in object_link_deity.h, which
// is #included by BOTH z_player.c (where the anim is SET) and this file (where the mask draws are GATED). Because
// `static` = internal linkage, each TU gets its OWN copy at a DIFFERENT address. `skelAnime.animation` stores the
// pointer from z_player.c's copy, but the draw gates here compared against THIS file's copy -> the pointers never
// matched -> every mask gate failed for BOTH ages (held never drew, on-face drew from frame 0 over Link's human
// face = the z-fighting mess, scream never swapped). Fix = compare by PATH STRING CONTENT, exactly what authoritative
// 2ship does (BEN_ANIM_EQUAL / strcmp) -- immune to the per-TU address difference. `skelAnime.animation` holds the
// OTR path string during the cutscene; the OTRSigCheck guard avoids a strcmp on a raw (non-OTR) animation header.
static s32 Player_AnimIsByName(SkelAnime* s, const char* animPath) {
    return (s->animation != NULL) && (animPath != NULL) && (ResourceMgr_OTRSigCheck((void*)s->animation) != 0) &&
           (strcmp((const char*)s->animation, animPath) == 0);
}

static void Player_DrawMaskInHand(PlayState* play, Player* this) {
    // FD (2026-07-12) ★HELD-MASK ROOT CAUSE (2ship parity audit): the "smeared on the face" bug was the fork adding
    // gPlayerAnim_pz_maskoffstart to `removing`. MM/2ship/RE use ONLY cl_maskoff for removal (2ship z_player_lib.c
    // :3668, fd_build:662). pz_maskoffstart fires while Link is still a NON-HUMAN FORM, so the current limb matrix is
    // the FORM skeleton's hand -- not the human hand the MM Translate(-323.67,412.15,-969.96) constant was authored
    // for -- so the mask lands off toward the face. Dropping pz_maskoffstart makes the held draw byte-match MM/2ship,
    // so it can be re-enabled (the earlier default-off cvar gate is removed).
    s32 removing = Player_AnimIsByName(&this->skelAnime, gPlayerAnim_cl_maskoff);
    s32 form = removing ? this->transformPreviousForm : this->transformTargetForm;
    f32 donFrame;

    if (form != LINK_AGE_DEITY) {
        return; // only the Fierce Deity mask has an in-hand model in this port
    }

    // FD (2026-07-12) ★HELD-MASK FIX: use the RE's/MM's EXACT narrow cl_setmask [8,12) window. An earlier widening
    // to [4,20) OVERLAPPED the on-face draw (which fires at curFrame >= 12): during [12,20) BOTH gFierceDeityMaskDL
    // copies drew -- the on-face one off the HEAD limb and the held one off the L_HAND limb -- and because Link's
    // hand is raised to his face during those don frames, the held copy Z-fought ON TOP of the face (emitted after
    // the face in the POLY_OPA stream since L_HAND=0x10 draws after HEAD=0x0B). That is the reported "mask distorts
    // Link's face texture and never appears in-hand" bug. RE keeps held [8,12) and on-face [12,inf) mutually
    // EXCLUSIVE (fd_build z_player_lib.c:675 vs :2212) so exactly one mask draws per frame. Revert shows it whole.
    donFrame = this->skelAnime.curFrame;
    if (removing ||
        (LINK_IS_HUMAN && Player_AnimIsByName(&this->skelAnime, gPlayerAnim_cl_setmask) &&
         (donFrame >= 8.0f) && (donFrame < 12.0f))) {
        OPEN_DISPS(play->state.gfxCtx);
        // FD (2026-07-12) ★ROOT-CAUSE FIX: set up the standard opaque render state, exactly like the WORKING
        // get-item mask draw (FierceDeity_DrawGiMask). Without it, the mask DL inherits the skeleton's skin
        // RSP/RDP pipeline state (segments 0x08/0x09, skin combiner) and renders INVISIBLE. This was the real
        // reason the held/on-face/scream masks never appeared (not the DL bake or the frame window).
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        Matrix_Push();
        // RE hand-hold transform (MM D_801C0970..): Player_DrawMaskInHand Matrix_Translate + MatrixMM_RotateZYX.
        Matrix_Translate(-323.67f, 412.15f, -969.96f, MTXMODE_APPLY);
        Matrix_RotateZYX(-0x32BE, -0x50DE, -0x7717, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        // FD (2026-07-12) #2: pass the __OTR__ path DIRECTLY to gSPDisplayList (LUS resolves it), exactly like
        // the working get-item mask draw FierceDeity_DrawGiMask. ResourceMgr_LoadGfxByName returned a bad/NULL
        // pointer for these DLs (the held mask never drew), so the indirection is removed.
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gFierceDeityMaskDL);
        Matrix_Pop();
        CLOSE_DISPS(play->state.gfxCtx);
    }
}

// FD (2026-07-26) Bonus Settings "Bunny Hood Fit": the child Bunny Hood self-loads seg-0x0D slot 7 (the HEAD
// limb's matrix). On the taller Adult / Fierce Deity heads it sinks in. We build a raised copy of slot 7 in
// the HEAD-limb draw below -- WHILE the interpolation-tracked head matrix is live on the stack, so the raised
// matrix interpolates every frame like the skeleton does (a hand-built matrix from a snapshot jitters). The
// result is left in gPlayerMaskFitMtxSeg (an 8-Mtx array whose slot 7 the mask DL loads), or NULL for none.
// Only the Bunny Hood is corrected; the 7 face masks already sit on the face. Child is never touched.
// Values are head-local model units (same space as the hood verts); local -Y is up. Tune from in-game feedback.
Mtx* gPlayerMaskFitMtxSeg;
#define MASKFIT_BUNNY_ADULT_RAISE 220.0f // head-local units the Bunny Hood lifts on Adult Link
#define MASKFIT_BUNNY_DEITY_RAISE 100.0f // head-local units the Bunny Hood lifts on Fierce Deity
#define MASKFIT_BUNNY_ADULT_PITCH 0x0900 // binang backward tilt on Adult Link (~12.7 deg, tucks the eyes back)
#define MASKFIT_BUNNY_DEITY_PITCH 0x0900 // binang backward tilt on Fierce Deity (~12.7 deg, tucks the eyes back)
#define MASKFIT_BUNNY_ADULT_SCALE 1.0f   // uniform grow of the hood on Adult Link (1.0 = child size)
#define MASKFIT_BUNNY_DEITY_SCALE 1.0f   // uniform grow of the hood on Fierce Deity

// Dropdown BonusSettings.MaskFit: 0 Off / 1 Adult / 2 Fierce Deity / 3 Both (default). Returns true (and the
// head-local raise, backward pitch in binang, and uniform scale) when the current form's Bunny Hood should be
// corrected.
static s32 Player_GetBunnyHoodFit(Player* this, f32* raiseY, s16* pitch, f32* scale) {
    s32 mode = CVarGetInteger(CVAR_ENHANCEMENT("BonusSettings.MaskFit"), 3);
    s32 formOn;

    if ((mode == 0) || (this->currentMask != PLAYER_MASK_BUNNY)) {
        return false;
    }
    if (LINK_IS_ADULT) {
        formOn = (mode == 1) || (mode == 3);
        *raiseY = -MASKFIT_BUNNY_ADULT_RAISE; // -Y = up
        *pitch = MASKFIT_BUNNY_ADULT_PITCH;
        *scale = MASKFIT_BUNNY_ADULT_SCALE;
    } else if (LINK_IS_DEITY) {
        formOn = (mode == 2) || (mode == 3);
        *raiseY = -MASKFIT_BUNNY_DEITY_RAISE;
        *pitch = MASKFIT_BUNNY_DEITY_PITCH;
        *scale = MASKFIT_BUNNY_DEITY_SCALE;
    } else {
        return false; // child (or anything else) keeps vanilla placement
    }
    return formOn;
}

void Player_PostLimbDrawGameplay(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    Player* this = (Player*)thisx;

    // FD (2026-07-11) Task 1: the on-face transform mask (gFierceDeityMaskDL / gGiFierceDeityMaskFaceDL) is now
    // drawn on PLAYER_LIMB_HEAD during the animated mask cutscene, positioned by this->transformMatrixModifiers
    // (RE z_player.c Player_PostLimbDrawGameplay, matrix emitted to POLY_OPA_DISP). See the PLAYER_LIMB_HEAD block.

    if (*dList != NULL) {
        Matrix_MultVec3f(&sZeroVec, D_80160000);
    }

    // FD (2026-07-13) SELF-CONTAINED ocarina: FD's ocarina R_HAND slot is the BARE FD hand; draw the correct ocarina
    // (Fairy vs OoT, extracted from base geometry into fd.o2r) on top, at the hand's limb matrix -- exactly where the
    // base ocarina mesh is authored. Works without the alt-asset customequipment pack, unlike adult/child. Only the
    // near LOD's slot is the ocarina hand; the far slot is the same bare hand, so this draws for both.
    if ((limbIndex == PLAYER_LIMB_R_HAND) && LINK_IS_DEITY &&
        (this->rightHandType == PLAYER_MODELTYPE_RH_OCARINA)) {
        OPEN_DISPS(play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, (INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY)
                                            ? (Gfx*)gFdFairyOcarinaDL
                                            : (Gfx*)gFdOotOcarinaDL);
        CLOSE_DISPS(play->state.gfxCtx);
    }

    if (limbIndex == PLAYER_LIMB_L_HAND) {
        MtxF sp14C;
        Actor* hookedActor;

        Math_Vec3f_Copy(&this->leftHandPos, D_80160000);

        // FD (2026-07-11): keep the melee-weapon tip/base tracking the blade while FD Z-targets but is NOT
        // swinging, so Player_FierceDeityParticles has fresh blade positions to sparkle from (RE
        // z_player_lib.c:1728). Mirrors the swing-time func_80090480 update but with meleeWeaponState == 0.
        if ((Player_CanUseSwordBeams(this) == 1) && (this->meleeWeaponState == 0)) {
            Vec3f sp124b[3];
            Vec3f newBasePos[3];

            if (Player_HoldsBrokenKnife(this)) {
                D_80126080.x = 1500.0f;
            } else {
                D_80126080.x = sMeleeWeaponLengths[Player_GetMeleeWeaponHeld(this)];
            }
            func_80090A28(this, sp124b);
            Matrix_MultVec3f(&D_801260A4[0], &newBasePos[0]);
            Matrix_MultVec3f(&D_801260A4[1], &newBasePos[1]);
            Matrix_MultVec3f(&D_801260A4[2], &newBasePos[2]);
            func_80090480(play, NULL, &this->meleeWeaponInfo[0], &sp124b[0], &newBasePos[0]);
        }

        // FD (2026-07-11) bug 7: draw the held transform mask in-hand during the mask cutscene (RE call site
        // z_player_lib.c:1997). No-op unless a mask-transform anim is mid-play (guards inside).
        Player_DrawMaskInHand(play, this);

        if (this->itemAction == PLAYER_IA_DEKU_STICK) {
            Vec3f sp124[3];

            OPEN_DISPS(play->state.gfxCtx);

            if (this->actor.scale.y >= 0.0f) {
                D_80126080.x = this->unk_85C * 5000.0f;
                func_80090A28(this, sp124);
                if (this->meleeWeaponState != 0) {
                    EffectBlure_ChangeType(Effect_GetByIndex(this->meleeWeaponEffectIndex), TRAIL_TYPE_STICK);
                    func_800906D4(play, this, sp124);
                } else {
                    Math_Vec3f_Copy(&this->meleeWeaponInfo[0].tip, &sp124[0]);
                }
            }

            Matrix_Translate(-428.26f, 267.2f, -33.82f, MTXMODE_APPLY);
            Matrix_RotateZYX(-0x8000, 0, 0x4000, MTXMODE_APPLY);
            Matrix_Scale(1.0f, this->unk_85C, 1.0f, MTXMODE_APPLY);

            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, gLinkChildLinkDekuStickDL);

            CLOSE_DISPS(play->state.gfxCtx);
        } else if ((this->actor.scale.y >= 0.0f) && (this->meleeWeaponState != 0)) {
            Vec3f spE4[3];

            if (Player_HoldsBrokenKnife(this)) {
                D_80126080.x = 1500.0f;
            } else {
                D_80126080.x = sMeleeWeaponLengths[Player_GetMeleeWeaponHeld(this)];
                EffectBlure_ChangeType(Effect_GetByIndex(this->meleeWeaponEffectIndex),
                                       sSwordTypes[Player_GetMeleeWeaponHeld(this)]);
            }

            func_80090A28(this, spE4);
            func_800906D4(play, this, spE4);
        } else if ((*dList != NULL) && (this->leftHandType == PLAYER_MODELTYPE_LH_BOTTLE)) {
            Color_RGB8* bottleColor = &sBottleColors[Player_ActionToBottle(this, this->itemAction)];

            OPEN_DISPS(play->state.gfxCtx);

            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetEnvColor(POLY_XLU_DISP++, bottleColor->r, bottleColor->g, bottleColor->b, 0);
            gSPDisplayList(POLY_XLU_DISP++, sBottleDLists[(gSaveContext.linkAge)]);

            CLOSE_DISPS(play->state.gfxCtx);
        }

        if (this->actor.scale.y >= 0.0f) {
            if (!Player_HoldsHookshot(this) && ((hookedActor = this->heldActor) != NULL)) {
                if (this->stateFlags1 & PLAYER_STATE1_READY_TO_FIRE) {
                    Matrix_MultVec3f(&sLeftHandArrowVec3, &hookedActor->world.pos);
                    Matrix_RotateZYX(0x69E8, -0x5708, 0x458E, MTXMODE_APPLY);
                    Matrix_Get(&sp14C);
                    Matrix_MtxFToYXZRotS(&sp14C, &hookedActor->world.rot, 0);
                    hookedActor->shape.rot = hookedActor->world.rot;
                } else if (this->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) {
                    Vec3s spB8;

                    Matrix_Get(&sp14C);
                    Matrix_MtxFToYXZRotS(&sp14C, &spB8, 0);

                    if (hookedActor->flags & ACTOR_FLAG_CARRY_X_ROT_INFLUENCE) {
                        hookedActor->world.rot.x = hookedActor->shape.rot.x = spB8.x - this->unk_3BC.x;
                    } else {
                        hookedActor->world.rot.y = hookedActor->shape.rot.y = this->actor.shape.rot.y + this->unk_3BC.y;
                    }
                }
            } else {
                Matrix_Get(&this->mf_9E0);
                Matrix_MtxFToYXZRotS(&this->mf_9E0, &this->unk_3BC, 0);
            }
        }
    } else if (limbIndex == PLAYER_LIMB_R_HAND) {
        Actor* heldActor = this->heldActor;

        if (this->rightHandType == PLAYER_MODELTYPE_RH_FF) {
            Matrix_Get(&this->shieldMf);
        } else if ((this->rightHandType == PLAYER_MODELTYPE_RH_BOW_SLINGSHOT) ||
                   (this->rightHandType == PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2)) {
            s32 stringModelToUse = gSaveContext.linkAge;
            if (CVarGetInteger(CVAR_ENHANCEMENT("BowSlingshotAmmoFix"), 0) ||
                CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0)) {
                stringModelToUse = Player_HoldsSlingshot(this);
            }
            BowStringData* stringData = &sBowStringData[stringModelToUse];

            OPEN_DISPS(play->state.gfxCtx);

            Matrix_Push();
            Matrix_Translate(stringData->pos.x, stringData->pos.y, stringData->pos.z, MTXMODE_APPLY);

            if ((this->stateFlags1 & PLAYER_STATE1_READY_TO_FIRE) && (this->unk_860 >= 0) && (this->unk_834 <= 10)) {
                Vec3f sp90;
                f32 distXYZ;

                Matrix_MultVec3f(&sZeroVec, &sp90);
                distXYZ = Math_Vec3f_DistXYZ(D_80160000, &sp90);

                this->unk_858 = distXYZ - 3.0f;
                if (distXYZ < 3.0f) {
                    this->unk_858 = 0.0f;
                } else {
                    this->unk_858 *= 1.6f;
                    if (this->unk_858 > 1.0f) {
                        this->unk_858 = 1.0f;
                    }
                }

                this->unk_85C = -0.5f;
            }

            Matrix_Scale(1.0f, this->unk_858, 1.0f, MTXMODE_APPLY);

            if (!LINK_IS_ADULT) {
                Matrix_RotateZ(this->unk_858 * -0.2f, MTXMODE_APPLY);
            }

            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_XLU_DISP++, stringData->dList);

            Matrix_Pop();

            CLOSE_DISPS(play->state.gfxCtx);
        } else if ((this->actor.scale.y >= 0.0f) && (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD)) {
            Matrix_Get(&this->shieldMf);
            Player_UpdateShieldCollider(play, this, &this->shieldQuad, sRightHandLimbModelShieldQuadVertices);
        }

        if (this->actor.scale.y >= 0.0f) {
            if (GameInteractor_Should(VB_DRAW_ADDITIONAL_RETICLES,
                                      (this->heldItemAction == PLAYER_IA_HOOKSHOT) ||
                                          (this->heldItemAction == PLAYER_IA_LONGSHOT),
                                      this)) {
                Matrix_MultVec3f(&D_80126184, &this->unk_3C8);

                if (heldActor != NULL) {
                    MtxF sp44;
                    s32 pad;

                    Matrix_MultVec3f(&D_80126190, &heldActor->world.pos);
                    Matrix_RotateZYX(0, -0x4000, -0x4000, MTXMODE_APPLY);
                    Matrix_Get(&sp44);
                    Matrix_MtxFToYXZRotS(&sp44, &heldActor->world.rot, 0);
                    heldActor->shape.rot = heldActor->world.rot;

                    if (func_8002DD78(this) != 0) {
                        Matrix_Translate(500.0f, 300.0f, 0.0f, MTXMODE_APPLY);
                        Player_DrawHookshotReticle(
                            play, this,
                            ((this->heldItemAction == PLAYER_IA_HOOKSHOT) ? 38600.0f : 77600.0f) *
                                CVarGetFloat(CVAR_CHEAT("HookshotReachMultiplier"), 1.0f));
                    }
                }
            }

            if ((this->unk_862 != 0) || ((func_8002DD6C(this) == 0) && (heldActor != NULL))) {
                if (!(this->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && (this->unk_862 != 0) &&
                    (this->exchangeItemId != EXCH_ITEM_NONE)) {
                    Math_Vec3f_Copy(&sGetItemRefPos, &this->leftHandPos);
                } else {
                    sGetItemRefPos.x = (this->bodyPartsPos[15].x + this->leftHandPos.x) * 0.5f;
                    sGetItemRefPos.y = (this->bodyPartsPos[15].y + this->leftHandPos.y) * 0.5f;
                    sGetItemRefPos.z = (this->bodyPartsPos[15].z + this->leftHandPos.z) * 0.5f;
                }

                if (this->unk_862 == 0) {
                    Math_Vec3f_Copy(&heldActor->world.pos, &sGetItemRefPos);
                }
            }
        }
    } else if (this->actor.scale.y >= 0.0f) {
        if (limbIndex == PLAYER_LIMB_SHEATH) {
            if ((this->rightHandType != PLAYER_MODELTYPE_RH_SHIELD) &&
                (this->rightHandType != PLAYER_MODELTYPE_RH_FF)) {
                if (Player_IsChildWithHylianShield(this)) {
                    Player_UpdateShieldCollider(play, this, &this->shieldQuad, sSheathLimbModelShieldQuadVertices);
                }

                Matrix_TranslateRotateZYX(&sSheathLimbModelShieldOnBackPos, &sSheathLimbModelShieldOnBackZyxRot);
                Matrix_Get(&this->shieldMf);
            }
        } else if (limbIndex == PLAYER_LIMB_HEAD) {
            // FD (2026-07-11) Task 1: draw the transform mask over the human face during the animated
            // mask-transform cutscene (RE z_player.c Player_PostLimbDrawGameplay, fd_build ~2204-2261). The
            // matrix is emitted to POLY_OPA_DISP -- NOT OVERLAY_DISP -- so the matrix and geometry stay in one
            // command stream (the verified RSP-hang crash fix). Driven by PLAYER_STATE3_TRANSFORMATION_MASK +
            // transformMatrixModifiers[2]/[3] (the squash), which Player_UpdateTransformationAnim ramps.
            // FD (2026-07-12) #5: the transform "blue vortex" -- a swirling CONICAL cloud model (gTransformEffectDL)
            // around the face during the close-up, tinted blue, alpha ramping with transformEventTimer2. MM DOES
            // have this model (an earlier "no swirl model" claim was wrong). It renders on BOTH transform and revert
            // (gated only by PLAYER_STATE3_TRANSFORMATION_MASK + the timer, not on age). The DL branches to segment
            // 0x0B for its two-tex cloud scroll (bound below via Gfx_TwoTexScrollEx). Drawn to POLY_XLU. Direct-path
            // gSPDisplayList -- valid once gTransformEffectDL is a COMPILED binary resource in fd.o2r (it was baked
            // as uncompiled XML before -> NULL -> nothing drew; the asset bake is being corrected).
            if ((this->stateFlags3 & PLAYER_STATE3_TRANSFORMATION_MASK) && (this->transformEventTimer2 != 0)) {
                static Vec3f sSwirlFaceOffset[LINK_AGE_MAX] = {
                    { -230.0f, -520.0f, 0.0f }, // adult (RE D_801C0E40 human offset)
                    { -230.0f, -520.0f, 0.0f }, // child
                    { 0.0f, 0.0f, 0.0f },       // deity
                };
                Vec3f* off = &sSwirlFaceOffset[gSaveContext.linkAge];

                OPEN_DISPS(play->state.gfxCtx);
                // FD (2026-07-12) ★ROOT-CAUSE FIX: clean XLU render-state setup (like the working two-tex-scroll
                // effect GetItem_DrawJewel in z_draw.c) so the cone doesn't inherit the skeleton's skin pipeline
                // state and render invisible. The 2-cycle + seg-0x0B setup below refines this clean base.
                Gfx_SetupDL_25Xlu(play->state.gfxCtx);
                Matrix_Push();
                // gTransformEffectDL is a 2-cycle effect (dual SetCombineLERP / SetRenderMode) that never sets the
                // cycle type itself (MM ran it inside a 2-cycle draw). Force 2-cycle so the combiner is well-formed.
                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
                // Bind seg 0x0B to the animated two-tex cloud scroll (RE sMaskEffectScroll {{-1,0,16,16},{1,-2,16,16}}).
                gSPSegment(POLY_XLU_DISP++, 0x0B,
                           Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, (0 - play->gameplayFrames) % 128, 0, 16, 16, 1,
                                              (play->gameplayFrames * 1) % 128, (play->gameplayFrames * 2) % 128, 16,
                                              16, -1, 0, 1, 2));
                Matrix_Translate(off->x, off->y, 0.0f, MTXMODE_APPLY);
                gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 255, (u8)this->transformEventTimer2);
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gTransformEffectDL);
                Matrix_Pop();
                CLOSE_DISPS(play->state.gfxCtx);
            }
            if ((this->stateFlags3 & PLAYER_STATE3_TRANSFORMATION_MASK) && LINK_IS_HUMAN &&
                (this->transformTargetForm == LINK_AGE_DEITY) &&
                ((!Player_AnimIsByName(&this->skelAnime, gPlayerAnim_cl_setmask)) ||
                 (this->skelAnime.curFrame >= 12.0f))) {
                // FD (2026-07-12) ★SCREAM-MASK PARITY (2ship): the on-face draw uses the calm gFierceDeityMaskDL, then
                // swaps to the open-mouth SCREAM model at the late frames -- MM/2ship's D_801C0B20[FD-1+4] swap: when
                // (cl_setmask curFrame >= 51) or cl_setmaskend (2ship z_player_lib.c:4049-4056). The scream model is
                // the face-space object_mask_boy_DL_000900 (gFierceDeityScreamMaskDL), imported from mm.o2r into
                // fd.o2r. Same head-limb matrix + squash as the calm mask; adult+child both use it (MM has no per-age
                // branch here -- only the human skeleton's limb matrix differs). (The old gGiFierceDeityMaskFaceDL was
                // the GET-ITEM pedestal model, wrong coordinate space -> invisible on the face.)
                s32 scream = ((Player_AnimIsByName(&this->skelAnime, gPlayerAnim_cl_setmask) &&
                               (this->skelAnime.curFrame >= 51.0f)) ||
                              Player_AnimIsByName(&this->skelAnime, gPlayerAnim_cl_setmaskend));
                OPEN_DISPS(play->state.gfxCtx);
                // Opaque render-state setup (like the working get-item mask draw) so the DL doesn't inherit the
                // skeleton skin pipeline state and render invisible.
                Gfx_SetupDL_25Opa(play->state.gfxCtx);
                // Human on-face offset is {0,0} in MM's D_801C0E04 (the FD mask DL is authored on the face).
                Matrix_Push();
                Matrix_Scale(1.0f, 1.0f - this->transformMatrixModifiers[3], 1.0f - this->transformMatrixModifiers[2],
                             MTXMODE_APPLY);
                gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx),
                          G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                Matrix_Pop();
                gSPDisplayList(POLY_OPA_DISP++, scream ? (Gfx*)gFierceDeityScreamMaskDL : (Gfx*)gFierceDeityMaskDL);
                CLOSE_DISPS(play->state.gfxCtx);
            }
            Matrix_MultVec3f(&D_801260D4, &this->actor.focus.pos);
            // FD (2026-07-26) Bonus Settings "Bunny Hood Fit": this HEAD-limb matrix is exactly what a worn mask
            // self-loads (seg-0x0D slot 7). While it is live on the stack, build a raised/scaled copy into an
            // 8-Mtx array's slot 7 for Player_DrawGameplay to bind. Building it here (not from a snapshot) keeps
            // it on the same per-frame interpolation path as the skeleton, so the hood doesn't jitter in motion.
            // Push/Pop is required: the head's child (hat) is drawn next off this same matrix.
            {
                f32 fitRaiseY, fitScale;
                s16 fitPitch;

                gPlayerMaskFitMtxSeg = NULL;
                if (Player_GetBunnyHoodFit(this, &fitRaiseY, &fitPitch, &fitScale)) {
                    Mtx* fitMtx = Graph_Alloc(play->state.gfxCtx, 8 * sizeof(Mtx));

                    Matrix_Push();
                    Matrix_Translate(0.0f, fitRaiseY, 0.0f, MTXMODE_APPLY);
                    if (fitPitch != 0) {
                        // Backward tilt is a pitch about world X, which in the head-local frame is local Z
                        // (head-local X maps to world Z, so RotateX would only roll the hood side to side).
                        Matrix_RotateZ(fitPitch * (M_PI / 0x8000), MTXMODE_APPLY);
                    }
                    if (fitScale != 1.0f) {
                        Matrix_Scale(fitScale, fitScale, fitScale, MTXMODE_APPLY);
                    }
                    MATRIX_TOMTX(&fitMtx[7]); // slot 7 == seg-0x0D offset 0x1C0, the slot the mask DL loads
                    Matrix_Pop();
                    gPlayerMaskFitMtxSeg = fitMtx;
                }
            }
        } else {
            Vec3f* vec = &sLeftRightFootLimbModelFootPos[(gSaveContext.linkAge)];

            Actor_SetFeetPos(&this->actor, limbIndex, PLAYER_LIMB_L_FOOT, vec, PLAYER_LIMB_R_FOOT, vec);
        }
    }
}

u32 func_80091738(PlayState* play, u8* segment, SkelAnime* skelAnime) {
    s16 linkObjectId = gLinkObjectIds[gSaveContext.linkAge];
    size_t size;
    void* ptr;

    size = gObjectTable[OBJECT_GAMEPLAY_KEEP].vromEnd - gObjectTable[OBJECT_GAMEPLAY_KEEP].vromStart;
    ptr = segment + 0x3800;
    DmaMgr_SendRequest1(ptr, gObjectTable[OBJECT_GAMEPLAY_KEEP].vromStart, size, __FILE__, __LINE__);

    size = gObjectTable[linkObjectId].vromEnd - gObjectTable[linkObjectId].vromStart;
    ptr = segment + 0x8800;
    DmaMgr_SendRequest1(ptr, gObjectTable[linkObjectId].vromStart, size, __FILE__, __LINE__);

    ptr = (void*)ALIGN16((intptr_t)ptr + size);

    gSegments[4] = VIRTUAL_TO_PHYSICAL(segment + 0x3800);
    gSegments[6] = VIRTUAL_TO_PHYSICAL(segment + 0x8800);

    // FD (2026-07-11) BUG 5: purge stale player skeleton registrations before the pause re-inits the model.
    //
    // ROOT CAUSE (a SoH divergence with no N64/RE analogue -- vanilla oot-master z_player_lib.c:1659 just calls
    // SkelAnime_InitLink here and is done). SoH's SkeletonPatcher keeps a GLOBAL list of every player SkelAnime
    // that ResourceMgr_LoadSkeletonByName has ever seen (soh/ResourceManagerHelpers.cpp:599), and
    // SkeletonPatcher::UpdateTunicSkeletons (soh/resource/type/Skeleton.cpp:136) re-points skelAnime->skeleton to
    // the matching base-age *tunic* skeleton for any entry whose registration PATH is gLinkChildSkel /
    // gLinkAdultSkel. That patch runs on the OnLinkSkeletonInit and OnLinkEquipmentChange hooks
    // (soh/Enhancements/cosmetics/CustomSkeletons.cpp:21-22).
    //
    // When a child/adult transforms into Fierce Deity, Player_ChangeAge swaps the drawn skeleton to
    // gLinkFierceDeitySkel but the ORIGINAL base-age registration made by Player_InitCommon is never removed
    // (RegisterSkeleton only ever pushes; nothing unregisters). Opening the pause calls SkelAnime_InitLink just
    // below, whose trailing GameInteractor_ExecuteOnLinkSkeletonInit runs UpdateTunicSkeletons, and that stale
    // base-age entry drags the player's -- and the freshly registered pause -- body limbs back to young/adult
    // Link. The FD hands + pelvis survive because they are chosen per-frame from the linkAge==LINK_AGE_DEITY DL
    // override (Player_OverrideLimbDrawPause), not from the skeleton; hence a young-Link body wearing FD hands
    // on the pause screen and, because this->skelAnime is patched in place, in-game after unpausing. A scene
    // reload's ResourceMgr_ClearSkeletons wipes the stale entry -- exactly why a level transition undoes it.
    //
    // Fierce Deity is never tunic-patched (IsLinkSkeletonPath excludes the FD path), so drop every registration
    // for the player's skelAnimes and for the pause skelAnime here. Their skeleton pointers already reference the
    // FD model and are left untouched, so nothing can drag them to a base age. Reverting FD->human re-registers
    // via Player_ChangeAge, restoring normal tunic patching. ResourceMgr_UnregisterSkeleton removes a single
    // entry per call, so loop to clear the duplicates that accumulate across repeated in-scene transforms.
    if (LINK_IS_DEITY) {
        Player* player = GET_PLAYER(play);
        s32 i;

        for (i = 0; i < 6; i++) {
            ResourceMgr_UnregisterSkeleton(&player->skelAnime);
            ResourceMgr_UnregisterSkeleton(&player->upperSkelAnime);
            ResourceMgr_UnregisterSkeleton(skelAnime);
        }
    }

    SkelAnime_InitLink(play, skelAnime, gPlayerSkelHeaders[gSaveContext.linkAge], &gPlayerAnim_link_normal_wait, 9, ptr,
                       ptr, PLAYER_LIMB_MAX);

    return size + 0x8800 + 0x90;
}

u8 sPauseModelGroupBySword[] = {
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_SWORD_KOKIRI
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_SWORD_MASTER
    PLAYER_MODELGROUP_BGS,              // PLAYER_SWORD_BIGGORON
};

s32 Player_OverrideLimbDrawPause(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* arg) {
    u8* playerSwordAndShield = arg;
    // SOH: Ensure positive value from playerSwordAndShield[] to avoid OOB array access.
    //      This can occur in the case where playerSwordAndShield[0] is PLAYER_SWORD_NONE
    u8 modelGroup =
        sPauseModelGroupBySword[playerSwordAndShield[0] > 0 ? playerSwordAndShield[0] - PLAYER_SWORD_KOKIRI : 0];
    s32 type;
    s32 dListOffset = 0;
    Gfx** dLists;

    // FD (2026-07-11) BUG A: `!LINK_IS_ADULT` here would route Fierce Deity (a non-adult form) to the CHILD
    // Hylian-shield hand model group. Gate on LINK_IS_CHILD so only true child uses the child-shield group;
    // deity is treated like adult (it uses the adult-proportioned skeleton).
    if ((modelGroup == PLAYER_MODELGROUP_SWORD_AND_SHIELD) && LINK_IS_CHILD &&
        (playerSwordAndShield[1] == PLAYER_SHIELD_HYLIAN)) {
        modelGroup = PLAYER_MODELGROUP_CHILD_HYLIAN_SHIELD;
    }

    if (limbIndex == PLAYER_LIMB_L_HAND) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_LEFT_HAND];
        sLeftHandType = type;

        // SOH: Handle unexpected swordless case. Previously OOB array access is avoided, but we want the
        //      hand model-type to be set to open (otherwise it is set to holding sword model-type)
        if (playerSwordAndShield[0] == PLAYER_SWORD_NONE) {
            type = PLAYER_MODELTYPE_LH_OPEN;
        }

        // FD (2026-07-11) BUG 13: the pause DL-group arrays were widened from a 2-form (adult,child) near/far
        // stride of 4 to a 3-form (adult,child,DEITY) near/far stride of NUM_DL_FORMS*2 == 6 (see
        // gPlayerLeftHandBgsDLs / sPlayerRightHandShieldDLs / sSheathWith*SwordDLs above). This broken-Biggoron
        // sub-block jump was left at the old literal 4, so it landed mid-block (into the deity near/far slots)
        // instead of the broken-knife block at +6. Use NUM_DL_FORMS*2, matching the in-game path
        // (Player_OverrideLimbDrawGameplayDefault, this file @ the `dLists += this->currentShield * (NUM_DL_FORMS
        // * 2)` sites) and the RE reference (fd_build z_player_lib.c:2333).
        if ((type == PLAYER_MODELTYPE_LH_BGS) && (gSaveContext.swordHealth <= 0.0f)) {
            dListOffset = NUM_DL_FORMS * 2;
        }

        // FD (2026-07-12): draw Fierce Deity's left hand + FD sword on the PAUSE equipment model. FD force-equips
        // ITEM_SWORD_DEITY on B, but the pause model group is derived from the EQUIPMENT sword slot (Master/
        // Biggoron), not the held item -- so for DEITY the L_HAND resolves to the LH_SWORD (or broken-BGS) DEITY
        // slot, both of which are gLinkFierceDeityEmptyDL -> empty hand, no blade. Force the LH_BGS FD-sword
        // sub-block (offset +12 = (NUM_DL_FORMS*2)*2 -> gLinkFierceDeityLeftHandHoldingSwordDL, the hand-with-blade
        // DL), mirroring the in-game Player_OverrideLimbDrawGameplayDefault L_HAND FD branch (~line 1505).
        if (LINK_IS_DEITY) {
            type = PLAYER_MODELTYPE_LH_BGS;
            dListOffset = (NUM_DL_FORMS * 2) * 2;
        }
    } else if (limbIndex == PLAYER_LIMB_R_HAND) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND];
        sRightHandType = type;
        // FD (2026-07-11) BUG 13: this is the exact leak in the report -- a human (child/adult) holding a sword
        // with a shield equipped. `playerSwordAndShield[1]` (the shield index) must jump whole per-shield blocks
        // of the widened sPlayerRightHandShieldDLs, whose stride is now NUM_DL_FORMS*2 (6), NOT the old 4 that the
        // stale `ptrSize = sizeof(uint32_t)` (== 4) preserved. With *4, child(index 1)+shield*4 landed on
        // gLinkFierceDeityRightHandDL (the deity slot of an adjacent shield block) -> FD's right hand appeared on
        // a young/adult Link. *6 (=NUM_DL_FORMS*2) selects the correct human hand. Matches the in-game
        // `dLists += this->currentShield * (NUM_DL_FORMS * 2)` and RE fd_build z_player_lib.c:2339.
        if (type == PLAYER_MODELTYPE_RH_SHIELD) {
            dListOffset = playerSwordAndShield[1] * (NUM_DL_FORMS * 2);
        }
    } else if (limbIndex == PLAYER_LIMB_SHEATH) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_SHEATH];
        // FD (2026-07-11) BUG 13: same widened-stride fix for the sheath+shield sub-block selection
        // (sSheathWithSwordDLs / sSheathWithoutSwordDLs are also NUM_DL_FORMS*2 stride). RE fd_build:2344.
        if ((type == PLAYER_MODELTYPE_SHEATH_18) || (type == PLAYER_MODELTYPE_SHEATH_19)) {
            dListOffset = playerSwordAndShield[1] * (NUM_DL_FORMS * 2);
        }
    } else if (limbIndex == PLAYER_LIMB_WAIST) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_WAIST];

        if (Player_IsCustomLinkModel()) {
            return 0;
        }
    } else {
        return 0;
    }

    dLists = &sPlayerDListGroups[type][gSaveContext.linkAge];
    *dList = dLists[dListOffset];

    return 0;
}

#include <overlays/actors/ovl_Demo_Effect/z_demo_effect.h>
void DemoEffect_DrawTriforceSpot(Actor* thisx, PlayState* play);

void Pause_DrawTriforceSpot(PlayState* play, s32 showLightColumn) {
    static DemoEffect triforce;
    static s16 rotation = 0;

    triforce.triforceSpot.crystalLightOpacity = 244;
    triforce.triforceSpot.triforceSpotOpacity = 249;
    triforce.triforceSpot.lightColumnOpacity = showLightColumn ? 244 : 0;
    triforce.triforceSpot.rotation = rotation;

    DemoEffect_DrawTriforceSpot(&triforce, play);

    rotation += 0x03E8;
}

void Player_DrawPauseImpl(PlayState* play, void* gameplayKeep, void* linkObject, SkelAnime* skelAnime, Vec3f* pos,
                          Vec3s* rot, f32 scale, s32 sword, s32 tunic, s32 shield, s32 boots, s32 width, s32 height,
                          Vec3f* eye, Vec3f* at, f32 fovy, void* colorFrameBuffer, void* depthFrameBuffer) {
    // Note: the viewport x and y values are overwritten below, before usage
    static Vp viewport = { (PAUSE_EQUIP_PLAYER_WIDTH / 2) << 2, (PAUSE_EQUIP_PLAYER_HEIGHT / 2) << 2, G_MAXZ / 2, 0,
                           (PAUSE_EQUIP_PLAYER_WIDTH / 2) << 2, (PAUSE_EQUIP_PLAYER_HEIGHT / 2) << 2, G_MAXZ / 2, 0 };
    static Lights1 lights1 = gdSPDefLights1(80, 80, 80, 255, 255, 255, 84, 84, -84);
    static Vec3f lightDir = { 89.8f, 0.0f, 89.8f };
    u8 playerSwordAndShield[2];
    Gfx* opaRef;
    Gfx* xluRef;
    u16 perspNorm;
    Mtx* perspMtx = Graph_Alloc(play->state.gfxCtx, sizeof(Mtx));
    Mtx* lookAtMtx = Graph_Alloc(play->state.gfxCtx, sizeof(Mtx));

    u8 mirrorWorldActive = CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0);

    OPEN_DISPS(play->state.gfxCtx);

    opaRef = POLY_OPA_DISP;
    POLY_OPA_DISP++;

    xluRef = POLY_XLU_DISP;
    POLY_XLU_DISP++;

    gSPDisplayList(WORK_DISP++, POLY_OPA_DISP);
    gSPDisplayList(WORK_DISP++, POLY_XLU_DISP);

    if (mirrorWorldActive) {
        gSPSetExtraGeometryMode(POLY_OPA_DISP++, G_EX_INVERT_CULLING);
        gSPSetExtraGeometryMode(POLY_XLU_DISP++, G_EX_INVERT_CULLING);
    }

    gSPSegment(POLY_OPA_DISP++, 0x00, NULL);

    gDPPipeSync(POLY_OPA_DISP++);

    gSPLoadGeometryMode(POLY_OPA_DISP++, 0);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_SHADE, G_CC_SHADE);
    gDPSetOtherMode(POLY_OPA_DISP++,
                    G_AD_DISABLE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE |
                        G_TD_CLAMP | G_TP_PERSP | G_CYC_FILL | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PIXEL | G_RM_NOOP | G_RM_NOOP2);

    // Also matches if some of the previous graphics commands are moved inside this block too. Possible macro?
    if (1) {
        s32 pad[2];

        gSPLoadGeometryMode(POLY_OPA_DISP++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    }

    gDPSetScissor(POLY_OPA_DISP++, G_SC_NON_INTERLACE, 0, 0, width, height);
    gSPClipRatio(POLY_OPA_DISP++, FRUSTRATIO_1);

    gDPSetColorImage(POLY_OPA_DISP++, G_IM_FMT_RGBA, G_IM_SIZ_16b, width, depthFrameBuffer);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_FILL);
    gDPSetRenderMode(POLY_OPA_DISP++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(POLY_OPA_DISP++, (GPACK_ZDZ(G_MAXFBZ, 0) << 16) | GPACK_ZDZ(G_MAXFBZ, 0));
    gDPFillRectangle(POLY_OPA_DISP++, 0, 0, width - 1, height - 1);

    gDPPipeSync(POLY_OPA_DISP++);

    gDPSetColorImage(POLY_OPA_DISP++, G_IM_FMT_RGBA, G_IM_SIZ_16b, width, colorFrameBuffer);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_FILL);
    gDPSetRenderMode(POLY_OPA_DISP++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(POLY_OPA_DISP++, (GPACK_RGBA5551(0, 0, 0, 1) << 16) | GPACK_RGBA5551(0, 0, 0, 1));
    gDPFillRectangle(POLY_OPA_DISP++, 0, 0, width - 1, height - 1);

    gDPPipeSync(POLY_OPA_DISP++);

    gDPSetDepthImage(POLY_OPA_DISP++, depthFrameBuffer);

    viewport.vp.vscale[0] = viewport.vp.vtrans[0] = width * ((1 << 2) / 2);
    viewport.vp.vscale[1] = viewport.vp.vtrans[1] = height * ((1 << 2) / 2);
    gSPViewport(POLY_OPA_DISP++, &viewport);

    guPerspective(perspMtx, &perspNorm, fovy, (f32)width / (f32)height, 10.0f, 4000.0f, 1.0f);

    gSPPerspNormalize(POLY_OPA_DISP++, perspNorm);
    gSPMatrix(POLY_OPA_DISP++, perspMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

    guLookAt(lookAtMtx, eye->x, eye->y, eye->z, at->x, at->y, at->z, 0.0f, 1.0f, 0.0f);

    gSPMatrix(POLY_OPA_DISP++, lookAtMtx, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);

    playerSwordAndShield[0] = sword;
    playerSwordAndShield[1] = shield;

    Matrix_SetTranslateRotateYXZ(
        pos->x - ((CVarGetInteger(CVAR_ENHANCEMENT("PauseMenuAnimatedLink"), 0) && LINK_AGE_IN_YEARS == YEARS_ADULT)
                      ? 25
                      : 0),
        pos->y - (CVarGetInteger(CVAR_GENERAL("PauseMenuAnimatedLinkTriforce"), 0) ? 16 : 0), pos->z, rot);
    Matrix_Scale(scale * (mirrorWorldActive ? -1 : 1), scale, scale, MTXMODE_APPLY);

    gSPSegment(POLY_OPA_DISP++, 0x04, gameplayKeep);
    gSPSegment(POLY_OPA_DISP++, 0x06, linkObject);

    gSPSetLights1(POLY_OPA_DISP++, lights1);

    func_80093C80(play);

    POLY_OPA_DISP = Gfx_SetFog2(POLY_OPA_DISP++, 0, 0, 0, 0, 997, 1000);

    func_8002EABC(pos, &play->view.eye, &lightDir, play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0C, gCullBackDList);

    Player_DrawImpl(play, skelAnime->skeleton, skelAnime->jointTable, skelAnime->dListCount, 0, tunic, boots, 0,
                    Player_OverrideLimbDrawPause, NULL, &playerSwordAndShield);

    if (CVarGetInteger(CVAR_GENERAL("PauseMenuAnimatedLinkTriforce"), 0)) {
        Matrix_SetTranslateRotateYXZ(pos->x - (LINK_AGE_IN_YEARS == YEARS_ADULT ? 25 : 0),
                                     pos->y + 280 + (LINK_AGE_IN_YEARS == YEARS_ADULT ? 48 : 0), pos->z, rot);
        Matrix_Scale(scale * (mirrorWorldActive ? -1 : 1), scale * 1, scale * 1, MTXMODE_APPLY);

        Pause_DrawTriforceSpot(play, 1);
    }

    if (mirrorWorldActive) {
        gSPClearExtraGeometryMode(POLY_OPA_DISP++, G_EX_INVERT_CULLING);
        gSPClearExtraGeometryMode(POLY_XLU_DISP++, G_EX_INVERT_CULLING);
    }

    gSPEndDisplayList(POLY_OPA_DISP++);
    gSPEndDisplayList(POLY_XLU_DISP++);

    gSPBranchList(opaRef, POLY_OPA_DISP);
    gSPBranchList(xluRef, POLY_XLU_DISP);

    CLOSE_DISPS(play->state.gfxCtx);
}

void Player_DrawPause(PlayState* play, u8* segment, SkelAnime* skelAnime, Vec3f* pos, Vec3s* rot, f32 scale, s32 sword,
                      s32 tunic, s32 shield, s32 boots) {
    Input* p1Input = &play->state.input[0];
    Vec3f eye = { 0.0f, 0.0f, -400.0f };
    Vec3f at = { 0.0f, 0.0f, 0.0f };
    Vec3s* destTable;
    Vec3s* srcTable;
    s32 i;
    bool canswitchrnd = false;

    gSegments[4] = VIRTUAL_TO_PHYSICAL(segment + 0x3800);
    gSegments[6] = VIRTUAL_TO_PHYSICAL(segment + 0x8800);

    uintptr_t* PauseMenuAnimSet[4] = { // IDLE                       // Two Handed                       // No shield //
                                       // Kid Hylian Shield
                                       gPlayerAnim_link_normal_wait, gPlayerAnim_link_fighter_wait_long,
                                       gPlayerAnim_link_normal_wait_free, gPlayerAnim_link_normal_wait_free
    };

    if (CVarGetInteger(CVAR_ENHANCEMENT("PauseMenuAnimatedLink"), 0) ||
        CVarGetInteger(CVAR_GENERAL("PauseMenuAnimatedLinkTriforce"), 0)) {
        uintptr_t anim = 0; // Initialise anim

        s16 EquipedStance;
        if (CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_BIGGORON) {
            EquipedStance = 1;
        } else if (CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SWORD_NONE) {
            EquipedStance = 2;
        } else if (CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SWORD_MASTER && LINK_AGE_IN_YEARS == YEARS_CHILD) {
            EquipedStance = 3;
        } else {
            // Link is idle so revert to 0
            EquipedStance = 0;
        }

        if (!CVarGetInteger(CVAR_GENERAL("PauseMenuAnimatedLinkTriforce"), 0)) {
            anim = PauseMenuAnimSet[EquipedStance];
        } else {
            anim = gPlayerAnim_link_magic_kaze2;
            sword = 0;
            shield = 0;
        }

        if (skelAnime->animation != anim) {
            LinkAnimation_Change(play, skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_LOOP, -6.0f);
        }

        LinkAnimation_Update(play, skelAnime);

        if (!LINK_IS_ADULT) {
            // Link is placed too far up by default when animating
            at.y += 60;
        }
    } else {

        // FD (2026-07-11) BUG A: the pause static-pose selector used `!LINK_IS_ADULT`, i.e. it treats every
        // non-adult form as CHILD. Fierce Deity (linkAge==LINK_AGE_DEITY==2) is `!LINK_IS_ADULT`, so it was
        // posed with the CHILD static pause pose (gLinkPauseChildJointTable) -- an aggressively hunched/short
        // young-Link stance -- which reads as "the pause shows young Link". The pause BODY is still the FD
        // skeleton (func_80091738 loads gPlayerSkelHeaders[DEITY]=&gLinkFierceDeitySkel correctly), but the
        // child pose collapses the FD skeleton into a child-like silhouette. FD uses the ADULT-proportioned
        // skeleton, so it must take the ADULT static pose. Gate on LINK_IS_CHILD so only true child gets the
        // child pose; adult AND deity fall through to the adult branch. Matches vanilla OoT z_player_lib.c:1832
        // for adult/child while adding the DEITY case (oot-master has no DEITY).
        if (LINK_IS_CHILD) {
            if (shield == PLAYER_SHIELD_DEKU) {
                srcTable = gLinkPauseChildDekuShieldJointTable;
            } else {
                srcTable = gLinkPauseChildJointTable;
            }
        } else {
            if (sword == PLAYER_SWORD_BIGGORON) {
                srcTable = gLinkPauseAdultBgsJointTable;
            } else if (shield != PLAYER_SHIELD_NONE) {
                srcTable = gLinkPauseAdultShieldJointTable;
            } else {
                srcTable = gLinkPauseAdultJointTable;
            }
        }

        srcTable = ResourceMgr_LoadArrayByNameAsVec3s(srcTable);
        Vec3s* ogSrcTable = srcTable;
        destTable = skelAnime->jointTable;
        for (i = 0; i < skelAnime->limbCount; i++) {
            *destTable++ = *srcTable++;
        }
        free(ogSrcTable);
    }

    Player_DrawPauseImpl(play, segment + 0x3800, segment + 0x8800, skelAnime, pos, rot, scale, sword, tunic, shield,
                         boots, PAUSE_EQUIP_PLAYER_WIDTH, PAUSE_EQUIP_PLAYER_HEIGHT, &eye, &at, 60.0f,
                         play->state.gfxCtx->curFrameBuffer,
                         play->state.gfxCtx->curFrameBuffer + (PAUSE_EQUIP_PLAYER_WIDTH * PAUSE_EQUIP_PLAYER_HEIGHT));
}
