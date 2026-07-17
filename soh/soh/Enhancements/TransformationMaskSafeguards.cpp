// FD (2026-07-12) #7/#8: Transformation-mask safeguard messages (aegiker fd_build parity).
//
// Registers the three custom Navi textboxes used by the "stuck" safeguards (#7, water turn-back prompt) and
// the "restricted actions" blocks (#8, Epona / Master Sword pedestal). They are served whenever their textId
// is opened -- the player code sets `naviTextId = -TEXT_...` to force Navi to talk them (see z_player.c), and
// the water two-choice "Yes" is handled in z_en_elf.c. Text is verbatim from fd_build/mod_assets/text
// (0x71B3-0x71B5), re-authored with the SoH CustomMessage codes (%c = light blue, %w = white, & = newline,
// \x1B%g..&..%w = two-choice).
#include <soh/OTRGlobals.h>
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"

#include <string>

extern "C" {
#include <z64.h>
}

#define TRANSFORM_MSG_TABLE_ID "TransformationMasks"

static void RegisterTransformationMaskSafeguardMessages() {
    CustomMessageManager* cmm = CustomMessageManager::Instance;
    cmm->AddCustomMessageTable(TRANSFORM_MSG_TABLE_ID);

    // #8 Epona mount block (RE 0x71B3).
    cmm->CreateMessage(TRANSFORM_MSG_TABLE_ID, TEXT_TRANSFORM_TOO_BIG,
                       CustomMessage("%cYou're too big!!", TEXTBOX_TYPE_BLACK));

    // #8 Master Sword pedestal block (RE 0x71B4).
    cmm->CreateMessage(TRANSFORM_MSG_TABLE_ID, TEXT_TRANSFORM_CANT_DO_THAT,
                       CustomMessage("%cYou can't do that in your&current form!", TEXTBOX_TYPE_BLACK));

    // #7 Water stuck-safeguard two-choice (RE 0x71B5). "Yes" (choiceIndex 0) reverts to human in z_en_elf.c.
    // Byte-for-byte parity with the RE (fd_build message_data.h 0x71B5):
    //   page 1: LIGHTBLUE, NAME (the player's name -> "@"), "... I'm not sure you can / climb out of this water in /
    //           your current form!"                                          (all light blue)
    //   BOX_BREAK ("^")
    //   page 2: LIGHTBLUE "Do you want to turn back / into a " DEFAULT "human" LIGHTBLUE "?" then the ADJUSTABLE
    //           (green) TWO_CHOICE "Yes / No".
    // SoH CustomMessage codes: %c = LIGHTBLUE, %w = DEFAULT/white, %g = ADJUSTABLE/green, @ = NAME, ^ = BOX_BREAK.
    cmm->CreateMessage(TRANSFORM_MSG_TABLE_ID, TEXT_TRANSFORM_WATER_WARNING,
                       CustomMessage("%c@... I'm not sure you can&climb out of this water in&your current form!^"
                                     "%cDo you want to turn back&into a %whuman%c?&\x1B%gYes&No%g",
                                     TEXTBOX_TYPE_BLACK));

    // #8b Fishing-hole "scary face" nag (MM3D parity). The fishing-pond owner stops a Fierce Deity heading for the
    // exit and grumbles about scaring off customers. Fired from z_fishing.c (Fishing_UpdateOwner) at the same doorway
    // trigger the "return the rod" cutscene uses. Plain owner speech (white on black), so no color codes.
    cmm->CreateMessage(TRANSFORM_MSG_TABLE_ID, TEXT_TRANSFORM_FISHING_SCARY,
                       CustomMessage("Hey. Hey! What's with that&scary face, fella?^"
                                     "If ya leave here lookin' like&that, you'll scare away&"
                                     "potential customers and&ruin our reputation!",
                                     TEXTBOX_TYPE_BLACK));

    // Serve each custom textbox when its textId opens (mirrors FierceDeityMask.cpp OnOpenText).
    COND_ID_HOOK(OnOpenText, TEXT_TRANSFORM_TOO_BIG, true, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage msg = CustomMessageManager::Instance->RetrieveMessage(TRANSFORM_MSG_TABLE_ID,
                                                                            TEXT_TRANSFORM_TOO_BIG, MF_FORMATTED);
        *loadFromMessageTable = false;
        msg.LoadIntoFont();
    });
    COND_ID_HOOK(OnOpenText, TEXT_TRANSFORM_CANT_DO_THAT, true, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage msg = CustomMessageManager::Instance->RetrieveMessage(TRANSFORM_MSG_TABLE_ID,
                                                                            TEXT_TRANSFORM_CANT_DO_THAT, MF_FORMATTED);
        *loadFromMessageTable = false;
        msg.LoadIntoFont();
    });
    COND_ID_HOOK(OnOpenText, TEXT_TRANSFORM_WATER_WARNING, true, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage msg = CustomMessageManager::Instance->RetrieveMessage(TRANSFORM_MSG_TABLE_ID,
                                                                            TEXT_TRANSFORM_WATER_WARNING, MF_FORMATTED);
        *loadFromMessageTable = false;
        msg.LoadIntoFont();
    });
    COND_ID_HOOK(OnOpenText, TEXT_TRANSFORM_FISHING_SCARY, true, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage msg = CustomMessageManager::Instance->RetrieveMessage(
            TRANSFORM_MSG_TABLE_ID, TEXT_TRANSFORM_FISHING_SCARY, MF_FORMATTED);
        *loadFromMessageTable = false;
        msg.LoadIntoFont();
    });
}

static RegisterShipInitFunc registerTransformationMaskSafeguardMessages(RegisterTransformationMaskSafeguardMessages);
