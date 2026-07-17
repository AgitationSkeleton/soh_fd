# Ship of Harkinian — Fierce Deity Fork

A fork of [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) (the PC port of *The Legend of
Zelda: Ocarina of Time*) that adds a fully-playable **Fierce Deity transformation mask** and a transformation
system, re-created from Aegiker's OoT Fierce Deity romhack and made behavior-accurate to *Majora's Mask* by
porting mechanics from the MM decompilation and 2 Ship 2 Harkinian.

> This is a **source + code** repository. It does **not** contain any game assets. Ocarina of Time and Majora's
> Mask assets belong to Nintendo — you must supply your own ROMs (see **Assets & `fd.o2r`** below).

## Features

- **Fierce Deity's Mask** — obtain, equip to a C-button, and transform with the full MM-style animated cutscene
  (mask-on, scream, white flash), the Fierce Deity model, sword, sword beams, movement (MM gait), and the MM
  usability rules (boss lairs + fishing hole unless "FD Usable Anywhere" is on; auto-revert safeguards).
- A **Transformation Masks** menu of toggles/cheats (FD-usable-anywhere, ocarina, item-unrestrict, strength, …).
- A **Bonus Settings** menu with optional Majora's-Mask flavor: **MM Jump Flips** (per-form regular/front-flip/
  somersault pool with the roll whoosh) and **MM Young Link Hookshot Sound** (child grapple voice).
- **Anchor co-op** integration for the Fierce Deity form (mask sync, remote transform SFX/visuals, correct
  per-form scale/model on other players' puppets).
- **Randomizer** support for the Fierce Deity's Mask.

## Assets & `fd.o2r` (important)

The Fierce Deity form needs **`fd.o2r`**, an asset archive containing the FD model, masks, animations, the
MM voice grunts / hookshot sounds, and related resources. All of the Majora's-Mask content is **derived from
your own Majora's Mask ROM at first launch** — exactly like Ship of Harkinian never ships Nintendo's `oot.o2r`
and instead generates it from your Ocarina of Time ROM. **No Nintendo assets are distributed in this
repository, and none are ever placed in the repo during generation.**

**How it works.** On first launch, after the usual "generate `oot.o2r`?" prompt, the fork shows a second
prompt for your Majora's Mask ROM (US, N64 or GameCube) and builds `fd.o2r` locally. The build runs the
bundled ZAPD asset extractor over the MM ROM using a curated subset of the MM asset XMLs, then decodes the
MM font-0 voice/whip samples (VADPCM) into the exact clips the form uses. The result is byte-accurate to a
full MM extraction.

**What *is* bundled** are only original, non-Nintendo works, credited below: **Aegiker's** authored
transformation sounds (created for the OoT Fierce Deity hack, not present in Majora's Mask), the hand-authored
transform "swirl" effect, the community **MM_Jumps** flip animations, and a few hand-patched display lists.
Everything sourced from Majora's Mask is generated from your ROM, never shipped.

## Building

Follow the standard Ship of Harkinian build instructions (see the SoH docs / `docs/`), which build `soh` for
Windows and Linux. On first launch, provide your Ocarina of Time ROM (to generate `oot.o2r`) and your Majora's
Mask ROM (to generate `fd.o2r`) when prompted.

## Credits & acknowledgements

This fork stands entirely on other people's work. Enormous thanks to:

- **[Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) & [libultraship](https://github.com/HarbourMasters/libultraship)** — HarbourMasters and the SoH contributors. This fork is built directly on their PC port and runtime; all of their code is the foundation here.
- **[2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian)** — the SoH-engine Majora's Mask port. Direct source of behavior and reference for this fork: the per-form item-usability table (`gPlayerFormItemRestrictions`), the transform-mask sound behavior, the "Hyrule Warriors Styled Link" reference, and many MM-accurate Fierce Deity mechanics.
- **Aegiker** — creator of the Ocarina of Time **Fierce Deity / "Transformation Masks" romhack** and the open-source Fierce Deity work that this fork reverse-engineers and re-ports. The FD form, its behaviors, and much of the transform flow originate from that hack.
- **[Ocarina of Time](https://github.com/zeldaret/oot) & [Majora's Mask](https://github.com/zeldaret/mm) decompilations (zeldaret)** — the decompiled sources SoH is built from and the authoritative reference for the MM behaviors ported here.
- **MM_Jumps (ModLoader64 addon)** — the Majora's Mask front-flip and somersault jump animations used by the "MM Jump Flips" feature.
- **ModLoader64 / Z64Online (OotOnline)** — reference for the planned per-player model-sync design.
- **Nintendo** — *Ocarina of Time* and *Majora's Mask*. All game assets are theirs; none are included here. Provide your own ROMs.
- **AgitationSkeleton** — this fork's author/maintainer.

If your work is used here and you are not credited, please open an issue — it's an oversight, not intent.

## License

The **code** in this fork inherits Ship of Harkinian's license (see `LICENSE`). It does **not** grant any rights
to Nintendo's assets or to third-party assets referenced by the build scripts; those remain the property of
their respective owners and are never distributed here.
