#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>
#include <winternl.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

#define COLOR_RESET  "\x1b[0m"
#define COLOR_GREEN  "\x1b[92m"
#define COLOR_RED  "\x1b[91m"
#define COLOR_YELLOW  "\x1b[93m"
#define COLOR_CYAN  "\x1b[96m"
#define COLOR_MAGENTA  "\x1b[95m"
#define COLOR_GRAY  "\x1b[90m"
#define COLOR_BOLD  "\x1b[1m"

static void enableVirtualTerminal() {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(consoleHandle, &mode))
        SetConsoleMode(consoleHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

struct FilmErrorSite { uint32_t moduleOffset; uint8_t length; uint8_t originalBytes[7]; };

static const FilmErrorSite FILM_ERROR_SITES_REACH[] = {
    {0x005A4B5, 7, {0xC6,0x05,0xCC,0x49,0x73,0x02,0x01}},
    {0x005A920, 7, {0x44,0x88,0x35,0x61,0x45,0x73,0x02}},
    {0x005B1E1, 7, {0xC6,0x05,0xA0,0x3C,0x73,0x02,0x01}},
    {0x005B2AA, 7, {0xC6,0x05,0xD7,0x3B,0x73,0x02,0x01}},
    {0x005BE9D, 7, {0xC6,0x05,0xE4,0x2F,0x73,0x02,0x01}},
    {0x0171717, 7, {0x44,0x88,0x25,0x6A,0xD7,0x61,0x02}},
    {0x0171979, 7, {0xC6,0x05,0x08,0xD5,0x61,0x02,0x01}},
    {0x0171E0F, 7, {0xC6,0x05,0x72,0xD0,0x61,0x02,0x01}},
    {0x035C53D, 7, {0xC6,0x05,0x44,0x29,0x43,0x02,0x01}},
    {0x035CD3E, 7, {0xC6,0x05,0x43,0x21,0x43,0x02,0x01}},
    {0x035D288, 7, {0xC6,0x05,0xF9,0x1B,0x43,0x02,0x01}},
    {0x03AA70D, 6, {0x88,0x1D,0x75,0x47,0x3E,0x02,0x00}},
    {0x03AAE97, 7, {0x44,0x88,0x25,0xEA,0x3F,0x3E,0x02}},
};

static const FilmErrorSite FILM_ERROR_SITES_H3[] = {
    {0x0035DCD, 7, {0x44,0x88,0x3D,0xCC,0x13,0x10,0x02}},
    {0x0036443, 7, {0x44,0x88,0x25,0x56,0x0D,0x10,0x02}},
    {0x00F1A31, 7, {0x40,0x88,0x2D,0x68,0x57,0x04,0x02}},
    {0x00F22AD, 7, {0xC6,0x05,0xEC,0x4E,0x04,0x02,0x01}},
    {0x00F2B39, 7, {0xC6,0x05,0x60,0x46,0x04,0x02,0x01}},
    {0x00F2F0B, 7, {0xC6,0x05,0x8E,0x42,0x04,0x02,0x01}},
    {0x0109D17, 7, {0xC6,0x05,0x82,0xD4,0x02,0x02,0x01}},
    {0x01CE1D7, 7, {0x44,0x88,0x25,0xC2,0x8F,0xF6,0x01}},
    {0x01CE3DB, 7, {0xC6,0x05,0xBE,0x8D,0xF6,0x01,0x01}},
    {0x01CE7F8, 7, {0xC6,0x05,0xA1,0x89,0xF6,0x01,0x01}},
    {0x0327F2A, 7, {0x40,0x88,0x35,0x6F,0xF2,0xE0,0x01}},
    {0x0327FE7, 7, {0x40,0x88,0x35,0xB2,0xF1,0xE0,0x01}},
    {0x0329489, 7, {0x40,0x88,0x35,0x10,0xDD,0xE0,0x01}},
};

static const FilmErrorSite FILM_ERROR_SITES_ODST[] = {
    {0x00380D7, 7, {0x44,0x88,0x3D,0xA2,0xCF,0x13,0x02}},
    {0x0038887, 7, {0x44,0x88,0x25,0xF2,0xC7,0x13,0x02}},
    {0x010D1C1, 7, {0x40,0x88,0x2D,0xB8,0x7E,0x06,0x02}},
    {0x010DA3D, 7, {0xC6,0x05,0x3C,0x76,0x06,0x02,0x01}},
    {0x010E2C9, 7, {0xC6,0x05,0xB0,0x6D,0x06,0x02,0x01}},
    {0x010E69B, 7, {0xC6,0x05,0xDE,0x69,0x06,0x02,0x01}},
    {0x012842B, 7, {0xC6,0x05,0x4E,0xCC,0x04,0x02,0x01}},
    {0x020318B, 7, {0x44,0x88,0x25,0xEE,0x1E,0xF7,0x01}},
    {0x020338F, 7, {0xC6,0x05,0xEA,0x1C,0xF7,0x01,0x01}},
    {0x02037AC, 7, {0xC6,0x05,0xCD,0x18,0xF7,0x01,0x01}},
    {0x035F40E, 7, {0x40,0x88,0x35,0x6B,0x5C,0xE1,0x01}},
    {0x035F4CB, 7, {0x40,0x88,0x35,0xAE,0x5B,0xE1,0x01}},
    {0x0360A69, 7, {0x40,0x88,0x35,0x10,0x46,0xE1,0x01}},
};

static const uint32_t POPUP_SITES_REACH[] = { 0x172A1B  , 0x17287F   };

static const uint32_t POPUP_SITES_H3[] = {
    0x10063, 0x42CA5, 0x42CF9, 0x45D1F, 0x481D6,
    0x6A456, 0x6D725, 0x7319A, 0x146BEF, 0x147007,
    0x1CEE81  ,
    0x2215FD, 0x2BA99E, 0x2BAA0C, 0x2BABAD, 0x2BAC35,
    0x2BAD8A, 0x2BD7F6, 0x2BE3E8, 0x2BE41B, 0x2BE4BB,
    0x2BEA76, 0x2BED0F, 0x2BF44C, 0x2CBA6F, 0x2CBB90,
    0x2D7564, 0x2D8762, 0x2F82EF, 0x2F898B, 0x2F9E4D,
    0x2FA1E6, 0x2FA3D2, 0x2FD009, 0x2FD054, 0x2FF1C3,
    0x2FF1F2, 0x2FF402, 0x2FF4E3, 0x2FF606, 0x2FF642,
    0x2FF66D, 0x2FF79A, 0x2FF7C9, 0x30368C, 0x304662,
    0x305D58, 0x3063BE, 0x306528, 0x306CE0, 0x307153,
    0x3087C1, 0x30AB6D, 0x30BD3B, 0x30BF17, 0x30E828,
    0x311039, 0x3118D6, 0x317742, 0x3177FE, 0x317CCA,
    0x3190BE  ,
    0x3206DC, 0x322076, 0x3220A5, 0x32245C, 0x322487,
    0x3225E2, 0x322611, 0x32275A, 0x3228DD, 0x322907,
    0x32AD50, 0x3356F2,
};
struct ErrorCode { uint8_t code; const char* where; };
static const ErrorCode ERROR_CODES_H3[] = {
    { 2,  "sub_1800F1850 per-frame update, the clip pump returned false"},
    { 3,  "sub_1800361A8 sim tick, sub_1801CF3BC returned false (clip stuck/failed)"},
    { 4,  "sub_180327D0C theater UI"},
    { 5,  "sub_180327D0C theater UI"},
    { 6,  "sub_1800F2280 replay / DVR start"},
    { 8,  "sub_1803292E4 theater UI (save clip)"},
    {10,  "KEYFRAME write or read failed (sub_1801CDF40 / sub_1801CE258)"},
    {12,  "APPLY KEYFRAME failed (sub_1801CE468), CRC or read error"},
    {13,  "sub_1800F2E14 seek/skip consume, no keyframe to land on"},
    {14,  "sub_1800F2A6C"},
    {15,  "sub_180109C00"},
};
static const ErrorCode ERROR_CODES_REACH[] = {
    { 1,  "sub_1803AA268 frame build"},
    { 2,  "sub_18005A758 update, pump failed"},
    { 3,  "sub_1803AABD0 advance / pump"},
    { 4,  "sub_18035D1B0 record-clip UI"},
    { 5,  "sub_18035CD08 clip stop"},
    { 6,  "sub_18005B1B4 seek/segment"},
    { 8,  "sub_18035C510 save-clip UI"},
    { 9,  "sub_18005B208 clip save op"},
    {10,  "keyframe writer (sub_1801714D0 / sub_180171800)"},
    {12,  "apply keyframe (sub_180171A0C)"},
    {13,  "sub_18005BD9C marker-seek consume"},
    {15,  "sub_18005A3B8 theater tick, header/version mismatch"},
};

struct RingReference { uint32_t moduleOffset; uint8_t length, which; uint8_t originalBytes[8]; };
struct RingImmediate { uint32_t moduleOffset; uint8_t length, immediateOffset, immediateSize; uint8_t originalBytes[6]; };

static const RingReference RING_REFERENCES_H3[] = {
  {0x0005528, 7, 1, {0x48,0x8D,0x05,0xC9,0x09,0x28,0x02}},
  {0x003639D, 7, 0, {0x4C,0x8D,0x0D,0x4C,0xFB,0x24,0x02}},
  {0x00F17CC, 7, 0, {0x48,0x8B,0x0D,0x1D,0x47,0x19,0x02}},
  {0x00F17F5, 8, 0, {0x48,0x83,0x25,0xF3,0x46,0x19,0x02,0x00}},
  {0x00F19C9, 7, 0, {0x48,0x8B,0x05,0x20,0x45,0x19,0x02}},
  {0x00F19E6, 7, 0, {0x48,0x8B,0x05,0x03,0x45,0x19,0x02}},
  {0x00F19FF, 7, 0, {0x48,0x89,0x35,0xEA,0x44,0x19,0x02}},
  {0x00F2506, 7, 0, {0x4C,0x8D,0x3D,0xE3,0x39,0x19,0x02}},
  {0x00F2E47, 7, 1, {0x48,0x8D,0x0D,0xAA,0x30,0x19,0x02}},
  {0x0109BC4, 8, 0, {0x48,0x83,0x25,0x24,0xC3,0x17,0x02,0x00}},
  {0x01CDFF7, 7, 2, {0x48,0x8D,0x15,0xFE,0x7E,0x0B,0x02}},
  {0x01CE042, 7, 0, {0x48,0x8B,0x05,0xA7,0x7E,0x0B,0x02}},
  {0x01CE056, 7, 0, {0x48,0x89,0x05,0x93,0x7E,0x0B,0x02}},
  {0x01CE070, 7, 0, {0x48,0x8D,0x35,0x79,0x7E,0x0B,0x02}},
  {0x01CE0A3, 7, 1, {0x48,0x8D,0x0D,0x4E,0x7E,0x0B,0x02}},
  {0x01CE0DC, 7, 1, {0x48,0x8D,0x0D,0x15,0x7E,0x0B,0x02}},
  {0x01CE0E8, 7, 0, {0x48,0x8B,0x0D,0x01,0x7E,0x0B,0x02}},
  {0x01CE172, 7, 0, {0x48,0x8B,0x0D,0x77,0x7D,0x0B,0x02}},
  {0x01CE18A, 7, 0, {0x48,0x8B,0x0D,0x5F,0x7D,0x0B,0x02}},
  {0x01CE1AC, 7, 0, {0x48,0x8B,0x0D,0x3D,0x7D,0x0B,0x02}},
  {0x01CE2A4, 7, 0, {0x48,0x8D,0x15,0x45,0x7C,0x0B,0x02}},
  {0x01CE2D1, 7, 0, {0x48,0x8B,0x05,0x18,0x7C,0x0B,0x02}},
  {0x01CE2E2, 7, 0, {0x48,0x89,0x05,0x07,0x7C,0x0B,0x02}},
  {0x01CE2FB, 7, 1, {0x48,0x8D,0x0D,0xF6,0x7B,0x0B,0x02}},
  {0x01CE307, 7, 0, {0x48,0x8B,0x0D,0xE2,0x7B,0x0B,0x02}},
  {0x01CE37A, 7, 0, {0x48,0x8B,0x0D,0x6F,0x7B,0x0B,0x02}},
  {0x01CE397, 7, 0, {0x48,0x8B,0x0D,0x52,0x7B,0x0B,0x02}},
  {0x01CE3B9, 7, 0, {0x48,0x8B,0x0D,0x30,0x7B,0x0B,0x02}},
  {0x01CE48D, 7, 1, {0x48,0x8D,0x35,0x64,0x7A,0x0B,0x02}},
  {0x01CE511, 7, 1, {0x48,0x8D,0x0D,0xE0,0x79,0x0B,0x02}},
  {0x01CE5DD, 7, 1, {0x48,0x8D,0x05,0x14,0x79,0x0B,0x02}},
  {0x01CE64C, 7, 1, {0x48,0x8D,0x0D,0xA5,0x78,0x0B,0x02}},
  {0x01CE714, 7, 1, {0x48,0x8D,0x35,0xDD,0x77,0x0B,0x02}},
  {0x01CE855, 7, 1, {0x48,0x8D,0x0D,0x9C,0x76,0x0B,0x02}},
  {0x01CE872, 7, 0, {0x4C,0x8D,0x0D,0x77,0x76,0x0B,0x02}},
  {0x01CE8CF, 7, 1, {0x48,0x8D,0x0D,0x22,0x76,0x0B,0x02}},
  {0x01CE8EC, 7, 2, {0x48,0x8D,0x15,0x09,0x76,0x0B,0x02}},
  {0x01CE948, 7, 1, {0x48,0x8D,0x05,0xA9,0x75,0x0B,0x02}},
};
static const RingImmediate RING_IMMEDIATES_H3[] = {
  {0x003636B, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x0036390, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x00363CE, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x01CE014, 4, 3, 1, {0x41,0x83,0xF8,0x10}},
  {0x01CE922, 4, 3, 1, {0x41,0x83,0xF8,0x10}},
  {0x01CE99E, 6, 2, 4, {0x41,0xBA,0x10,0x00,0x00,0x00}},
  {0x01CEAD2, 4, 3, 1, {0x41,0x83,0xFA,0x10}},
  {0x00F2571, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x00F25D8, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x01CE08F, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x01CE2C1, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
};
static const RingReference RING_REFERENCES_ODST[] = {
  {0x00057E8, 7, 1, {0x48,0x8D,0x05,0x39,0x46,0x2C,0x02}},
  {0x00387E1, 7, 0, {0x4C,0x8D,0x0D,0x38,0x16,0x29,0x02}},
  {0x010CF5C, 7, 0, {0x48,0x8B,0x0D,0xBD,0xCE,0x1B,0x02}},
  {0x010CF85, 8, 0, {0x48,0x83,0x25,0x93,0xCE,0x1B,0x02,0x00}},
  {0x010D159, 7, 0, {0x48,0x8B,0x05,0xC0,0xCC,0x1B,0x02}},
  {0x010D176, 7, 0, {0x48,0x8B,0x05,0xA3,0xCC,0x1B,0x02}},
  {0x010D18F, 7, 0, {0x48,0x89,0x35,0x8A,0xCC,0x1B,0x02}},
  {0x010DC96, 7, 0, {0x4C,0x8D,0x3D,0x83,0xC1,0x1B,0x02}},
  {0x010E5D7, 7, 1, {0x48,0x8D,0x0D,0x4A,0xB8,0x1B,0x02}},
  {0x01282D8, 8, 0, {0x48,0x83,0x25,0x40,0x1B,0x1A,0x02,0x00}},
  {0x0202FAB, 7, 2, {0x48,0x8D,0x15,0x7A,0x6E,0x0C,0x02}},
  {0x0202FF6, 7, 0, {0x48,0x8B,0x05,0x23,0x6E,0x0C,0x02}},
  {0x020300A, 7, 0, {0x48,0x89,0x05,0x0F,0x6E,0x0C,0x02}},
  {0x0203024, 7, 0, {0x48,0x8D,0x35,0xF5,0x6D,0x0C,0x02}},
  {0x0203057, 7, 1, {0x48,0x8D,0x0D,0xCA,0x6D,0x0C,0x02}},
  {0x0203090, 7, 1, {0x48,0x8D,0x0D,0x91,0x6D,0x0C,0x02}},
  {0x020309C, 7, 0, {0x48,0x8B,0x0D,0x7D,0x6D,0x0C,0x02}},
  {0x0203126, 7, 0, {0x48,0x8B,0x0D,0xF3,0x6C,0x0C,0x02}},
  {0x020313E, 7, 0, {0x48,0x8B,0x0D,0xDB,0x6C,0x0C,0x02}},
  {0x0203160, 7, 0, {0x48,0x8B,0x0D,0xB9,0x6C,0x0C,0x02}},
  {0x0203258, 7, 0, {0x48,0x8D,0x15,0xC1,0x6B,0x0C,0x02}},
  {0x0203285, 7, 0, {0x48,0x8B,0x05,0x94,0x6B,0x0C,0x02}},
  {0x0203296, 7, 0, {0x48,0x89,0x05,0x83,0x6B,0x0C,0x02}},
  {0x02032AF, 7, 1, {0x48,0x8D,0x0D,0x72,0x6B,0x0C,0x02}},
  {0x02032BB, 7, 0, {0x48,0x8B,0x0D,0x5E,0x6B,0x0C,0x02}},
  {0x020332E, 7, 0, {0x48,0x8B,0x0D,0xEB,0x6A,0x0C,0x02}},
  {0x020334B, 7, 0, {0x48,0x8B,0x0D,0xCE,0x6A,0x0C,0x02}},
  {0x020336D, 7, 0, {0x48,0x8B,0x0D,0xAC,0x6A,0x0C,0x02}},
  {0x0203441, 7, 1, {0x48,0x8D,0x35,0xE0,0x69,0x0C,0x02}},
  {0x02034C5, 7, 1, {0x48,0x8D,0x0D,0x5C,0x69,0x0C,0x02}},
  {0x0203591, 7, 1, {0x48,0x8D,0x05,0x90,0x68,0x0C,0x02}},
  {0x0203600, 7, 1, {0x48,0x8D,0x0D,0x21,0x68,0x0C,0x02}},
  {0x02036C8, 7, 1, {0x48,0x8D,0x35,0x59,0x67,0x0C,0x02}},
  {0x0203809, 7, 1, {0x48,0x8D,0x0D,0x18,0x66,0x0C,0x02}},
  {0x0203826, 7, 0, {0x4C,0x8D,0x0D,0xF3,0x65,0x0C,0x02}},
  {0x0203883, 7, 1, {0x48,0x8D,0x0D,0x9E,0x65,0x0C,0x02}},
  {0x02038A0, 7, 2, {0x48,0x8D,0x15,0x85,0x65,0x0C,0x02}},
  {0x02038FC, 7, 1, {0x48,0x8D,0x05,0x25,0x65,0x0C,0x02}},
};
static const RingImmediate RING_IMMEDIATES_ODST[] = {
  {0x00387AF, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x00387D4, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x0038812, 3, 2, 1, {0x83,0xF9,0x10}},
  {0x0202FC8, 4, 3, 1, {0x41,0x83,0xF8,0x10}},
  {0x02038D6, 4, 3, 1, {0x41,0x83,0xF8,0x10}},
  {0x0203952, 6, 2, 4, {0x41,0xBA,0x10,0x00,0x00,0x00}},
  {0x0203A86, 4, 3, 1, {0x41,0x83,0xFA,0x10}},
  {0x010DD01, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x010DD68, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x0203043, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
  {0x0203275, 6, 2, 4, {0x48,0x3D,0x80,0x01,0x00,0x00}},
};

struct Title {
    const wchar_t* module;
    const char*    name;
    uint32_t hook;
    uint8_t  hookLength, hookOriginalBytes[8];
    uint32_t startFunction, stopFunction, skipFunction;
    uint32_t poolInitFunction, pool;
    uint32_t state, speed, tick;
    uint32_t saveFunction, clipId;
    uint32_t saveOperation;
    uint32_t keyframeTable; uint8_t keyframeSlotCount, keyframeStride, keyframeTickOffset, keyframeValidOffset;
    uint32_t clipReset;
    uint32_t clipMaxLength, clipStart, clipEnd;
    uint32_t cameraBlobHandle, cameraBlobMode, filmWriterState;
    uint32_t closeBlobFunction, cameraBlobObject, closeWriterFunction, filmWriterObject;
    uint32_t filmWriterError;
    uint32_t clipLengthPatch;
    uint32_t keyframeGates[3];
    uint8_t  keyframeGateOperandBytes[3];
    const RingReference* ringReferences; int ringReferenceCount;
    const RingImmediate* ringImmediates; int ringImmediateCount;
    const ErrorCode*     errorCodes; int errorCodeCount;
    const FilmErrorSite* filmErrorSites; int filmErrorSiteCount;
    const uint32_t*      popupSites; int popupSiteCount;
};

static const Title TITLES[] = {
  { L"haloreach.dll", "Halo: Reach",
    0x1741FC, 7, {0x48,0x8B,0xC4,0x48,0x89,0x58,0x10},
    0x17245C, 0x172518, 0x05B184,
    0x0D2878, 0x4E306B8,
    0x29BBA38, 0x278EE3C, 0x275A228,
    0x05B208, 0x29B9F20,
    0x29BA358,
    0x2999428, 11, 56, 4, 0x33,
    0x17337C,
    0x29B9010, 0x29B9D0C, 0x29B9D10,
    0, 0, 0,
    0, 0, 0, 0,
    0,
    0,
    {0, 0, 0}, {0, 0, 0},
    nullptr, 0, nullptr, 0,
    ERROR_CODES_REACH, (int)(sizeof(ERROR_CODES_REACH)/sizeof(ErrorCode)),
    FILM_ERROR_SITES_REACH, (int)(sizeof(FILM_ERROR_SITES_REACH)/sizeof(FilmErrorSite)),
    POPUP_SITES_REACH, (int)(sizeof(POPUP_SITES_REACH)/sizeof(uint32_t)) },

  { L"halo3.dll", "Halo 3",
    0x0F1850, 7, {0x48,0x8B,0xC4,0x48,0x89,0x58,0x18},
    0x1CEAE4, 0x1CEB8C, 0x0F2250,
    0x1227D8, 0x46B7718,
    0x94C850, 0x2137154, 0x1FD1E18,
    0, 0,
    0,
    0x2285EF8, 16, 24, 4, 23,
    0x1CF670,
    0x94C330, 0x94C708, 0x94C70C,
    0x93C4F0, 0x93C4E8, 0x93C510,
    0xB501C, 0x93C460, 0x107E04, 0x93C510,
    0x94C504,
    0x1CEBA8,
    {0x1CDF88, 0x1CE038, 0x3632E}, {0x78, 0x78, 0x79},
    RING_REFERENCES_H3, (int)(sizeof(RING_REFERENCES_H3)/sizeof(RingReference)),
    RING_IMMEDIATES_H3, (int)(sizeof(RING_IMMEDIATES_H3)/sizeof(RingImmediate)),
    ERROR_CODES_H3, (int)(sizeof(ERROR_CODES_H3)/sizeof(ErrorCode)),
    FILM_ERROR_SITES_H3, (int)(sizeof(FILM_ERROR_SITES_H3)/sizeof(FilmErrorSite)),
    POPUP_SITES_H3, (int)(sizeof(POPUP_SITES_H3)/sizeof(uint32_t)) },

  { L"halo3odst.dll", "Halo 3: ODST",
    0x10CFE0, 7, {0x48,0x8B,0xC4,0x48,0x89,0x58,0x18},
    0x203A98, 0x203B40, 0x10D9E0,
    0x1433F4, 0x46E29E8,
    0x993400, 0x2175034, 0x2022BB4,
    0, 0,
    0,
    0x22C9E28, 16, 24, 4, 23,
    0x2049E4,
    0x992EE0, 0x9932B8, 0x9932BC,
    0, 0, 0,
    0, 0, 0, 0,
    0,
    0x203B5C,
    {0x202F3C, 0x202FEC, 0x38772}, {0x78, 0x78, 0x79},
    RING_REFERENCES_ODST, (int)(sizeof(RING_REFERENCES_ODST)/sizeof(RingReference)),
    RING_IMMEDIATES_ODST, (int)(sizeof(RING_IMMEDIATES_ODST)/sizeof(RingImmediate)),
    ERROR_CODES_H3, (int)(sizeof(ERROR_CODES_H3)/sizeof(ErrorCode)),
    FILM_ERROR_SITES_ODST, (int)(sizeof(FILM_ERROR_SITES_ODST)/sizeof(FilmErrorSite)),
    nullptr, 0 },
};
static const int TITLE_COUNT = (int)(sizeof(TITLES) / sizeof(TITLES[0]));

static const Title* g_title = nullptr;

static int32_t g_clipLengthTicks = 30 * 30;
static bool g_forceBudget = false;
static bool g_holdOpen = false;

static void enableDebugPrivilege() {
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) return;
    TOKEN_PRIVILEGES tokenPrivileges{}; tokenPrivileges.PrivilegeCount = 1;
    LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tokenPrivileges.Privileges[0].Luid);
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(token, FALSE, &tokenPrivileges, 0, nullptr, nullptr);
    CloseHandle(token);
}

static bool findModule(DWORD& processIdOut, uint64_t& baseOut, const Title*& titleOut) {
    HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processSnapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W processEntry{ sizeof(processEntry) };
    bool found = false;
    for (BOOL ok = Process32FirstW(processSnapshot, &processEntry); ok && !found; ok = Process32NextW(processSnapshot, &processEntry)) {
        HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processEntry.th32ProcessID);
        if (moduleSnapshot == INVALID_HANDLE_VALUE) continue;
        MODULEENTRY32W moduleEntry{ sizeof(moduleEntry) };
        for (BOOL moreModules = Module32FirstW(moduleSnapshot, &moduleEntry); moreModules && !found; moreModules = Module32NextW(moduleSnapshot, &moduleEntry)) {
            for (int i = 0; i < TITLE_COUNT; i++) {
                if (_wcsicmp(moduleEntry.szModule, TITLES[i].module) == 0) {
                    processIdOut = processEntry.th32ProcessID; baseOut = (uint64_t)moduleEntry.modBaseAddr;
                    titleOut = &TITLES[i]; found = true; break;
                }
            }
        }
        CloseHandle(moduleSnapshot);
    }
    CloseHandle(processSnapshot);
    return found;
}

static uint64_t allocateNear(HANDLE process, uint64_t anchor, SIZE_T size) {
    SYSTEM_INFO systemInfo; GetSystemInfo(&systemInfo);
    const uint64_t granularity = systemInfo.dwAllocationGranularity;
    const uint64_t limit = 0x78000000ull;
    uint64_t start = anchor & ~(granularity - 1);
    for (uint64_t offset = granularity; offset < limit; offset += granularity) {
        for (int direction = 0; direction < 2; ++direction) {
            uint64_t candidate = direction ? start + offset : start - offset;
            void* allocation = VirtualAllocEx(process, (void*)candidate, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (allocation) return (uint64_t)allocation;
        }
    }
    return 0;
}

template<class ValueType> static bool writeValue(HANDLE process, uint64_t address, const ValueType& value) {
    SIZE_T transferred; return WriteProcessMemory(process, (void*)address, &value, sizeof(ValueType), &transferred) && transferred == sizeof(ValueType);
}
static bool writeBuffer(HANDLE process, uint64_t address, const void* buffer, SIZE_T length) {
    SIZE_T transferred; return WriteProcessMemory(process, (void*)address, buffer, length, &transferred) && transferred == length;
}
static bool readBuffer(HANDLE process, uint64_t address, void* buffer, SIZE_T length) {
    SIZE_T transferred; return ReadProcessMemory(process, (void*)address, buffer, length, &transferred) && transferred == length;
}
static bool writeCode(HANDLE process, uint64_t address, const void* buffer, SIZE_T length) {
    DWORD oldProtection; if (!VirtualProtectEx(process, (void*)address, length, PAGE_EXECUTE_READWRITE, &oldProtection)) return false;
    bool ok = writeBuffer(process, address, buffer, length);
    VirtualProtectEx(process, (void*)address, length, oldProtection, &oldProtection);
    FlushInstructionCache(process, (void*)address, length);
    return ok;
}

static char* formatHexBytes(char* out, int capacity, const uint8_t* bytes, int length) {
    int position = 0;
    for (int i = 0; i < length && position < capacity - 3; i++)
        position += snprintf(out + position, capacity - position, "%s%02X", i ? " " : "", bytes[i]);
    return out;
}

static int filmErrorApply(HANDLE process, uint64_t base) {
    const uint8_t nopBytes[7] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    for (int i = 0; i < g_title->filmErrorSiteCount; i++) {
        uint8_t currentBytes[7] = {0};
        if (!readBuffer(process, base + g_title->filmErrorSites[i].moduleOffset, currentBytes, g_title->filmErrorSites[i].length)) return -1;
        bool isOriginal = memcmp(currentBytes, g_title->filmErrorSites[i].originalBytes, g_title->filmErrorSites[i].length) == 0;
        bool isNop = true; for (int byteIndex = 0; byteIndex < g_title->filmErrorSites[i].length; byteIndex++) if (currentBytes[byteIndex] != 0x90) isNop = false;
        if (!isOriginal && !isNop) return -1;
    }
    int count = 0;
    for (int i = 0; i < g_title->filmErrorSiteCount; i++)
        if (writeCode(process, base + g_title->filmErrorSites[i].moduleOffset, nopBytes, g_title->filmErrorSites[i].length)) count++;
    return count;
}
static void filmErrorRevert(HANDLE process, uint64_t base) {
    for (int i = 0; i < g_title->filmErrorSiteCount; i++)
        writeCode(process, base + g_title->filmErrorSites[i].moduleOffset, g_title->filmErrorSites[i].originalBytes, g_title->filmErrorSites[i].length);
}

static bool g_keyframeCaptureOn = false;
static bool g_keyframeCaptureEnabled = true;
static int keyframeGateSet(HANDLE process, uint64_t base, uint8_t want) {
    if (!g_title->keyframeGates[0]) return -1;
    const uint8_t expect = (want == 1) ? 2 : 1;
    for (int i = 0; i < 3; i++) {
        uint8_t currentBytes[4] = {0};
        if (!readBuffer(process, base + g_title->keyframeGates[i], currentBytes, 4)) return -1;
        if (currentBytes[0] != 0x80 || currentBytes[1] != g_title->keyframeGateOperandBytes[i] || currentBytes[2] != 0x10) return -1;
        if (currentBytes[3] != expect && currentBytes[3] != want) return -1;
    }
    int patchedCount = 0;
    for (int i = 0; i < 3; i++)
        if (writeCode(process, base + g_title->keyframeGates[i] + 3, &want, 1)) patchedCount++;
    return patchedCount;
}

#define RING_STOCK_SLOTS 16
static int      g_ringSlotCount    = 0;
static uint64_t g_ringCave = 0;
static bool     g_ringRelocated   = false;
static uint64_t ringEntriesAddress(uint64_t base) { return g_ringRelocated ? g_ringCave + 8 : base + g_title->keyframeTable; }
static uint64_t ringFilePointerAddress(uint64_t base)  { return g_ringRelocated ? g_ringCave : base + g_title->keyframeTable - 8; }
static int      ringSlotCount()            { return g_ringRelocated ? g_ringSlotCount : (int)g_title->keyframeSlotCount; }

static int suspendAllThreads(DWORD processId, std::vector<HANDLE>& out) {
    HANDLE threadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (threadSnapshot == INVALID_HANDLE_VALUE) return 0;
    THREADENTRY32 threadEntry{ sizeof(threadEntry) };
    for (BOOL ok = Thread32First(threadSnapshot, &threadEntry); ok; ok = Thread32Next(threadSnapshot, &threadEntry)) {
        if (threadEntry.th32OwnerProcessID != processId) continue;
        HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
        if (!threadHandle) continue;
        if (SuspendThread(threadHandle) == (DWORD)-1) { CloseHandle(threadHandle); continue; }
        out.push_back(threadHandle);
    }
    CloseHandle(threadSnapshot);
    return (int)out.size();
}
static void resumeAllThreads(std::vector<HANDLE>& threads) {
    for (size_t i = 0; i < threads.size(); i++) { ResumeThread(threads[i]); CloseHandle(threads[i]); }
    threads.clear();
}

static uint64_t ringResolve(HANDLE process, uint64_t base, const RingReference& reference) {
    uint8_t currentBytes[8] = {0};
    if (!readBuffer(process, base + reference.moduleOffset, currentBytes, reference.length)) return 0;
    if (memcmp(currentBytes, reference.originalBytes, 3) != 0) return 0;
    int32_t displacement; memcpy(&displacement, currentBytes + 3, 4);
    return base + reference.moduleOffset + reference.length + (int64_t)displacement;
}
static bool ringPoint(HANDLE process, uint64_t base, const RingReference& reference, uint64_t target) {
    uint8_t instructionBytes[8];
    memcpy(instructionBytes, reference.originalBytes, reference.length);
    int32_t displacement = (int32_t)((int64_t)target - (int64_t)(base + reference.moduleOffset + reference.length));
    memcpy(instructionBytes + 3, &displacement, 4);
    return writeCode(process, base + reference.moduleOffset, instructionBytes, reference.length);
}
static uint32_t ringImmediateOriginalValue(const RingImmediate& immediate) {
    if (immediate.immediateSize == 1) return immediate.originalBytes[immediate.immediateOffset];
    return (uint32_t)immediate.originalBytes[immediate.immediateOffset] | ((uint32_t)immediate.originalBytes[immediate.immediateOffset+1] << 8)
         | ((uint32_t)immediate.originalBytes[immediate.immediateOffset+2] << 16) | ((uint32_t)immediate.originalBytes[immediate.immediateOffset+3] << 24);
}

static int ringApply(HANDLE process, DWORD processId, uint64_t base, int slots) {
    if (!g_title->ringReferenceCount || !g_ringCave) return -1;
    const uint64_t header = base + g_title->keyframeTable - 8, entries = base + g_title->keyframeTable;
    const uint64_t caveHeader = g_ringCave, caveEntries = g_ringCave + 8;

    int stock = 0, alreadyRelocated = 0;
    for (int i = 0; i < g_title->ringReferenceCount; i++) {
        const RingReference& reference = g_title->ringReferences[i];
        uint64_t want = (reference.which == 0) ? header  : (reference.which == 1) ? entries  : entries  + 4;
        uint64_t caveTarget = (reference.which == 0) ? caveHeader : (reference.which == 1) ? caveEntries : caveEntries + 4;
        uint64_t resolved = ringResolve(process, base, reference);
        if (resolved == want) stock++;
        else if (resolved == caveTarget) alreadyRelocated++;
        else return -(1000 + i);
    }
    for (int i = 0; i < g_title->ringImmediateCount; i++) {
        const RingImmediate& immediate = g_title->ringImmediates[i];
        uint8_t currentBytes[6] = {0};
        if (!readBuffer(process, base + immediate.moduleOffset, currentBytes, immediate.length)) return -(2000 + i);
        if (memcmp(currentBytes, immediate.originalBytes, immediate.immediateOffset) != 0) return -(2000 + i);
    }
    if (stock == 0) return 0;

    std::vector<uint8_t> seed((size_t)8 + (size_t)slots * 24, 0);
    if (!readBuffer(process, header, seed.data(), 8 + RING_STOCK_SLOTS * 24)) return -3;
    if (!writeBuffer(process, g_ringCave, seed.data(), seed.size())) return -4;

    std::vector<HANDLE> suspendedThreads;
    suspendAllThreads(processId, suspendedThreads);
    int referencesPatched = 0, immediatesPatched = 0;
    for (int i = 0; i < g_title->ringReferenceCount; i++) {
        const RingReference& reference = g_title->ringReferences[i];
        uint64_t caveTarget = (reference.which == 0) ? caveHeader : (reference.which == 1) ? caveEntries : caveEntries + 4;
        if (ringPoint(process, base, reference, caveTarget)) referencesPatched++;
    }
    for (int i = 0; i < g_title->ringImmediateCount; i++) {
        const RingImmediate& immediate = g_title->ringImmediates[i];
        uint32_t newValue = (ringImmediateOriginalValue(immediate) == RING_STOCK_SLOTS) ? (uint32_t)slots
                                                     : (uint32_t)slots * 24;
        uint8_t instructionBytes[6]; memcpy(instructionBytes, immediate.originalBytes, immediate.length);
        if (immediate.immediateSize == 1) instructionBytes[immediate.immediateOffset] = (uint8_t)newValue; else memcpy(instructionBytes + immediate.immediateOffset, &newValue, 4);
        if (writeCode(process, base + immediate.moduleOffset, instructionBytes, immediate.length)) immediatesPatched++;
    }
    resumeAllThreads(suspendedThreads);
    g_ringRelocated = true;
    return (referencesPatched == g_title->ringReferenceCount && immediatesPatched == g_title->ringImmediateCount) ? slots : -5;
}

static void ringRevert(HANDLE process, DWORD processId, uint64_t base) {
    if (!g_ringRelocated || !g_title->ringReferenceCount || !g_ringCave) return;
    const uint64_t header = base + g_title->keyframeTable - 8;
    std::vector<uint8_t> back((size_t)8 + RING_STOCK_SLOTS * 24, 0);
    readBuffer(process, g_ringCave, back.data(), back.size());
    std::vector<HANDLE> suspendedThreads;
    suspendAllThreads(processId, suspendedThreads);
    writeBuffer(process, header, back.data(), back.size());
    for (int i = 0; i < g_title->ringReferenceCount; i++) {
        const RingReference& reference = g_title->ringReferences[i];
        ringPoint(process, base, reference, (reference.which == 0) ? header : (reference.which == 1) ? header + 8 : header + 12);
    }
    for (int i = 0; i < g_title->ringImmediateCount; i++)
        writeCode(process, base + g_title->ringImmediates[i].moduleOffset, g_title->ringImmediates[i].originalBytes, g_title->ringImmediates[i].length);
    resumeAllThreads(suspendedThreads);
    g_ringRelocated = false;
}

#define MAX_POPUP_SITES 128
static uint8_t g_popupOriginalBytes[MAX_POPUP_SITES][5];
static bool    g_popupSaved = false;
static int popupApply(HANDLE process, uint64_t base) {
    if (g_title->popupSiteCount <= 0) return -1;
    for (int i = 0; i < g_title->popupSiteCount; i++) {
        uint8_t currentBytes[5] = {0};
        if (!readBuffer(process, base + g_title->popupSites[i], currentBytes, 5)) return -1;
        bool isCall = currentBytes[0] == 0xE8;
        bool isNop  = currentBytes[0]==0x90 && currentBytes[1]==0x90 && currentBytes[2]==0x90 && currentBytes[3]==0x90 && currentBytes[4]==0x90;
        if (!isCall && !isNop) return -1;
    }
    if (!g_popupSaved) {
        bool allCalls = true;
        for (int i = 0; i < g_title->popupSiteCount; i++) {
            uint8_t currentBytes[5] = {0};
            if (!readBuffer(process, base + g_title->popupSites[i], currentBytes, 5) || currentBytes[0] != 0xE8) allCalls = false;
        }
        if (allCalls) {
            for (int i = 0; i < g_title->popupSiteCount; i++)
                readBuffer(process, base + g_title->popupSites[i], g_popupOriginalBytes[i], 5);
            g_popupSaved = true;
        }
    }
    const uint8_t nopBytes[5] = {0x90,0x90,0x90,0x90,0x90};
    int patchedCount = 0;
    for (int i = 0; i < g_title->popupSiteCount; i++)
        if (writeCode(process, base + g_title->popupSites[i], nopBytes, 5)) patchedCount++;
    return patchedCount;
}
static void popupRevert(HANDLE process, uint64_t base) {
    if (!g_popupSaved) return;
    for (int i = 0; i < g_title->popupSiteCount; i++)
        writeCode(process, base + g_title->popupSites[i], g_popupOriginalBytes[i], 5);
}

static uint8_t g_clipLengthOriginalBytes[6];
static bool    g_clipLengthPatched = false;
static bool clipLengthApply(HANDLE process, uint64_t base, int32_t ticks) {
    if (!g_title->clipLengthPatch) return false;
    uint8_t currentBytes[6] = {0};
    if (!readBuffer(process, base + g_title->clipLengthPatch, currentBytes, 6)) return false;
    bool stock = (currentBytes[0] == 0x8B && currentBytes[1] == 0x0D);
    bool done  = (currentBytes[0] == 0xB9 && currentBytes[5] == 0x90);
    if (!stock && !done) return false;
    if (stock) memcpy(g_clipLengthOriginalBytes, currentBytes, 6);
    uint8_t patchBytes[6] = { 0xB9, 0, 0, 0, 0, 0x90 };
    memcpy(patchBytes + 1, &ticks, 4);
    if (!writeCode(process, base + g_title->clipLengthPatch, patchBytes, 6)) return false;
    g_clipLengthPatched = true;
    return true;
}
static void clipLengthRevert(HANDLE process, uint64_t base) {
    if (g_clipLengthPatched && g_title->clipLengthPatch)
        writeCode(process, base + g_title->clipLengthPatch, g_clipLengthOriginalBytes, 6);
}

static void clipDiagnostics(HANDLE process, uint64_t base, const char* why) {
    auto readInt32 = [&](uint32_t moduleOffset) { int32_t value = 0; if (moduleOffset) readBuffer(process, base + moduleOffset, &value, 4); return value; };
    auto readUInt64 = [&](uint32_t moduleOffset) { uint64_t value = 0; if (moduleOffset) readBuffer(process, base + moduleOffset, &value, 8); return value; };
    auto readByte  = [&](uint32_t moduleOffset) { uint8_t  value = 0; if (moduleOffset) readBuffer(process, base + moduleOffset, &value, 1); return value; };

    int32_t tick = readInt32(g_title->tick), currentState = readInt32(g_title->state);
    int32_t clipStartTick = readInt32(g_title->clipStart), clipEndTick = readInt32(g_title->clipEnd), budget = readInt32(g_title->clipMaxLength);
    float currentSpeed = 0.0f; readBuffer(process, base + g_title->speed, &currentSpeed, 4);

    printf(COLOR_BOLD "\n  ---- clip diagnostics (%s) ----\n" COLOR_RESET, why);
    printf("   film     tick %-8d %7.1fs   speed %.2fx   state %d\n", tick, tick / 30.0, currentSpeed, currentState);
    printf("   clip     start %-8d end %-8d budget %d\n", clipStartTick, clipEndTick, budget);

    if (g_title->clipLengthPatch) {
        uint8_t budgetBytes[6] = {0};
        readBuffer(process, base + g_title->clipLengthPatch, budgetBytes, 6);
        bool patched = (budgetBytes[0] == 0xB9 && budgetBytes[5] == 0x90);
        int32_t budgetImmediate = 0; memcpy(&budgetImmediate, budgetBytes + 1, 4);
        printf("   budget load +0x%-7X %02X %02X %02X %02X %02X %02X  %s%s\n" COLOR_RESET,
               g_title->clipLengthPatch, budgetBytes[0], budgetBytes[1], budgetBytes[2], budgetBytes[3], budgetBytes[4], budgetBytes[5],
               patched ? COLOR_GREEN : COLOR_RED,
               patched ? "PATCHED" : "NOT PATCHED, the stop window can never pass");
        if (patched) printf("   effective budget = %d ticks (%.0fs)\n", budgetImmediate, budgetImmediate / 30.0);
    }

    int32_t effectiveBudget = budget;
    if (g_title->clipLengthPatch) {
        uint8_t budgetBytes[6] = {0}; readBuffer(process, base + g_title->clipLengthPatch, budgetBytes, 6);
        if (budgetBytes[0] == 0xB9) memcpy(&effectiveBudget, budgetBytes + 1, 4);
    }
    bool windowPasses = (tick >= 0 && tick >= clipStartTick && (clipStartTick + effectiveBudget - tick) >= 0);
    int32_t writerState = readInt32(g_title->filmWriterState), gate = g_title->cameraBlobObject ? readInt32(g_title->cameraBlobObject + 140) : -1;
    uint8_t writerError = readByte(g_title->filmWriterError);
    uint64_t blobHandle = readUInt64(g_title->cameraBlobHandle);
    int32_t blobMode = readInt32(g_title->cameraBlobMode);
    printf("   [1] window   %s  tick>=0 && tick>=start && start+budget-tick>=0\n"
           "                    -> %d >= %d && %d + %d - %d = %d\n",
           windowPasses ? COLOR_GREEN "PASS" COLOR_RESET : COLOR_RED "FAIL" COLOR_RESET, tick, clipStartTick, clipStartTick, effectiveBudget, tick, clipStartTick + effectiveBudget - tick);
    printf("   [2] writer   %s  sub_180107E04: state %d (want 0/1), err %u (want 0)\n",
           (writerState == 0 || writerState == 1) ? COLOR_GREEN "PASS" COLOR_RESET : COLOR_RED "FAIL" COLOR_RESET, writerState, writerError);
    printf("   [3] camblob  %s  sub_1801CFA0C: dword+140 = %d (want exactly 1)\n",
           gate == 1 ? COLOR_GREEN "PASS" COLOR_RESET : COLOR_RED "FAIL" COLOR_RESET, gate);
    printf("   camblob  handle 0x%llX  mode %d\n", (unsigned long long)blobHandle, blobMode);
    printf(COLOR_GRAY "   note: the engine returns at the FIRST failing condition, so a later\n"
           "   line reading -1 may simply mean it was never reached.\n" COLOR_RESET);

    if (g_title->keyframeTable) {
        const int slotCount = ringSlotCount();
        std::vector<uint8_t> keyframeBytes((size_t)slotCount * g_title->keyframeStride);
        if (readBuffer(process, ringEntriesAddress(base), keyframeBytes.data(), keyframeBytes.size())) {
            int live = 0;
            for (int i = 0; i < slotCount; i++)
                if (keyframeBytes[(size_t)i * g_title->keyframeStride + g_title->keyframeValidOffset]) live++;
            printf("   keyframes %d/%d valid   err flag %u code %d\n\n",
                   live, slotCount, readByte(g_title->speed + 0x4C), readInt32(g_title->speed + 0x50));
        }
    }
}

typedef NTSTATUS (NTAPI *NtQuerySystemInformationFunction)(ULONG, PVOID, ULONG, PULONG);
#define SystemExtendedHandleInformation 64

struct SystemHandleEntry {
    PVOID  Object;
    ULONG_PTR UniqueProcessId;
    HANDLE HandleValue;
    ULONG  GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG  HandleAttributes;
    ULONG  Reserved;
};
struct SystemHandleInformationEx {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SystemHandleEntry Handles[1];
};

static int releaseClipHandles(HANDLE process, DWORD processId) {
    NtQuerySystemInformationFunction ntQuerySystemInformation = (NtQuerySystemInformationFunction)GetProcAddress(GetModuleHandleA("ntdll.dll"),
                                            "NtQuerySystemInformation");
    if (!ntQuerySystemInformation) { printf(COLOR_RED "[F2] NtQuerySystemInformation unavailable\n" COLOR_RESET); return -1; }

    ULONG capacity = 1 << 20;
    std::vector<uint8_t> buffer;
    NTSTATUS status;
    for (;;) {
        buffer.assign(capacity, 0);
        ULONG requiredSize = 0;
        status = ntQuerySystemInformation(SystemExtendedHandleInformation, buffer.data(), capacity, &requiredSize);
        if (status == (NTSTATUS)0xC0000004L  ) { capacity *= 2; continue; }
        break;
    }
    if (status < 0) { printf(COLOR_RED "[F2] handle enumeration failed (0x%lX)\n" COLOR_RESET, (unsigned long)status); return -1; }

    auto* handleInfo = (SystemHandleInformationEx*)buffer.data();
    int closedCount = 0, inspectedCount = 0;
    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; i++) {
        const SystemHandleEntry& entry = handleInfo->Handles[i];
        if ((DWORD)entry.UniqueProcessId != processId) continue;
        HANDLE duplicate = nullptr;
        if (!DuplicateHandle(process, entry.HandleValue, GetCurrentProcess(), &duplicate, 0, FALSE,
                             DUPLICATE_SAME_ACCESS))
            continue;
        if (GetFileType(duplicate) == FILE_TYPE_DISK) {
            char path[MAX_PATH] = {0};
            DWORD pathLength = GetFinalPathNameByHandleA(duplicate, path, MAX_PATH, FILE_NAME_NORMALIZED);
            if (pathLength && pathLength < MAX_PATH) {
                inspectedCount++;
                const char* fileName = strrchr(path, '\\');
                fileName = fileName ? fileName + 1 : path;
                if (_stricmp(fileName, "sf_camera.blob") == 0 || _stricmp(fileName, "sf_snippet.film") == 0) {
                    HANDLE sourceHandle = nullptr;
                    if (DuplicateHandle(process, entry.HandleValue, GetCurrentProcess(), &sourceHandle, 0, FALSE,
                                        DUPLICATE_CLOSE_SOURCE)) {
                        if (sourceHandle) CloseHandle(sourceHandle);
                        closedCount++;
                        printf(COLOR_GREEN "[F2] closed leaked handle" COLOR_RESET COLOR_GRAY "  %s\n" COLOR_RESET, fileName);
                    }
                }
            }
        }
        CloseHandle(duplicate);
    }
    printf(closedCount ? COLOR_GREEN "[F2] released %d handle(s), F5 should work again\n" COLOR_RESET
                  : COLOR_YELLOW "[F2] no leaked clip handles found (inspected %d file handles)\n" COLOR_RESET,
           closedCount ? closedCount : inspectedCount);
    return closedCount;
}

struct Teardown { HANDLE process; DWORD processId; uint64_t base, hookAddress, cave; bool filmErrorBlockingOn; bool active;
                  bool paused; float previousSpeed;
                  bool clipLengthSeeded; int32_t previousClipLength; };
static Teardown     g_teardown = {};
static volatile LONG g_cleaned = 0;

static void doCleanup() {
    if (InterlockedExchange(&g_cleaned, 1) != 0) return;
    if (!g_teardown.active) return;
    HANDLE process = g_teardown.process; uint64_t base = g_teardown.base;
    char oldHex[64], newHex[64];
    printf("\n" COLOR_BOLD "[*] reverting memory:\n" COLOR_RESET);

    if (g_teardown.filmErrorBlockingOn) {
        for (int i = 0; i < g_title->filmErrorSiteCount; i++) {
            uint8_t currentBytes[7] = {0};
            readBuffer(process, base + g_title->filmErrorSites[i].moduleOffset, currentBytes, g_title->filmErrorSites[i].length);
            printf(COLOR_GRAY "      film-err [%2d] +0x%-8X 0x%llX  %-20s -> %s\n" COLOR_RESET,
                   i + 1, g_title->filmErrorSites[i].moduleOffset, (unsigned long long)(base + g_title->filmErrorSites[i].moduleOffset),
                   formatHexBytes(oldHex, 64, currentBytes, g_title->filmErrorSites[i].length),
                   formatHexBytes(newHex, 64, g_title->filmErrorSites[i].originalBytes, g_title->filmErrorSites[i].length));
        }
        filmErrorRevert(process, base);
    }

    if (g_popupSaved) {
        for (int i = 0; i < g_title->popupSiteCount; i++) {
            uint8_t currentBytes[5] = {0};
            readBuffer(process, base + g_title->popupSites[i], currentBytes, 5);
            printf(COLOR_GRAY "      popup    [%2d] +0x%-8X 0x%llX  %-20s -> %s\n" COLOR_RESET,
                   i + 1, g_title->popupSites[i], (unsigned long long)(base + g_title->popupSites[i]),
                   formatHexBytes(oldHex, 64, currentBytes, 5), formatHexBytes(newHex, 64, g_popupOriginalBytes[i], 5));
        }
        popupRevert(process, base);
    }

    if (g_teardown.paused) {
        float now = -1.0f;
        if (readBuffer(process, base + g_title->speed, &now, 4) && now == 0.0f) {
            writeValue(process, base + g_title->speed, g_teardown.previousSpeed);
            printf(COLOR_GRAY "      speed         +0x%-8X 0x%llX  %-20s -> %.2fx\n" COLOR_RESET,
                   g_title->speed, (unsigned long long)(base + g_title->speed), "0.00x (paused)", g_teardown.previousSpeed);
        }
    }

    if (g_keyframeCaptureOn) {
        keyframeGateSet(process, base, 2);
        printf(COLOR_GRAY "      kf gates      +0x%-8X/+0x%-8X  imm 1 -> 2\n" COLOR_RESET,
               g_title->keyframeGates[0], g_title->keyframeGates[1]);
    }

    if (g_ringRelocated) {
        ringRevert(process, g_teardown.processId, base);
        printf(COLOR_GRAY "      kf ring       %d -> %d slots  (%d refs + %d bounds restored,\n"
               "                    header + slots 0..15 carried back)\n" COLOR_RESET,
               g_ringSlotCount, RING_STOCK_SLOTS, g_title->ringReferenceCount, g_title->ringImmediateCount);
    }

    if (g_clipLengthPatched) {
        clipLengthRevert(process, base);
        char originalHex[64];
        printf(COLOR_GRAY "      clip budget   +0x%-8X 0x%llX  mov ecx,imm32;nop -> %s\n" COLOR_RESET,
               g_title->clipLengthPatch, (unsigned long long)(base + g_title->clipLengthPatch),
               formatHexBytes(originalHex, 64, g_clipLengthOriginalBytes, 6));
    }

    if (g_teardown.clipLengthSeeded && g_title->clipMaxLength) {
        writeValue(process, base + g_title->clipMaxLength, g_teardown.previousClipLength);
        printf(COLOR_GRAY "      clip len      +0x%-8X 0x%llX  %-20s -> %d\n" COLOR_RESET,
               g_title->clipMaxLength, (unsigned long long)(base + g_title->clipMaxLength),
               "(seeded)", g_teardown.previousClipLength);
    }

    {
        uint8_t currentBytes[8] = {0};
        readBuffer(process, g_teardown.hookAddress, currentBytes, g_title->hookLength);
        DWORD oldProtection;
        VirtualProtectEx(process, (void*)g_teardown.hookAddress, g_title->hookLength, PAGE_EXECUTE_READWRITE, &oldProtection);
        writeBuffer(process, g_teardown.hookAddress, g_title->hookOriginalBytes, g_title->hookLength);
        VirtualProtectEx(process, (void*)g_teardown.hookAddress, g_title->hookLength, oldProtection, &oldProtection);
        FlushInstructionCache(process, (void*)g_teardown.hookAddress, g_title->hookLength);
        printf(COLOR_GRAY "      hook          +0x%-8X 0x%llX  %-20s -> %s\n" COLOR_RESET,
               g_title->hook, (unsigned long long)g_teardown.hookAddress,
               formatHexBytes(oldHex, 64, currentBytes, g_title->hookLength), formatHexBytes(newHex, 64, g_title->hookOriginalBytes, g_title->hookLength));
    }

    if (g_title && g_title->closeBlobFunction)
        releaseClipHandles(process, g_teardown.processId);

    VirtualFreeEx(process, (void*)g_teardown.cave, 0, MEM_RELEASE);
    CloseHandle(process);
    printf("\n" COLOR_GREEN "[+] restored + cleaned up (%d film + %d popup + hook + cave freed). bye.\n" COLOR_RESET,
           g_teardown.filmErrorBlockingOn ? g_title->filmErrorSiteCount : 0, g_popupSaved ? g_title->popupSiteCount : 0);
}

static BOOL WINAPI consoleControlHandler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT: case CTRL_BREAK_EVENT: case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT: case CTRL_SHUTDOWN_EVENT:
            doCleanup();
            return TRUE;
        default: return FALSE;
    }
}

#define RUN_RESTART 2

static int run() {
    enableVirtualTerminal();
    enableDebugPrivilege();

    printf(COLOR_GRAY "Waiting for something to happen..?\n");
    printf(COLOR_GRAY "@hybridsego \n");
    printf(COLOR_BOLD "build " __DATE__ " " __TIME__ "\n\n" COLOR_RESET);

    DWORD processId = 0; uint64_t base = 0;
    if (!findModule(processId, base, g_title)) {
        printf(COLOR_GRAY "[*] waiting for a supported game DLL to load");
        for (int i = 0; i < TITLE_COUNT; i++)
            printf("%s%ls", i ? ", " : " (", TITLES[i].module);
        printf(")...\n" COLOR_RESET);
        for (int spin = 0; !findModule(processId, base, g_title); spin++) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                printf(COLOR_YELLOW "[!] cancelled.\n" COLOR_RESET);
                return 1;
            }
            Sleep(100);
        }
        printf(COLOR_GREEN "[+]" COLOR_RESET " module appeared, attaching immediately\n");
    }
    printf(COLOR_GREEN "[+]" COLOR_RESET " " COLOR_BOLD "%s" COLOR_RESET "  (%ls)  pid=%lu  base=0x%llX\n",
           g_title->name, g_title->module, processId, (unsigned long long)base);

    HANDLE process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
                              FALSE, processId);
    if (!process) { printf(COLOR_RED "[!] OpenProcess failed (%lu). Run as Administrator.\n" COLOR_RESET, GetLastError()); return 1; }

    const uint64_t hookAddress  = base + g_title->hook;
    const uint64_t stateAddress = base + g_title->state;
    const uint64_t startFunctionAddress   = base + g_title->startFunction;
    const uint64_t stopFunctionAddress    = base + g_title->stopFunction;
    const uint64_t poolInitFunctionAddress= base + g_title->poolInitFunction;
    const uint64_t skipFunctionAddress    = base + g_title->skipFunction;
    const uint64_t speedAddress = base + g_title->speed;
    const uint64_t tickAddress  = base + g_title->tick;
    const uint64_t saveFunctionAddress= g_title->saveFunction ? base + g_title->saveFunction : 0;

    uint8_t currentBytes[8];
    if (!readBuffer(process, hookAddress, currentBytes, g_title->hookLength)) { printf(COLOR_RED "[!] read hook site failed.\n" COLOR_RESET); CloseHandle(process); return 1; }
    if (memcmp(currentBytes, g_title->hookOriginalBytes, g_title->hookLength) != 0) {
        char oldHex[64], newHex[64];
        printf(COLOR_RED "[!] Prologue at +0x%X != expected (build mismatch or already hooked).\n" COLOR_RESET, g_title->hook);
        printf(COLOR_GRAY "    found %s, expected %s\n" COLOR_RESET,
               formatHexBytes(oldHex, 64, currentBytes, g_title->hookLength), formatHexBytes(newHex, 64, g_title->hookOriginalBytes, g_title->hookLength));
        CloseHandle(process); return 1;
    }

    const SIZE_T caveSize = 0x400;
    uint64_t cave = allocateNear(process, hookAddress, caveSize);
    if (!cave) { printf(COLOR_RED "[!] allocNear failed.\n" COLOR_RESET); CloseHandle(process); return 1; }
    const uint64_t slotAddress   = cave;
    const uint64_t codeAddress   = cave + 32;
    const uint64_t nameBuffer    = cave + 0x200;
    const uint64_t descriptionBuffer    = cave + 0x300;
    const uint64_t returnAddress = hookAddress + g_title->hookLength;

    uint8_t caveCode[0x80]; int caveCodeLength = 0;
    auto emitBytes = [&](std::initializer_list<uint8_t> bytes){ for (uint8_t byteValue : bytes) caveCode[caveCodeLength++] = byteValue; };
    auto emitQword = [&](uint64_t value){ for (int i = 0; i < 8; i++) caveCode[caveCodeLength++] = (uint8_t)(value >> (8*i)); };
    emitBytes({0x9C});
    emitBytes({0x50,0x51,0x52});
    emitBytes({0x41,0x50, 0x41,0x51, 0x41,0x52, 0x41,0x53});
    emitBytes({0x48,0xB8}); emitQword(slotAddress);
    emitBytes({0x4C,0x8B,0x10});
    emitBytes({0x4D,0x85,0xD2});
    emitBytes({0x74,0x1E});
    emitBytes({0x48,0x8B,0x48,0x08});
    emitBytes({0x48,0x8B,0x50,0x10});
    emitBytes({0x4C,0x8B,0x40,0x18});
    emitBytes({0x48,0xC7,0x00, 0x00,0x00,0x00,0x00});
    emitBytes({0x48,0x83,0xEC,0x28});
    emitBytes({0x41,0xFF,0xD2});
    emitBytes({0x48,0x83,0xC4,0x28});
    emitBytes({0x41,0x5B, 0x41,0x5A, 0x41,0x59, 0x41,0x58});
    emitBytes({0x5A,0x59,0x58});
    emitBytes({0x9D});
    for (int i = 0; i < g_title->hookLength; i++) caveCode[caveCodeLength++] = g_title->hookOriginalBytes[i];
    emitBytes({0xFF,0x25,0x00,0x00,0x00,0x00}); emitQword(returnAddress);

    uint8_t zeroSlot[32] = {0};
    if (!writeBuffer(process, slotAddress, zeroSlot, 32) || !writeBuffer(process, codeAddress, caveCode, caveCodeLength)) {
        printf(COLOR_RED "[!] writing cave failed.\n" COLOR_RESET); VirtualFreeEx(process, (void*)cave, 0, MEM_RELEASE); CloseHandle(process); return 1;
    }

    uint8_t patch[8];
    memset(patch, 0x90, sizeof(patch));
    int32_t relativeJump = (int32_t)((int64_t)codeAddress - (int64_t)(hookAddress + 5));
    patch[0] = 0xE9; memcpy(patch + 1, &relativeJump, 4);
    DWORD oldProtection;
    VirtualProtectEx(process, (void*)hookAddress, g_title->hookLength, PAGE_EXECUTE_READWRITE, &oldProtection);
    bool hooked = writeBuffer(process, hookAddress, patch, g_title->hookLength);
    VirtualProtectEx(process, (void*)hookAddress, g_title->hookLength, oldProtection, &oldProtection);
    FlushInstructionCache(process, (void*)hookAddress, g_title->hookLength);
    if (!hooked) { printf(COLOR_RED "[!] hook write failed.\n" COLOR_RESET); VirtualFreeEx(process, (void*)cave, 0, MEM_RELEASE); CloseHandle(process); return 1; }
    printf(COLOR_GREEN "[+]" COLOR_RESET " cave @ 0x%llX   hook @ +0x%X\n", (unsigned long long)cave, g_title->hook);
    printf(COLOR_GREEN "[+]" COLOR_RESET " record       @ 0x%llX  (+0x%X)\n", (unsigned long long)startFunctionAddress, g_title->startFunction);
    printf(COLOR_GREEN "[+]" COLOR_RESET " stop/capture @ 0x%llX  (+0x%X)\n", (unsigned long long)stopFunctionAddress,  g_title->stopFunction);
    printf(COLOR_GREEN "[+]" COLOR_RESET " skip         @ 0x%llX  (+0x%X)\n", (unsigned long long)skipFunctionAddress,  g_title->skipFunction);
    printf(COLOR_GREEN "[+]" COLOR_RESET " speed        @ 0x%llX  (+0x%X)\n", (unsigned long long)speedAddress, g_title->speed);

    g_teardown = { process, processId, base, hookAddress, cave, false, true, false, 1.0f, false, 0 };
    SetConsoleCtrlHandler(consoleControlHandler, TRUE);

    auto request = [&](uint64_t function, uint64_t argument1, uint64_t argument2 = 0, uint64_t argument3 = 0){
        writeValue(process, slotAddress + 8, argument1); writeValue(process, slotAddress + 16, argument2); writeValue(process, slotAddress + 24, argument3);
        writeValue(process, slotAddress, function);
    };
    auto readState = [&]() -> int32_t { int32_t value = -1; readBuffer(process, stateAddress, &value, 4); return value; };
    auto readTick  = [&]() -> int32_t { int32_t value = -1; readBuffer(process, tickAddress,  &value, 4); return value; };
    auto readSpeed = [&]() -> float   { float value = -1.f; readBuffer(process, speedAddress, &value, 4); return value; };

    auto skip = [&](int direction, const char* key) {
        const char* dirName = (direction == 1) ? "BACK" : "FORWARD";
        int32_t startTick = readTick();
        request(skipFunctionAddress, (uint64_t)(uint32_t)direction);
        int32_t endTick = startTick; bool jumped = false;
        for (int i = 0; i < 25 && !jumped; i++) {
            Sleep(10);
            endTick = readTick();
            jumped = (endTick < startTick - 2) || (endTick > startTick + 30);
        }
        if (jumped)
            printf(COLOR_CYAN "[%s] skip %s" COLOR_RESET COLOR_GRAY "  tick %d -> %d  (%.1fs -> %.1fs)\n" COLOR_RESET,
                   key, dirName, startTick, endTick, startTick / 30.0, endTick / 30.0);
        else
            printf(COLOR_YELLOW "[%s] skip %s, no jump in 250 ms" COLOR_RESET COLOR_GRAY
                   " (no keyframe that way, or playback not active; tick %d / %.1fs)\n" COLOR_RESET,
                   key, dirName, endTick, endTick / 30.0);
    };

    const uint64_t seekAddress = base + g_title->speed + 0x0C;
    const uint64_t seekFlagAddress = base + g_title->speed + 0x04;
    auto abortClip = [&]() {
        int32_t stateBefore = readState();
        int32_t none = -1; uint8_t zero = 0;
        writeValue(process, seekAddress, none);
        writeValue(process, seekFlagAddress, zero);
        if (readSpeed() == 0.0f) { writeValue(process, speedAddress, 1.0f); g_teardown.paused = false; }
        if (g_title->closeBlobFunction) {
            request(base + g_title->closeBlobFunction, base + g_title->cameraBlobObject);
            Sleep(90);
            request(base + g_title->closeWriterFunction, base + g_title->filmWriterObject);
            Sleep(90);
            request(base + g_title->closeWriterFunction, base + g_title->filmWriterObject);
            Sleep(90);
        }
        if (g_title->clipReset) request(base + g_title->clipReset, 0);
        else { int32_t zeroState = 0; writeValue(process, stateAddress, zeroState); }
        Sleep(120);
        printf(COLOR_MAGENTA "[F4] abort" COLOR_RESET COLOR_GRAY
               "  state %d -> %d, seek cleared, speed %.2fx\n" COLOR_RESET,
               stateBefore, readState(), readSpeed());
        if (g_title->cameraBlobHandle) {
            uint64_t blobHandle = 0; readBuffer(process, base + g_title->cameraBlobHandle, &blobHandle, 8);
            printf(COLOR_GRAY "     sf_camera.blob handle now 0x%llX %s\n" COLOR_RESET,
                   (unsigned long long)blobHandle,
                   (blobHandle == 0 || blobHandle == ~0ull) ? "(no handle tracked, if F5 still fails, press F2)"
                                          : "(still open)");
        }
    };

    auto keyframes = [&]() {
        const uint32_t slotCount = (uint32_t)ringSlotCount(), stride = g_title->keyframeStride;
        std::vector<uint8_t> buffer(slotCount * stride);
        if (!readBuffer(process, ringEntriesAddress(base), buffer.data(), buffer.size())) {
            printf(COLOR_YELLOW "[F7] couldn't read the keyframe table\n" COLOR_RESET);
            return;
        }
        int32_t now = readTick();
        printf(COLOR_BOLD "[F7] keyframe index" COLOR_RESET COLOR_GRAY
               "  (%s0x%llX, %u slots x %u B)   current tick %d / %.1fs\n" COLOR_RESET,
               g_ringRelocated ? "relocated @ " : "+0x", (unsigned long long)(g_ringRelocated ? ringEntriesAddress(base) : g_title->keyframeTable),
               slotCount, stride, now, now / 30.0);
        int live = 0;
        for (uint32_t i = 0; i < slotCount; i++) {
            const uint8_t* entry = buffer.data() + i * stride;
            int32_t tick; memcpy(&tick, entry + g_title->keyframeTickOffset, 4);
            uint8_t valid = entry[g_title->keyframeValidOffset];
            bool empty = true;
            for (uint32_t byteIndex = 0; byteIndex < stride; byteIndex++)
                if (entry[byteIndex] != 0xFF && entry[byteIndex] != 0x00) empty = false;
            if (!valid && empty) continue;
            if (valid) live++;
            char hexText[80]; int hexLength = 0;
            for (uint32_t byteIndex = 0; byteIndex < stride && hexLength < 72; byteIndex++)
                hexLength += snprintf(hexText + hexLength, sizeof(hexText) - hexLength, "%02X", entry[byteIndex]);
            printf(COLOR_GRAY "      [%2u] tick %-9d %7.1fs %-9s %s\n" COLOR_RESET, i, tick, tick / 30.0,
                   valid ? "VALID" : "picked?", hexText);
        }
        if (live) {
            printf(COLOR_GREEN "      %d/%u slots valid, skip has targets.\n" COLOR_RESET, live, slotCount);
        } else {
            printf(COLOR_YELLOW "      0/%u slots valid, nothing for skip back/forward to jump to.\n"
                   "      Any 'picked?' rows above mean the writer DID allocate a slot but\n"
                   "      the snapshot never completed; no rows at all means it never ran.\n" COLOR_RESET, slotCount);
        }
    };

    int filmErrorPatchedCount = filmErrorApply(process, base);
    bool filmErrorBlockingOn = filmErrorPatchedCount >= 0;
    g_teardown.filmErrorBlockingOn = filmErrorBlockingOn;
    if (filmErrorBlockingOn) {
        printf(COLOR_GREEN "[+]" COLOR_RESET " film-error blocking ON (%d/%d sites):\n", filmErrorPatchedCount, g_title->filmErrorSiteCount);
        for (int i = 0; i < g_title->filmErrorSiteCount; i++) {
            uint8_t nopBytes[7]; memset(nopBytes, 0x90, g_title->filmErrorSites[i].length);
            char oldHex[64], newHex[64];
            printf(COLOR_GRAY "      [%2d] +0x%-8X 0x%llX  %-20s -> %s\n" COLOR_RESET,
                   i + 1, g_title->filmErrorSites[i].moduleOffset, (unsigned long long)(base + g_title->filmErrorSites[i].moduleOffset),
                   formatHexBytes(oldHex, 64, g_title->filmErrorSites[i].originalBytes, g_title->filmErrorSites[i].length),
                   formatHexBytes(newHex, 64, nopBytes, g_title->filmErrorSites[i].length));
        }
    } else {
        printf(COLOR_YELLOW "[!] film-error sites didn't match this build, skipped.\n" COLOR_RESET);
    }
    if (g_title->popupSiteCount > 0) {
        int popupPatchedCount = popupApply(process, base);
        if (popupPatchedCount >= 0) {
            const uint8_t nopBytes[5] = {0x90,0x90,0x90,0x90,0x90};
            printf(COLOR_GREEN "[+]" COLOR_RESET " SAVE-FAILED popup blocking ON (%d/%d sites):\n", popupPatchedCount, g_title->popupSiteCount);
            for (int i = 0; i < g_title->popupSiteCount; i++) {
                char oldHex[64], newHex[64];
                printf(COLOR_GRAY "      [%2d] +0x%-8X 0x%llX  %-20s -> %s\n" COLOR_RESET,
                       i + 1, g_title->popupSites[i], (unsigned long long)(base + g_title->popupSites[i]),
                       formatHexBytes(oldHex, 64, g_popupOriginalBytes[i], 5), formatHexBytes(newHex, 64, nopBytes, 5));
            }
        } else {
            printf(COLOR_YELLOW "[!] SAVE-FAILED popup sites didn't match this build, skipped.\n" COLOR_RESET);
        }
    }

    if (g_title->clipLengthPatch && g_forceBudget) {
        if (clipLengthApply(process, base, g_clipLengthTicks))
            printf(COLOR_GREEN "[+]" COLOR_RESET " clip length budget patched at +0x%X" COLOR_GRAY
                   "  (mov ecx,%d;nop)\n"
                   "      the engine pins the real field at 0, which would make F6 impossible\n" COLOR_RESET,
                   g_title->clipLengthPatch, g_clipLengthTicks);
        else
            printf(COLOR_YELLOW "[!] clip length budget site didn't match, F6 will likely refuse.\n" COLOR_RESET);
    }

    if (g_keyframeCaptureEnabled && g_title->keyframeGates[0]) {
        if (keyframeGateSet(process, base, 1) == 3) {
            g_keyframeCaptureOn = true;
            printf(COLOR_GREEN "[+]" COLOR_RESET " keyframe capture ON" COLOR_GRAY
                   "  (3 gates 2->1), campaign films capture nothing\n"
                   "      without this, which breaks rewind AND clip recording. -nokf disables.\n" COLOR_RESET);
        } else {
            printf(COLOR_YELLOW "[!] keyframe capture gates didn't match, rewind/clips won't work.\n" COLOR_RESET);
        }
    }

    if (g_ringSlotCount) {
        if (!g_title->ringReferenceCount) {
            printf(COLOR_YELLOW "[!] -slots isn't mapped for %s, ring left at stock.\n" COLOR_RESET, g_title->name);
        } else {
            g_ringCave = allocateNear(process, base + g_title->keyframeTable, 0x1000);
            int ringResult = g_ringCave ? ringApply(process, processId, base, g_ringSlotCount) : -2;
            if (ringResult == g_ringSlotCount) {
                printf(COLOR_GREEN "[+]" COLOR_RESET " keyframe ring %d -> %d slots" COLOR_GRAY
                       "  @ 0x%llX (%d refs + %d bounds)\n"
                       "      rewind reach ~%d min at the 600-tick grid; scratch file grows to ~%d MB\n" COLOR_RESET,
                       RING_STOCK_SLOTS, g_ringSlotCount, (unsigned long long)g_ringCave,
                       g_title->ringReferenceCount, g_title->ringImmediateCount,
                       (g_ringSlotCount * 600) / (30 * 60), (int)((int64_t)g_ringSlotCount * 0x990000 / (1024 * 1024)));
            } else if (ringResult <= -1000 && ringResult > -2000) {
                printf(COLOR_RED "[!] ring reference %d (+0x%X) didn't resolve where expected, "
                       "NOT relocating.\n" COLOR_RESET, -ringResult - 1000, g_title->ringReferences[-ringResult - 1000].moduleOffset);
            } else if (ringResult <= -2000) {
                printf(COLOR_RED "[!] ring bound %d (+0x%X) didn't match, NOT relocating.\n" COLOR_RESET,
                       -ringResult - 2000, g_title->ringImmediates[-ringResult - 2000].moduleOffset);
            } else {
                printf(COLOR_RED "[!] ring resize failed (%d), ring left at stock.\n" COLOR_RESET, ringResult);
            }
        }
    }

    if (g_title->saveFunction) {
        uint64_t pool = 0; readBuffer(process, base + g_title->pool, &pool, 8);
        if (pool == 0) {
            request(poolInitFunctionAddress, 0);
            for (int i = 0; i < 40 && pool == 0; i++) { Sleep(25); readBuffer(process, base + g_title->pool, &pool, 8); }
        }
        if (pool) printf(COLOR_GREEN "[+]" COLOR_RESET " save pool ready @ 0x%llX\n", (unsigned long long)pool);
        else      printf(COLOR_YELLOW "[!] save pool still NULL, enter a Theater film, save may fail.\n" COLOR_RESET);
    } else {
        uint64_t pool = 0; readBuffer(process, base + g_title->pool, &pool, 8);
        printf(COLOR_GRAY "[*] storage pool %s (left alone, only Reach's F12 needs it)\n" COLOR_RESET,
               pool ? "already provisioned by the engine" : "NULL");
    }

    printf("\n  " COLOR_BOLD COLOR_CYAN "F4" COLOR_RESET "  = abort/unstick  " COLOR_GRAY "(clear seek + reset clip state)" COLOR_RESET);
    printf("\n  " COLOR_BOLD COLOR_CYAN "F5" COLOR_RESET "  = record");
    printf("\n  " COLOR_BOLD COLOR_CYAN "F6" COLOR_RESET "  = stop/capture");
    printf("\n  " COLOR_BOLD COLOR_CYAN "F7" COLOR_RESET "  = keyframe index " COLOR_GRAY "(what skip can jump to)" COLOR_RESET);
    printf("\n  " COLOR_BOLD COLOR_CYAN "F8" COLOR_RESET "  = pause / play");
    printf("\n  " COLOR_BOLD COLOR_CYAN "F9" COLOR_RESET "  = skip back      " COLOR_GRAY "(prev keyframe, ~10 s)" COLOR_RESET);
    printf("\n  " COLOR_BOLD COLOR_CYAN "F10" COLOR_RESET " = skip forward   " COLOR_GRAY "(next keyframe, ~10 s)" COLOR_RESET);
    if (g_keyframeCaptureEnabled && g_title->keyframeGates[0])
        printf("\n  " COLOR_BOLD COLOR_CYAN "F3" COLOR_RESET "  = keyframe capture toggle " COLOR_GRAY
               "(ON by default; needed for rewind AND clips)" COLOR_RESET);
    printf("\n  " COLOR_BOLD COLOR_CYAN "ESC" COLOR_RESET " = quit\n");
    printf(COLOR_GRAY "\n[*] (start recording while the film is PLAYING; saved .film lands in the game root folder)\n\n" COLOR_RESET);

    bool f1WasDown=false, f2WasDown=false, f3WasDown=false, f4WasDown=false, f5WasDown=false, f6WasDown=false, f7WasDown=false, f8WasDown=false, f9WasDown=false, f10WasDown=false, f11WasDown=false, f12WasDown=false;
    int lastState = -99;
    int keyframePollCounter = 0, lastValidCount = -1, lastKeyframeError = 0;
    uint64_t lastScratchFilePointer = 0;
    bool sawScratchFile = false;
    bool keyframeCaptureSuspended = false;
    int32_t lastErrorCode = 0; DWORD lastErrorTime = 0; int errorRepeatCount = 0;
    int moduleCheckCounter = 0, patchCheckCounter = 0;
    uint32_t lastRingChecksum = 0;
    bool warnedGateReverted = false;
    std::vector<uint8_t> previousRing;
    for (;;) {
        bool f4Down  = (GetAsyncKeyState(VK_F4)  & 0x8000) != 0;
        bool f5Down  = (GetAsyncKeyState(VK_F5)  & 0x8000) != 0;
        bool f6Down  = (GetAsyncKeyState(VK_F6)  & 0x8000) != 0;
        bool f7Down  = (GetAsyncKeyState(VK_F7)  & 0x8000) != 0;
        bool f8Down  = (GetAsyncKeyState(VK_F8)  & 0x8000) != 0;
        bool f9Down  = (GetAsyncKeyState(VK_F9)  & 0x8000) != 0;
        bool f10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
        bool f1Down  = (GetAsyncKeyState(VK_F1)  & 0x8000) != 0;
        bool f2Down  = (GetAsyncKeyState(VK_F2)  & 0x8000) != 0;
        bool f3Down  = (GetAsyncKeyState(VK_F3)  & 0x8000) != 0;
        bool f12Down = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
        bool escapeDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

        int32_t currentState = readState();
        if (currentState != lastState) {
            int32_t pendingSeek = -1; readBuffer(process, seekAddress, &pendingSeek, 4);
            printf(COLOR_GRAY "    state = %-3d  tick %d (%.1fs)  speed %.2fx%s\n" COLOR_RESET,
                   currentState, readTick(), readTick() / 30.0, readSpeed(),
                   pendingSeek != -1 ? "  [seek pending]" : "");
            lastState = currentState;
        }

        if (f4Down && !f4WasDown) abortClip();

        if (f5Down && !f5WasDown) {
            if (currentState != 0) {
                printf(COLOR_YELLOW "[F5] ignored (state %d, must be 0/idle), F4 aborts a stuck clip\n" COLOR_RESET, currentState);
            } else {
                float before = readSpeed();
                request(startFunctionAddress, 0);
                int32_t newState = -99;
                for (int i = 0; i < 40; i++) {
                    Sleep(25);
                    newState = readState();
                    int32_t pendingSeek = -1; readBuffer(process, seekAddress, &pendingSeek, 4);
                    if (newState != 0 && pendingSeek == -1) break;
                }
                if (newState == 0) {
                    printf(COLOR_YELLOW "[F5] record-start didn't take (still state 0).\n" COLOR_RESET);
                    if (g_title->cameraBlobHandle) {
                        uint64_t blobHandle = 0; int32_t blobMode = -1, writerState = -1;
                        readBuffer(process, base + g_title->cameraBlobHandle, &blobHandle, 8);
                        readBuffer(process, base + g_title->cameraBlobMode, &blobMode, 4);
                        readBuffer(process, base + g_title->filmWriterState, &writerState, 4);
                        bool blobOk = (blobHandle != 0 && blobHandle != ~0ull);
                        bool writerOk = (writerState == 0 || writerState == 1);
                        printf(COLOR_GRAY "     step 1 open sf_camera.blob : %s (handle 0x%llX, mode %d)\n"
                               "     step 2 create film writer  : %s (state %d)\n"
                               "     step 3 seek to current tick: %s\n" COLOR_RESET,
                               blobOk ? "OK" : "FAILED", (unsigned long long)blobHandle, blobMode,
                               writerOk ? "OK" : "not open", writerState,
                               (blobOk && writerOk) ? "the one that failed" : "not reached");
                        if (!blobOk)
                            printf(COLOR_YELLOW "     sf_camera.blob is still held open by an earlier attempt.\n"
                                   "     The engine opens it ShareMode:None, so CreateFile returns a\n"
                                   "     SHARING VIOLATION, and on failure it overwrites the object's\n"
                                   "     handle with -1, so the real OS handle is ORPHANED and no engine\n"
                                   "     close can reach it.  >> Press F2, then F5.\n" COLOR_RESET);
                    }
                    clipDiagnostics(process, base, "F5 refused");
                } else {
                    float now = readSpeed();
                    if (now == 0.0f) {
                        float back = (before > 0.0f) ? before : 1.0f;
                        writeValue(process, speedAddress, back);
                        g_teardown.paused = false;
                        printf(COLOR_CYAN "[F5] record" COLOR_RESET COLOR_GRAY "  (state %d; resumed %.2fx, "
                               "a clip only advances while playing)\n" COLOR_RESET, newState, back);
                    } else {
                        printf(COLOR_CYAN "[F5] record" COLOR_RESET COLOR_GRAY "  (state %d, %.2fx)\n" COLOR_RESET, newState, now);
                    }
                    if (newState == 1) {
                        int live = 0, slotCount = ringSlotCount();
                        std::vector<uint8_t> keyframeBytes((size_t)slotCount * g_title->keyframeStride);
                        if (readBuffer(process, ringEntriesAddress(base), keyframeBytes.data(), keyframeBytes.size()))
                            for (int i = 0; i < slotCount; i++)
                                if (keyframeBytes[(size_t)i * g_title->keyframeStride + g_title->keyframeValidOffset]) live++;
                        if (live == 0)
                            printf(COLOR_YELLOW "     no keyframes exist (0/%d), recording CANNOT start.\n" COLOR_RESET
                                   COLOR_GRAY "     The clip only leaves state 1 when a keyframe is applied\n"
                                   "     (sub_1801CE468 -> sub_1800F2364). Enable capture first:\n"
                                   "     run with -kf, press F3, let the film play past a 600-tick\n"
                                   "     boundary until F7 shows valid slots, THEN F5.\n" COLOR_RESET,
                                   slotCount);
                        else
                            printf(COLOR_GRAY "     %d/%d keyframes exist; waiting for one to be applied to\n"
                                   "     move the clip out of state 1.\n" COLOR_RESET, live, slotCount);
                    }
                }
            }
        }
        if (f6Down && !f6WasDown) {
            if (currentState >= 1 && currentState < 4) {
                if (g_title->clipMaxLength && !g_title->clipLengthPatch) {
                    int32_t currentBudget = 0; readBuffer(process, base + g_title->clipMaxLength, &currentBudget, 4);
                    if (currentBudget <= 0) {
                        if (!g_teardown.clipLengthSeeded) { g_teardown.previousClipLength = currentBudget; g_teardown.clipLengthSeeded = true; }
                        bool ok = writeValue(process, base + g_title->clipMaxLength, g_clipLengthTicks);
                        int32_t back = -12345; readBuffer(process, base + g_title->clipMaxLength, &back, 4);
                        printf(COLOR_GRAY "     budget %d -> wrote %d (%s), reads back %d%s\n" COLOR_RESET,
                               currentBudget, g_clipLengthTicks, ok ? "ok" : "WRITE FAILED", back,
                               back == g_clipLengthTicks ? "" : "  <- the engine is re-zeroing it");
                    }
                }
                if (g_title->filmWriterState) {
                    int32_t writerState = -1; readBuffer(process, base + g_title->filmWriterState, &writerState, 4);
                    if (writerState != 0 && writerState != 1) {
                        int32_t one = 1;
                        writeValue(process, base + g_title->filmWriterState, one);
                        int32_t readBack = -1; readBuffer(process, base + g_title->filmWriterState, &readBack, 4);
                        printf(COLOR_GRAY "     writer state was %d (nothing in the engine ever sets it) "
                               "-> wrote 1, reads %d\n" COLOR_RESET, writerState, readBack);
                    }
                }
                if (g_title->cameraBlobObject) {
                    int32_t gate = -1; readBuffer(process, base + g_title->cameraBlobObject + 140, &gate, 4);
                    if (gate != 1) {
                        int32_t one = 1;
                        writeValue(process, base + g_title->cameraBlobObject + 140, one);
                        int32_t readBack = -1; readBuffer(process, base + g_title->cameraBlobObject + 140, &readBack, 4);
                        printf(COLOR_GRAY "     camblob +140 was %d (mode-2 open never sets it) "
                               "-> wrote 1, reads %d\n" COLOR_RESET, gate, readBack);
                    }
                }

                int32_t writerStateBefore = -1, gateBefore = -1; uint8_t writerErrorBefore = 0xFF;
                if (g_title->filmWriterState) readBuffer(process, base + g_title->filmWriterState, &writerStateBefore, 4);
                if (g_title->cameraBlobObject)      readBuffer(process, base + g_title->cameraBlobObject + 140, &gateBefore, 4);
                if (g_title->filmWriterError)   readBuffer(process, base + g_title->filmWriterError, &writerErrorBefore, 1);

                request(stopFunctionAddress, 0);
                int32_t newState = currentState;
                for (int i = 0; i < 30 && newState == currentState; i++) { Sleep(25); newState = readState(); }
                if (newState == 4) {
                    printf(COLOR_CYAN "[F6] stop/capture" COLOR_RESET COLOR_GRAY "  (state %d, clip is ready)\n" COLOR_RESET, newState);
                    if (readSpeed() != 0.0f) {
                        g_teardown.previousSpeed = readSpeed(); g_teardown.paused = true;
                        writeValue(process, speedAddress, 0.0f);
                        printf(COLOR_GRAY "     paused, state 4 requires speed 0 or the engine\n"
                               "     raises film-error 3 every frame. F8 resumes.\n" COLOR_RESET);
                    }
                } else if (newState != currentState) {
                    printf(COLOR_CYAN "[F6] stop/capture" COLOR_RESET COLOR_GRAY "  (state %d)\n" COLOR_RESET, newState);
                } else {
                    printf(COLOR_YELLOW "[F6] stop REFUSED, still state %d.\n" COLOR_RESET, newState);
                    printf(COLOR_BOLD "   BEFORE the stop ran (these are the real preconditions):\n" COLOR_RESET);
                    printf("     writer state  %-4d %s\n     writer err    %-4u %s\n"
                           "     camblob +140  %-4d %s\n",
                           writerStateBefore,  (writerStateBefore == 0 || writerStateBefore == 1) ? COLOR_GREEN "ok, sub_180107E04 got a valid writer" COLOR_RESET
                                                              : COLOR_RED "BAD, needs 0 or 1" COLOR_RESET,
                           writerErrorBefore, writerErrorBefore == 0 ? COLOR_GREEN "ok" COLOR_RESET : COLOR_RED "BAD, must be 0" COLOR_RESET,
                           gateBefore, gateBefore == 1 ? COLOR_GREEN "ok" COLOR_RESET
                                                 : COLOR_RED "BAD, sub_1801CFA0C returns 0 unless this is 1" COLOR_RESET);
                    clipDiagnostics(process, base, "F6 refused, values below are POST-stop");
                    printf(COLOR_GRAY "     F4 aborts and closes the files.\n" COLOR_RESET);
                }
            }
            else printf(COLOR_YELLOW "[F6] ignored (state %d, must be 1..3/recording)\n" COLOR_RESET, currentState);
        }

        if (f7Down && !f7WasDown) keyframes();

        if (f1Down && !f1WasDown) {
            g_holdOpen = !g_holdOpen;
            int32_t endTick = -1; readBuffer(process, base + g_title->speed + 0x1C, &endTick, 4);
            printf(COLOR_CYAN "[F1] hold film open %s" COLOR_RESET COLOR_GRAY "  (end tick currently %d)\n" COLOR_RESET,
                   g_holdOpen ? "ON, end tick forced to -1, no auto-exit" : "OFF", endTick);
        }
        if (g_holdOpen) {
            int32_t endTick = -1; readBuffer(process, base + g_title->speed + 0x1C, &endTick, 4);
            if (endTick != -1) {
                int32_t none = -1;
                writeValue(process, base + g_title->speed + 0x1C, none);
                printf(COLOR_GRAY "[F1] cleared end tick %d (%.1fs), would have exited the film\n" COLOR_RESET,
                       endTick, endTick / 30.0);
            }
        }

        if (f3Down && !f3WasDown) {
            if (!g_keyframeCaptureEnabled) {
                printf(COLOR_YELLOW "[F3] keyframe capture is disabled, it can break the film.\n" COLOR_RESET
                       COLOR_GRAY "     Pass -kf to enable it.\n" COLOR_RESET);
            } else if (!g_title->keyframeGates[0]) {
                printf(COLOR_YELLOW "[F3] capture gates not mapped for %s\n" COLOR_RESET, g_title->name);
            } else if (g_keyframeCaptureOn) {
                if (keyframeGateSet(process, base, 2) == 3) {
                    g_keyframeCaptureOn = false;
                    printf(COLOR_CYAN "[F3] keyframe capture OFF\n" COLOR_RESET);
                } else {
                    printf(COLOR_YELLOW "[F3] could not restore the gates\n" COLOR_RESET);
                }
            } else {
                int patchedCount = keyframeGateSet(process, base, 1);
                if (patchedCount == 3) {
                    g_keyframeCaptureOn = true; lastValidCount = -1; lastScratchFilePointer = 0;
                    sawScratchFile = false; lastKeyframeError = 0; lastRingChecksum = 0;
                    warnedGateReverted = false; previousRing.clear();
                    int32_t currentTick = readTick();
                    int32_t nextBoundary = ((currentTick / 600) + 1) * 600;
                    printf(COLOR_CYAN "[F3] keyframe capture ON" COLOR_RESET COLOR_GRAY
                           "  (3 gates 2->1: 2 in the writer, 1 in the\n"
                           "      caller that publishes the index entry)\n"
                           "      You are at %d; next 600-tick boundary %d (%d ticks / ~%.0fs).\n" COLOR_RESET,
                           currentTick, nextBoundary, nextBoundary - currentTick, (nextBoundary - currentTick) / 30.0);
                    if (readSpeed() == 0.0f)
                        printf(COLOR_YELLOW "      THE FILM IS PAUSED, press F8 or the tick never advances.\n" COLOR_RESET);
                } else {
                    printf(COLOR_YELLOW "[F3] gate sites did not match this build, skipped.\n" COLOR_RESET);
                }
            }
        }

        if (f8Down && !f8WasDown) {
            float currentSpeed = readSpeed();
            if (currentSpeed < 0.0f) {
                printf(COLOR_YELLOW "[F8] could not read playback speed\n" COLOR_RESET);
            } else if (currentSpeed > 0.0f) {
                g_teardown.previousSpeed = currentSpeed; g_teardown.paused = true;
                writeValue(process, speedAddress, 0.0f);
                int32_t currentTick = readTick();
                printf(COLOR_CYAN "[F8] pause" COLOR_RESET COLOR_GRAY "  (was %.2fx, tick %d / %.1fs)\n" COLOR_RESET,
                       currentSpeed, currentTick, currentTick / 30.0);
            } else {
                float back = (g_teardown.previousSpeed > 0.0f) ? g_teardown.previousSpeed : 1.0f;
                writeValue(process, speedAddress, back);
                g_teardown.paused = false;
                printf(COLOR_CYAN "[F8] play" COLOR_RESET COLOR_GRAY "  (%.2fx)\n" COLOR_RESET, back);
            }
        }

        if (f9Down  && !f9WasDown)  skip(1, "F9");
        if (f10Down && !f10WasDown) skip(2, "F10");

        if (f12Down && !f12WasDown) {
            if (!saveFunctionAddress) {
                printf(COLOR_YELLOW "[F12] no direct save entry on %s, use the in-game save flow.\n" COLOR_RESET,
                       g_title->name);
            } else if (currentState == 4) {
                static const wchar_t clipName[] = L"ClippingPrototype";
                static const wchar_t clipDescription[] = L"saved by theater_utils";
                writeBuffer(process, nameBuffer, clipName, sizeof(clipName));
                writeBuffer(process, descriptionBuffer, clipDescription, sizeof(clipDescription));
                int32_t id = 0; readBuffer(process, base + g_title->clipId, &id, 4);
                request(saveFunctionAddress, (uint64_t)(uint32_t)id, nameBuffer, descriptionBuffer);
                printf(COLOR_CYAN "[F12] save (id=%d)\n" COLOR_RESET, id);
            } else {
                printf(COLOR_YELLOW "[F12] ignored (state %d, must be 4/captured)\n" COLOR_RESET, currentState);
            }
        }

        {
            int32_t errorCode = 0;
            readBuffer(process, base + g_title->speed + 0x50, &errorCode, 4);
            if (errorCode != 0) {
                DWORD nowMilliseconds = GetTickCount();
                if (errorCode != lastErrorCode || nowMilliseconds - lastErrorTime > 3000) {
                    if (errorCode == lastErrorCode && errorRepeatCount > 0)
                        printf(COLOR_GRAY "     (previous code %d repeated %d times)\n" COLOR_RESET,
                               lastErrorCode, errorRepeatCount);
                    const char* where = "unmapped";
                    for (int i = 0; i < g_title->errorCodeCount; i++)
                        if (g_title->errorCodes[i].code == (uint8_t)errorCode) { where = g_title->errorCodes[i].where; break; }
                    if (errorCode == 3 && currentState == 4 && readSpeed() != 0.0f) {
                        printf(COLOR_YELLOW "[!] film-error 3: a clip is captured (state 4) and the engine\n"
                               "    wants playback paused. Press F8, or save/discard the clip.\n" COLOR_RESET);
                    } else {
                        printf(COLOR_RED "\n[!] FILM ERROR code %d" COLOR_RESET COLOR_BOLD "  %s\n" COLOR_RESET, errorCode, where);
                        clipDiagnostics(process, base, "film error");
                    }
                    lastErrorCode = errorCode; lastErrorTime = nowMilliseconds; errorRepeatCount = 0;
                } else {
                    errorRepeatCount++;
                }
                int32_t zeroCode = 0;
                writeValue(process, base + g_title->speed + 0x50, zeroCode);
            }
        }

        if (g_title->keyframeGates[0] && g_keyframeCaptureEnabled) {
            bool recording = (currentState != 0);
            if (recording && g_keyframeCaptureOn) {
                if (keyframeGateSet(process, base, 2) == 3) {
                    g_keyframeCaptureOn = false; keyframeCaptureSuspended = true;
                    printf(COLOR_GRAY "[kf] capture suspended while the clip records "
                           "(the engine does the same)\n" COLOR_RESET);
                }
            } else if (!recording && keyframeCaptureSuspended) {
                if (keyframeGateSet(process, base, 1) == 3) {
                    g_keyframeCaptureOn = true; keyframeCaptureSuspended = false;
                    lastRingChecksum = 0; previousRing.clear();
                    printf(COLOR_GRAY "[kf] capture resumed\n" COLOR_RESET);
                }
            }
        }

        if (g_keyframeCaptureOn && ++keyframePollCounter >= 12) {
            keyframePollCounter = 0;
            uint64_t scratchFilePointer = 0; readBuffer(process, ringFilePointerAddress(base), &scratchFilePointer, 8);
            const int slotCount = ringSlotCount();
            std::vector<uint8_t> keyframeBytes((size_t)slotCount * g_title->keyframeStride);
            int live = 0;
            if (readBuffer(process, ringEntriesAddress(base), keyframeBytes.data(), keyframeBytes.size()))
                for (int i = 0; i < slotCount; i++)
                    if (keyframeBytes[(size_t)i * g_title->keyframeStride + g_title->keyframeValidOffset]) live++;

            if (scratchFilePointer && !sawScratchFile) {
                sawScratchFile = true;
                printf(COLOR_GRAY "[kf] scratch file pointer set @ 0x%llX (writer, reader or update -\n"
                       "     not proof of capture; watch for a ring change)\n" COLOR_RESET,
                       (unsigned long long)scratchFilePointer);
            }

            uint32_t checksum = 2166136261u;
            for (size_t i = 0; i < keyframeBytes.size(); i++) { checksum ^= keyframeBytes[i]; checksum *= 16777619u; }
            if (checksum != lastRingChecksum) {
                bool first = previousRing.empty();
                if (first) previousRing.assign(keyframeBytes.begin(), keyframeBytes.end());
                lastRingChecksum = checksum;
                int changed = 0;
                for (int i = 0; i < slotCount; i++) {
                    const uint8_t* entry = keyframeBytes.data() + (size_t)i * g_title->keyframeStride;
                    const uint8_t* previousEntry = previousRing.data() + (size_t)i * g_title->keyframeStride;
                    if (!first && memcmp(entry, previousEntry, g_title->keyframeStride) == 0) continue;
                    if (first) {
                        bool empty = true;
                        for (int byteIndex = 0; byteIndex < g_title->keyframeStride; byteIndex++)
                            if (entry[byteIndex] != 0xFF && entry[byteIndex] != 0x00) empty = false;
                        if (empty) continue;
                    }
                    changed++;
                    int32_t entryTick; memcpy(&entryTick, entry + g_title->keyframeTickOffset, 4);
                    char hexText[80]; int hexLength = 0;
                    for (int byteIndex = 0; byteIndex < g_title->keyframeStride && hexLength < 72; byteIndex++)
                        hexLength += snprintf(hexText + hexLength, sizeof(hexText) - hexLength, "%02X", entry[byteIndex]);
                    printf(COLOR_GREEN "[kf] slot %-2d" COLOR_RESET " tick=%-8d valid=%u  %s\n",
                           i, entryTick, entry[g_title->keyframeValidOffset], hexText);
                }
                if (changed)
                    printf(COLOR_GRAY "     (^ ring changed at film tick %d)\n" COLOR_RESET, readTick());
                previousRing.assign(keyframeBytes.begin(), keyframeBytes.end());
            }
            if (live > 0 && live != lastValidCount)
                printf(COLOR_GREEN "[kf] %d/%d slot%s VALID -> F9 can skip back now\n" COLOR_RESET,
                       live, slotCount, live == 1 ? "" : "s");
            lastScratchFilePointer = scratchFilePointer; lastValidCount = live;

            {
                uint8_t gateBytes[3] = {0, 0, 0};
                bool gatesLive = true;
                for (int i = 0; i < 3; i++) {
                    readBuffer(process, base + g_title->keyframeGates[i] + 3, &gateBytes[i], 1);
                    if (gateBytes[i] != 1) gatesLive = false;
                }
                if (!gatesLive && !warnedGateReverted) {
                    warnedGateReverted = true;
                    printf(COLOR_RED "[kf] the gate patch is NO LONGER APPLIED" COLOR_RESET COLOR_GRAY
                           "  (imm now %u/%u/%u, expected 1/1/1)\n" COLOR_RESET, gateBytes[0], gateBytes[1], gateBytes[2]);
                }
            }

            int32_t errorCode = 0; readBuffer(process, base + g_title->speed + 0x50, &errorCode, 4);
            if (errorCode && errorCode != lastKeyframeError) {
                lastKeyframeError = errorCode;
                printf(COLOR_YELLOW "[kf] film-error code %d raised" COLOR_RESET COLOR_GRAY
                       "  (10 = keyframe writer, 12 = apply keyframe, 13 = seek consume,\n"
                       "      2 = per-frame update, 3 = sim tick)\n" COLOR_RESET, errorCode);
            }
            if (errorCode == 10) {
                if (keyframeGateSet(process, base, 2) == 3) g_keyframeCaptureOn = false;
                int32_t zeroCode = 0; writeValue(process, base + g_title->speed + 0x50, zeroCode);
                printf(COLOR_MAGENTA "[kf] capture FAILED (writer error 10), reverted automatically.\n" COLOR_RESET);
            }
        }

        if (currentState == 12 && g_title->saveOperation) {
            int32_t completed = 0, succeeded = 0;
            readBuffer(process, base + g_title->saveOperation + 5812, &completed, 4);
            readBuffer(process, base + g_title->saveOperation + 5820, &succeeded, 4);
            if (completed != 0 && succeeded == 0) {
                int32_t failedState = 15; writeValue(process, stateAddress, failedState);
                printf(COLOR_MAGENTA "[auto] save failed, recovered (state 15).\n" COLOR_RESET);
            }
        }

        f1WasDown = f1Down; f2WasDown = f2Down; f3WasDown = f3Down; f4WasDown = f4Down; f5WasDown = f5Down; f6WasDown = f6Down; f7WasDown = f7Down;
        f8WasDown = f8Down; f9WasDown = f9Down; f10WasDown = f10Down; f12WasDown = f12Down;
        if (escapeDown) break;

        if (++patchCheckCounter >= 60) {
            patchCheckCounter = 0;
            uint8_t firstSiteByte = 0;
            bool wasGone = g_teardown.filmErrorBlockingOn &&
                           readBuffer(process, base + g_title->filmErrorSites[0].moduleOffset, &firstSiteByte, 1) && firstSiteByte != 0x90;
            if (g_teardown.filmErrorBlockingOn) filmErrorApply(process, base);
            if (g_title->popupSiteCount > 0) popupApply(process, base);
            if (g_keyframeCaptureOn && !keyframeCaptureSuspended && g_title->keyframeGates[0]) keyframeGateSet(process, base, 1);
            if (g_forceBudget && g_title->clipLengthPatch) clipLengthApply(process, base, g_clipLengthTicks);
            if (g_ringRelocated && g_ringSlotCount) {
                int reapplyResult = ringApply(process, processId, base, g_ringSlotCount);
                if (reapplyResult == g_ringSlotCount)
                    printf(COLOR_YELLOW "[!] keyframe ring had reverted, relocated again (%d slots).\n" COLOR_RESET,
                           g_ringSlotCount);
                else if (reapplyResult < 0)
                    printf(COLOR_RED "[!] keyframe ring re-apply failed (%d), ring is STOCK now.\n" COLOR_RESET, reapplyResult);
            }
            if (wasGone) {
                printf(COLOR_YELLOW "\n[!] the game had reset our patches (film session reloaded in\n"
                       "    place), re-applied.\n" COLOR_RESET);
                lastRingChecksum = 0; previousRing.clear(); lastValidCount = -1;
            }
        }

        if (++moduleCheckCounter >= 60) {
            moduleCheckCounter = 0;
            DWORD newProcessId = 0; uint64_t newBase = 0; const Title* newTitle = nullptr;
            bool present = findModule(newProcessId, newBase, newTitle);
            if (!present || newBase != base || newProcessId != processId) {
                printf(COLOR_YELLOW "\n[!] the game DLL was %s, our patches went with it.\n" COLOR_RESET,
                       present ? "reloaded at a new base" : "unloaded");
                g_teardown.active = false;
                CloseHandle(process);
                printf(COLOR_GRAY "    re-attaching...\n\n" COLOR_RESET);
                return RUN_RESTART;
            }
        }
        Sleep(16);
    }

    doCleanup();
    return 0;
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-nokf")) {
            g_keyframeCaptureEnabled = false;
        } else if ((!strcmp(argv[i], "-len") || !strcmp(argv[i], "-budget")) && i + 1 < argc) {
            int seconds = atoi(argv[++i]);
            if (seconds > 0) { g_clipLengthTicks = seconds * 30; g_forceBudget = true; }
        } else if (!strcmp(argv[i], "-slots") && i + 1 < argc) {
            int slotCount = atoi(argv[++i]);
            if (slotCount >= RING_STOCK_SLOTS + 1 && slotCount <= 127) g_ringSlotCount = slotCount;
            else printf(COLOR_YELLOW "[!] -slots must be %d..127 (imm8 limit); ignoring '%s'.\n" COLOR_RESET,
                        RING_STOCK_SLOTS + 1, argv[i]);
        }
    }
    int runResult;
    do {
        g_cleaned    = 0;
        g_teardown         = {};
        g_clipLengthPatched = false;
        g_popupSaved = false;
        g_keyframeCaptureOn       = false;
        g_ringRelocated     = false;
        g_ringCave   = 0;
        runResult = run();
    } while (runResult == RUN_RESTART);

    printf("\nPress Enter to exit...");
    (void)getchar();
    return runResult;
}
