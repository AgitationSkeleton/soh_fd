// FD (2026-07-11): Custom acquisition message + text hook for the Fierce Deity mask.
//
// Registers a CustomMessageManager message keyed by TEXT_FIERCE_DEITY_MASK and an OnOpenText hook
// that serves it whenever the FD-mask acquisition textbox is opened (its textId is embedded in the
// GetItemEntry built by GiveFierceDeityMask() in src/code/fierce_deity_items.c).
#include <soh/OTRGlobals.h>
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"

#include <cstdarg>
#include <cstring>

extern "C" {
#include <z64.h>
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#define FIERCE_DEITY_MESSAGE_TABLE_ID "FierceDeity"

// FD (2026-07-12): per-language X offset for a 32x32 get-item icon (mirrors z_message's sIconItem32XOffsets).
// MUST live at file scope, NOT inside the COND_VB_SHOULD body: the preprocessor doesn't respect braces, so the
// brace-initializer commas would be parsed as extra macro arguments ("too many arguments for COND_VB_SHOULD").
static const s16 sFdIconItem32XOffsets[] = { 74, 74, 74, 54 };

// FD (2026-07-11): Registers the custom FD-mask acquisition textbox and the hook that serves it.
// Runs on boot via RegisterShipInitFunc.
static void RegisterFierceDeityMaskMessage() {
    CustomMessageManager* cmm = CustomMessageManager::Instance;
    cmm->AddCustomMessageTable(FIERCE_DEITY_MESSAGE_TABLE_ID);

    // FD (2026-07-11): OoT get-item parity (like the Ocarina of Time message): item icon (\x13 + itemId),
    // BLUE textbox, RED item name (%r..%w), and the C-button icon glyph (\xA1 = [C] in the message font).
    // FD (2026-07-12) #3: OoT tints the C-button glyph YELLOW in get-item messages (cf. the Fairy Ocarina "Set
    // it to (C)" text), so wrap the [C] in %y..%w. '&' = CustomMessage newline.
    std::string fdMaskText = std::string("\x13", 1) + std::string(1, (char)ITEM_MASK_DEITY) +
                             "You got the %rFierce Deity's Mask%w!&Could this mask's dark powers&"
                             "be as bad as Majora?&Try it on with %y" + std::string(1, (char)0xA1) + "%w.";
    cmm->CreateMessage(FIERCE_DEITY_MESSAGE_TABLE_ID, TEXT_FIERCE_DEITY_MASK,
                       CustomMessage(fdMaskText, TEXTBOX_TYPE_BLUE));

    // When the FD-mask acquisition textbox opens, serve the custom text instead of the vanilla table.
    COND_ID_HOOK(OnOpenText, TEXT_FIERCE_DEITY_MASK, true, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage msg = CustomMessageManager::Instance->RetrieveMessage(
            FIERCE_DEITY_MESSAGE_TABLE_ID, TEXT_FIERCE_DEITY_MASK, MF_FORMATTED);
        *loadFromMessageTable = false;
        msg.LoadIntoFont();
    });

    // FD (2026-07-12): custom FD-mask item icon (ITEM_MASK_DEITY, 0x9F >= ITEM_CUSTOM) in the get-item
    // textbox. The icon control code (\x13 + id) is handled by z_message's Message_Load/DrawItemIcon,
    // BOTH of which are gated `id < ITEM_CUSTOM` and so skip our custom id. Critically, the LOAD path is
    // what advances msgBufPos PAST the id byte (see Message_LoadItemIcon: `msgBufPos++`); skipping it
    // leaves 0x9F to be re-processed as a text glyph (charmap [A]=0x9F -> the stray "[A]" seen on screen)
    // AND never loads the icon texture (garbage square). So we take over both, mirroring rando's
    // Load/DrawCustomItemIcon but keyed on our textbox instead of IS_RANDO.

    // LOAD: copy the FD-mask icon OTR path into the textbox segment, set 32x32 size, and (essential)
    // advance msgBufPos past the id byte ourselves. Leaves *should=false so vanilla LoadItemIcon stays off.
    COND_VB_SHOULD(VB_LOAD_ITEM_ICON, true, {
        if (*should == false && gPlayState != NULL &&
            gPlayState->msgCtx.textId == TEXT_FIERCE_DEITY_MASK) {
            MessageContext* msgCtx = &gPlayState->msgCtx;
            if ((u8)msgCtx->font.msgBuf[msgCtx->msgBufPos + 1] == ITEM_MASK_DEITY) {
                bool displayAsEnglish = static_cast<bool>(va_arg(args, int));
                u8 language = displayAsEnglish ? LANGUAGE_ENG : gSaveContext.language;
                R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sFdIconItem32XOffsets[language];
                R_TEXTBOX_ICON_YPOS = (R_TEXTBOX_Y + 10) + 6;
                R_TEXTBOX_ICON_SIZE = 32;
                strcpy((char*)((uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE),
                       (const char*)gItemIcons[ITEM_MASK_DEITY]);
                msgCtx->msgBufPos++;
                msgCtx->choiceNum = 1;
            }
        }
    });

    // DRAW: emit the 32x32 RGBA texture block for the icon the LOAD hook staged into the segment.
    COND_VB_SHOULD(VB_DRAW_ITEM_ICON, true, {
        if (*should == false && gPlayState != NULL &&
            gPlayState->msgCtx.textId == TEXT_FIERCE_DEITY_MASK) {
            MessageContext* msgCtx = &gPlayState->msgCtx;
            Gfx** p = va_arg(args, Gfx**);
            Gfx* gfx = *p;
            gDPLoadTextureBlock(gfx++, (uintptr_t)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE,
                                G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            *p = gfx;
        }
    });
}

static RegisterShipInitFunc registerFierceDeityMaskMessage(RegisterFierceDeityMaskMessage);
