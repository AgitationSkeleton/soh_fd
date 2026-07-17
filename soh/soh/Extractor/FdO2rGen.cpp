#include "FdO2rGen.h"
#include "Extract.h"
#include "FdO2rManifest.h"

#include <ship/Context.h>
#include <ship/resource/archive/O2rArchive.h>
#include <ship/resource/File.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

// Read a whole entry from an opened O2R archive as raw bytes (empty on failure).
std::vector<uint8_t> ReadArchiveEntry(Ship::O2rArchive& ar, const std::string& path) {
    auto file = ar.LoadFile(path);
    if (file == nullptr || file->Buffer == nullptr) {
        return {};
    }
    return std::vector<uint8_t>(file->Buffer->begin(), file->Buffer->end());
}

// Read a whole file off disk as raw bytes (empty on failure).
std::vector<uint8_t> ReadDiskFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// --- MM audio-sample decoding (for the Young Link hookshot whip) -------------------------------------------
// The MM font-0 whip grunts are stored as N64 VADPCM inside LUS "OSMP" AudioSample V2 resources. We decode them
// to PCM here at generation time and write plain WAV, so the runtime keeps using its existing WAV mixer path.
// OSMP V2 layout (little-endian body after a 64-byte LUS resource header): u8 codec/medium/unkBit26/isRelocated,
// u32 dataSize, data[dataSize], u32 loop{start,end,count}, u32 loopStateCount + s16 state[], s32 book.order,
// s32 book.npredictors, u32 bookCount + s16 book[]. Verified against Shipwright's AudioSampleFactory reader and
// byte-exact against the shipped FD hurt grunts (SAMPLE_0_167..177). The VADPCM step is an RSP-HLE port.
struct OsmpSample {
    uint8_t codec = 0xFF;
    std::vector<uint8_t> data;
    std::vector<int16_t> book; // interleaved: [predictor][order][8]
};

template <typename T> static bool ReadLE(const std::vector<uint8_t>& b, size_t& o, T& v) {
    if (o + sizeof(T) > b.size()) {
        return false;
    }
    std::memcpy(&v, b.data() + o, sizeof(T));
    o += sizeof(T);
    return true;
}

static bool ParseOsmp(const std::vector<uint8_t>& b, OsmpSample& out) {
    // The 'OSMP' fourcc is stored little-endian, so bytes 4..7 read as "PMSO". Body starts after the 64-byte
    // LUS resource header.
    if (b.size() < 0x40 + 12 ||
        (std::memcmp(b.data() + 4, "PMSO", 4) != 0 && std::memcmp(b.data() + 4, "OSMP", 4) != 0)) {
        return false;
    }
    size_t o = 0x40;
    out.codec = b[o];
    o += 4; // codec, medium, unkBit26, isRelocated
    uint32_t dataSize = 0;
    if (!ReadLE(b, o, dataSize) || o + dataSize > b.size()) {
        return false;
    }
    out.data.assign(b.begin() + o, b.begin() + o + dataSize);
    o += dataSize;
    o += 12; // loop start/end/count
    uint32_t loopStateCount = 0;
    if (!ReadLE(b, o, loopStateCount)) {
        return false;
    }
    o += 2 * (size_t)loopStateCount;
    int32_t order = 0, npred = 0;
    if (!ReadLE(b, o, order) || !ReadLE(b, o, npred)) {
        return false;
    }
    uint32_t bookCount = 0;
    if (!ReadLE(b, o, bookCount) || o + 2 * (size_t)bookCount > b.size()) {
        return false;
    }
    out.book.resize(bookCount);
    for (uint32_t i = 0; i < bookCount; i++) {
        std::memcpy(&out.book[i], b.data() + o + 2 * i, 2);
    }
    return true;
}

static int16_t Clamp16(int32_t x) {
    return (int16_t)(x < -32768 ? -32768 : (x > 32767 ? 32767 : x));
}

// Extract a 4-bit VADPCM sample: shift the masked nibble into an s16, then arithmetic-shift right by (12-scale).
static int32_t Psample(uint8_t byte, uint8_t mask, int lsh, int rsh) {
    int32_t v = ((int32_t)(byte & mask) << lsh) & 0xFFFF;
    if (v >= 0x8000) {
        v -= 0x10000;
    }
    return v >> rsh; // arithmetic shift (v is signed)
}

// codec 0: 9-byte VADPCM frames, order-2 codebook, 16 samples/frame.
static std::vector<int16_t> DecodeVadpcm(const OsmpSample& s) {
    std::vector<int16_t> out;
    if (s.codec != 0 || s.data.size() < 9) {
        return out;
    }
    const size_t frames = s.data.size() / 9;
    out.reserve(frames * 16);
    int32_t last14 = 0, last15 = 0; // trailing two samples of the previous frame
    for (size_t f = 0; f < frames; f++) {
        const size_t base = f * 9;
        const uint8_t code = s.data[base];
        const int scale = (code & 0xF0) >> 4;
        const int idx = code & 0x0F;
        const int rsh = scale < 12 ? (12 - scale) : 0;
        const int16_t* cb = s.book.data() + (size_t)idx * 16;
        if ((size_t)idx * 16 + 16 > s.book.size()) {
            return {};
        }
        int32_t frame[16];
        for (int i = 0; i < 8; i++) {
            const uint8_t by = s.data[base + 1 + i];
            frame[2 * i] = Psample(by, 0xF0, 8, rsh);
            frame[2 * i + 1] = Psample(by, 0x0F, 12, rsh);
        }
        int32_t nf[16];
        // Two subframes of 8; each uses the previous two decoded samples as history.
        auto residuals = [&](int db, int sb, int32_t l0, int32_t l1) {
            for (int i = 0; i < 8; i++) {
                int64_t accu = (int64_t)frame[sb + i] << 11;
                accu += (int64_t)cb[i] * l0 + (int64_t)cb[8 + i] * l1;
                for (int k = 0; k < i; k++) {
                    accu += (int64_t)cb[8 + k] * frame[sb + (i - 1 - k)];
                }
                nf[db + i] = Clamp16((int32_t)(accu >> 11));
            }
        };
        residuals(0, 0, last14, last15);
        residuals(8, 8, nf[6], nf[7]);
        for (int i = 0; i < 16; i++) {
            out.push_back((int16_t)nf[i]);
        }
        last14 = nf[14];
        last15 = nf[15];
    }
    return out;
}

// Build a minimal RIFF/WAVE (PCM, mono, 16-bit) file around a PCM buffer.
static std::vector<uint8_t> MakeWav(const std::vector<int16_t>& pcm, uint32_t sampleRate) {
    const uint32_t dataBytes = (uint32_t)(pcm.size() * 2);
    const uint32_t byteRate = sampleRate * 2;
    std::vector<uint8_t> w;
    w.reserve(44 + dataBytes);
    auto put32 = [&](uint32_t v) { for (int i = 0; i < 4; i++) w.push_back((uint8_t)(v >> (8 * i))); };
    auto put16 = [&](uint16_t v) { for (int i = 0; i < 2; i++) w.push_back((uint8_t)(v >> (8 * i))); };
    const char* riff = "RIFF"; w.insert(w.end(), riff, riff + 4);
    put32(36 + dataBytes);
    const char* wave = "WAVEfmt "; w.insert(w.end(), wave, wave + 8);
    put32(16);          // fmt chunk size
    put16(1);           // PCM
    put16(1);           // mono
    put32(sampleRate);
    put32(byteRate);
    put16(2);           // block align
    put16(16);          // bits
    const char* data = "data"; w.insert(w.end(), data, data + 4);
    put32(dataBytes);
    for (int16_t s : pcm) {
        put16((uint16_t)s);
    }
    return w;
}

// Decode an OSMP whip sample from the extract into a WAV buffer (empty on failure).
static std::vector<uint8_t> DecodeWhipToWav(Ship::O2rArchive& src, const std::string& samplePath) {
    OsmpSample s;
    if (!ParseOsmp(ReadArchiveEntry(src, samplePath), s)) {
        return {};
    }
    auto pcm = DecodeVadpcm(s);
    if (pcm.empty()) {
        return {};
    }
    // Native rate for this font-0 Link-voice family (the sibling hurt grunts decode byte-exact at 20000 Hz).
    return MakeWav(pcm, 20000);
}

} // namespace

bool FdO2rGen::NeedsGeneration(const std::string& appShortName) {
    return !std::filesystem::exists(Ship::Context::LocateFileAcrossAppDirs("fd.o2r", appShortName));
}

bool FdO2rGen::Generate(const std::string& installPath, const std::string& dataPath,
                        const std::string& appShortName) {
    // Automation hook (headless / CI): if SOH_FD_MM_ROM points at a Majora's Mask ROM, skip all dialogs and
    // generate fd.o2r from it directly.
    const char* autoRom = std::getenv("SOH_FD_MM_ROM");
    const bool autoMode = (autoRom != nullptr && autoRom[0] != '\0');

    // 1. Ask. Declining just launches SoH without the Fierce Deity form.
    if (!autoMode) {
        int ret = Extractor::ShowYesNoBox(
            "Generate fd.o2r (Fierce Deity assets)",
            "The Fierce Deity form needs fd.o2r, which is generated from your own Majora's Mask ROM\n"
            "(US, N64 or GameCube) - no game assets are distributed with this fork.\n\n"
            "Generate it now? Choose No to launch without the Fierce Deity form.");
        if (ret != IDYES) {
            return false;
        }
    }

    // 2. Find and validate a Majora's Mask ROM: auto-scan the install/data dirs, else show a file picker.
    Extractor extract;
    extract.SetMM(true);

    bool haveRom = false;
    if (autoMode) {
        haveRom = extract.RunFileStandalone(autoRom);
        if (!haveRom) {
            return false;
        }
    } else {
        std::vector<std::string> roms;
        extract.SetSearchPath(installPath);
        extract.GetRoms(roms);
        extract.SetSearchPath(dataPath);
        extract.GetRoms(roms);

        for (const auto& rom : roms) {
            if (extract.RunFileStandalone(rom)) {
                haveRom = true;
                break;
            }
        }
        if (!haveRom) {
            // RomSearchMode::Both is a no-op filter in MM mode (no Master Quest), so this is a plain picker.
            if (!extract.ManuallySearchForRomMatchingType(RomSearchMode::Both)) {
                Extractor::ShowErrorBox(
                    "No Majora's Mask ROM",
                    "No valid Majora's Mask ROM was provided. The Fierce Deity form will be unavailable.\n"
                    "You can generate fd.o2r later by deleting it (if present) and relaunching.");
                return false;
            }
        }
    }

    // 3. ZAPD-extract the curated MM subset (8 XMLs) to a temporary archive.
    std::atomic<size_t> extractCount{ 0 };
    std::atomic<size_t> totalExtract{ 0 };
    std::string tempFull = extract.ExtractCuratedToTemp(installPath, &extractCount, &totalExtract);
    if (tempFull.empty() || !std::filesystem::exists(tempFull)) {
        if (!autoMode) {
            Extractor::ShowErrorBox(
                "fd.o2r generation failed",
                "Failed to extract the Majora's Mask assets. The Fierce Deity form will be unavailable.");
        }
        return false;
    }

    // 4. Filter the full extract down to the FD subset and merge in the bundled originals -> fd.o2r.
    const std::string outPath = (std::filesystem::temp_directory_path() / "fd.o2r.tmp").string();
    {
        std::error_code ec;
        std::filesystem::remove(outPath, ec);
    }

    bool ok = true;
    {
        Ship::O2rArchive src(tempFull);
        Ship::O2rArchive dst(outPath);
        if (!src.Open() || !dst.Open()) {
            ok = false;
        }

        // O2rArchive::WriteFile hands libzip a non-owning pointer to the data (it is not read until Close()),
        // so every buffer we add must stay alive until dst.Close(). Retain them here. Reserve up front so the
        // outer vector never reallocates while libzip holds pointers into it.
        std::vector<std::vector<uint8_t>> retain;
        retain.reserve(kFdRomDerivedPaths.size() + kFdRenamedRomPaths.size() + kFdBundledPaths.size() + 8);
        auto writeKeep = [&](const std::string& path, std::vector<uint8_t>&& bytes) {
            retain.push_back(std::move(bytes));
            dst.WriteFile(path, retain.back());
        };

        // 4a. ROM-derived resources, copied verbatim (byte-identical to a real MM extract). Some are legitimately
        // zero-length (e.g. gLinkFierceDeitySkelLimbs, an empty limb-table placeholder); those must still be
        // written. LoadFile returns null for a genuinely-missing entry AND for a valid size-0 entry, so use
        // HasFile to tell them apart.
        if (ok) {
            for (const char* path : kFdRomDerivedPaths) {
                if (!src.HasFile(path)) {
                    ok = false;
                    break;
                }
                writeKeep(path, ReadArchiveEntry(src, path));
            }
        }

        // 4a-renamed. ROM-derived resources that are byte-identical to an MM resource under a different name:
        // the FD voice grunts, the MM_Jumps animation data, the transform-swirl textures, and the HUD icons.
        if (ok) {
            for (const auto& rr : kFdRenamedRomPaths) {
                if (!src.HasFile(rr.src)) {
                    ok = false;
                    break;
                }
                writeKeep(rr.dst, ReadArchiveEntry(src, rr.src));
            }
        }

        // 4a-whip. ROM-derive the Young Link hookshot whip: decode the two MM font-0 samples (VADPCM) to WAV.
        // These are stored decoded (not as Sample resources) so the runtime whip mixer plays them directly.
        if (ok) {
            static const FdRenamedResource kWhip[] = {
                { "audio/samples/SAMPLE_0_202_META", "custom/samples/yl/YL_Whip1.wav" },
                { "audio/samples/SAMPLE_0_201_META", "custom/samples/yl/YL_Whip2.wav" },
            };
            for (const auto& w : kWhip) {
                if (!src.HasFile(w.src)) {
                    ok = false;
                    break;
                }
                auto wav = DecodeWhipToWav(src, w.src);
                if (wav.empty()) {
                    ok = false;
                    break;
                }
                writeKeep(w.dst, std::move(wav));
            }
        }

        // 4b. portVersion, taken from our own extract (stamped with this build's version).
        if (ok) {
            auto pv = ReadArchiveEntry(src, "portVersion");
            if (!pv.empty()) {
                writeKeep("portVersion", std::move(pv));
            }
        }

        // 4c. One ROM-derived display list that carries a small fork patch: extract, then apply the bytes.
        if (ok) {
            auto bytes = ReadArchiveEntry(src, kFdPatchedDlPath);
            if (bytes.empty()) {
                ok = false;
            } else {
                for (const auto& patch : kFdPatchedDlBytes) {
                    if (patch.offset < bytes.size()) {
                        bytes[patch.offset] = patch.value;
                    }
                }
                writeKeep(kFdPatchedDlPath, std::move(bytes));
            }
        }

        // 4d. Bundled original (non-Nintendo) resources shipped under assets/fd_bundled/.
        if (ok) {
            const std::string bundledRoot = installPath + "/assets/fd_bundled/";
            for (const char* path : kFdBundledPaths) {
                auto bytes = ReadDiskFile(bundledRoot + path);
                if (bytes.empty()) {
                    ok = false;
                    break;
                }
                writeKeep(path, std::move(bytes));
            }
        }

        // Close writes the zip while `retain` (and thus every source buffer) is still alive.
        src.Close();
        dst.Close();
    }

    {
        std::error_code ec;
        std::filesystem::remove(tempFull, ec);
    }

    if (!ok) {
        if (!autoMode) {
            Extractor::ShowErrorBox("fd.o2r generation failed",
                                    "Failed to assemble fd.o2r from the extracted assets. "
                                    "The Fierce Deity form will be unavailable.");
        }
        std::error_code ec;
        std::filesystem::remove(outPath, ec);
        return false;
    }

    // 5. Move the finished fd.o2r into the app directory, next to oot.o2r.
    const std::string finalPath = Ship::Context::GetAppDirectoryPath(appShortName) + "/fd.o2r";
    std::error_code ec;
    std::filesystem::rename(outPath, finalPath, ec);
    if (ec) {
        std::filesystem::copy(outPath, finalPath, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(outPath, ec);
    }
    // Headless/CI: SOH_FD_MM_ROM mode is "extract fd.o2r and quit" - don't proceed into the game window.
    if (autoMode) {
        std::exit(0);
    }
    return true;
}
