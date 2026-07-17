#ifndef Z_EN_M_THUNDER_H
#define Z_EN_M_THUNDER_H

#include <libultraship/libultra.h>
#include "global.h"

struct EnMThunder;

typedef void (*EnMThunderActionFunc)(struct EnMThunder*, PlayState*);

// FD (2026-07-12): render subtype. 0/1 are the normal OoT spin/charge (drawn via `attackStrength`, so these
// stay 0 for vanilla). 2/3 are the Fierce Deity sword-beam projectile (RE z_en_m_thunder.h:23), set only by
// the FD beam Init path -> beam draw. Kept separate from `attackStrength` so the vanilla spin is untouched.
typedef enum {
    ENMTHUNDER_SUBTYPE_SPIN_GREAT,       // 0
    ENMTHUNDER_SUBTYPE_SPIN_REGULAR,     // 1
    ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT,  // 2
    ENMTHUNDER_SUBTYPE_SWORDBEAM_REGULAR // 3
} EnMThunderSubtype;

typedef struct EnMThunder {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ColliderCylinder collider;
    /* 0x0198 */ LightNode* lightNode;
    /* 0x019C */ LightInfo lightInfo;
    /* 0x01AC */ f32 spinAttackTimer;
    /* 0x01B0 */ f32 spinAttackAlpha;
    /* 0x01B4 */ f32 spinTrailTexScroll;
    /* 0x01B8 */ f32 spinChargePercent;
    /* 0x01BC */ f32 dimmingIntensity;
    /* 0x01C0 */ EnMThunderActionFunc actionFunc;
    /* 0x01C4 */ u16 followPlayerTimer;
    /* 0x01C6 */ u8 attackStrength;
    /* 0x01C7 */ u8 swordType;
    /* 0x01C8 */ u8 chargeAlpha;
    /* 0x01C9 */ u8 targetScale;
    /* 0x01CA */ u8 isUsingMagic;
    /* 0x01CB */ u8 subtype; // FD (2026-07-12): EnMThunderSubtype -- selects the FD sword-beam draw (>= SWORDBEAM_GREAT)
} EnMThunder; // size = 0x01CC

#endif
