// SekiroTweaks - dinput8.dll proxy that applies FPS unlock, borderless window and
// mouse-only camera auto-rotate removal at game start-up. Patterns and patch bytes
// are ported from uberhalit/SekiroFpsUnlockAndMore (MIT).
//
// Built freestanding (no CRT, no Windows SDK headers) with clang + lld-link.

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef int                i32;
typedef unsigned long long u64;
typedef long long          i64;
typedef unsigned long      DWORD;
typedef int                BOOL;
typedef void*              HANDLE;
typedef void*              HWND;
typedef void*              HMODULE;
typedef wchar_t            WCHAR;
typedef u64                SIZE_T;
typedef i64                LONG_PTR;
typedef u64                UINT_PTR;
typedef long               LONG;
typedef long               HRESULT;

#define WINAPI __stdcall
#define IMP extern "C" __declspec(dllimport)

struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
struct MONITORINFO { DWORD cbSize; RECT rcMonitor; RECT rcWork; DWORD dwFlags; };
struct MSG { HWND hwnd; u32 message; UINT_PTR wParam; LONG_PTR lParam; DWORD time; POINT pt; DWORD lPrivate; };
struct MSLLHOOKSTRUCT { POINT pt; DWORD mouseData; DWORD flags; DWORD time; UINT_PTR dwExtraInfo; };
struct MEMORY_BASIC_INFORMATION {
    void* BaseAddress; void* AllocationBase; DWORD AllocationProtect; u16 PartitionId;
    SIZE_T RegionSize; DWORD State; DWORD Protect; DWORD Type;
};
struct SYSTEM_INFO {
    u16 wProcessorArchitecture; u16 wReserved; DWORD dwPageSize;
    void* lpMinimumApplicationAddress; void* lpMaximumApplicationAddress;
    UINT_PTR dwActiveProcessorMask; DWORD dwNumberOfProcessors; DWORD dwProcessorType;
    DWORD dwAllocationGranularity; u16 wProcessorLevel; u16 wProcessorRevision;
};
struct XINPUT_GAMEPAD { u16 wButtons; u8 bLeftTrigger; u8 bRightTrigger; short sThumbLX, sThumbLY, sThumbRX, sThumbRY; };
struct XINPUT_STATE { DWORD dwPacketNumber; XINPUT_GAMEPAD Gamepad; };

typedef DWORD   (WINAPI *THREADPROC)(void*);
typedef LONG_PTR(WINAPI *HOOKPROC)(int, UINT_PTR, LONG_PTR);
typedef BOOL    (WINAPI *WNDENUMPROC)(HWND, LONG_PTR);
typedef DWORD   (WINAPI *XInputGetState_t)(DWORD, XINPUT_STATE*);

// kernel32
IMP HMODULE WINAPI GetModuleHandleW(const WCHAR*);
IMP DWORD   WINAPI GetModuleFileNameW(HMODULE, WCHAR*, DWORD);
IMP u32     WINAPI GetSystemDirectoryW(WCHAR*, u32);
IMP HMODULE WINAPI LoadLibraryW(const WCHAR*);
IMP void*   WINAPI GetProcAddress(HMODULE, const char*);
IMP HANDLE  WINAPI CreateThread(void*, SIZE_T, THREADPROC, void*, DWORD, DWORD*);
IMP void    WINAPI Sleep(DWORD);
IMP BOOL    WINAPI VirtualProtect(void*, SIZE_T, DWORD, DWORD*);
IMP SIZE_T  WINAPI VirtualQuery(const void*, MEMORY_BASIC_INFORMATION*, SIZE_T);
IMP void*   WINAPI VirtualAlloc(void*, SIZE_T, DWORD, DWORD);
IMP void    WINAPI GetSystemInfo(SYSTEM_INFO*);
IMP u32     WINAPI GetPrivateProfileIntW(const WCHAR*, const WCHAR*, i32, const WCHAR*);
IMP DWORD   WINAPI GetPrivateProfileStringW(const WCHAR*, const WCHAR*, const WCHAR*, WCHAR*, DWORD, const WCHAR*);
IMP HANDLE  WINAPI CreateFileW(const WCHAR*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE);
IMP BOOL    WINAPI WriteFile(HANDLE, const void*, DWORD, DWORD*, void*);
IMP BOOL    WINAPI CloseHandle(HANDLE);
IMP DWORD   WINAPI GetCurrentProcessId();
IMP DWORD   WINAPI GetCurrentThreadId();
IMP HANDLE  WINAPI GetCurrentProcess();
IMP BOOL    WINAPI FlushInstructionCache(HANDLE, const void*, SIZE_T);
IMP BOOL    WINAPI DisableThreadLibraryCalls(HMODULE);
IMP u64     WINAPI GetTickCount64();
IMP DWORD   WINAPI GetFileAttributesW(const WCHAR*);

// user32
IMP BOOL     WINAPI EnumWindows(WNDENUMPROC, LONG_PTR);
IMP DWORD    WINAPI GetWindowThreadProcessId(HWND, DWORD*);
IMP BOOL     WINAPI IsWindowVisible(HWND);
IMP HWND     WINAPI GetWindow(HWND, u32);
IMP LONG_PTR WINAPI GetWindowLongPtrW(HWND, int);
IMP LONG_PTR WINAPI SetWindowLongPtrW(HWND, int, LONG_PTR);
IMP BOOL     WINAPI SetWindowPos(HWND, HWND, int, int, int, int, u32);
IMP BOOL     WINAPI GetWindowRect(HWND, RECT*);
IMP BOOL     WINAPI GetClientRect(HWND, RECT*);
IMP BOOL     WINAPI IsIconic(HWND);
IMP void*    WINAPI MonitorFromWindow(HWND, DWORD);
IMP BOOL     WINAPI GetMonitorInfoW(void*, MONITORINFO*);
IMP HWND     WINAPI GetForegroundWindow();
IMP short    WINAPI GetAsyncKeyState(int);
IMP void*    WINAPI SetWindowsHookExW(int, HOOKPROC, HMODULE, DWORD);
IMP LONG_PTR WINAPI CallNextHookEx(void*, int, UINT_PTR, LONG_PTR);
IMP BOOL     WINAPI GetMessageW(MSG*, HWND, u32, u32);
IMP BOOL     WINAPI AdjustWindowRectEx(RECT*, DWORD, BOOL, DWORD);
extern "C" __declspec(dllimport) int __cdecl wsprintfA(char*, const char*, ...);

// ---------------------------------------------------------------------------
// freestanding runtime bits
extern "C" int _fltused = 0;
extern "C" __attribute__((optnone)) void* memset(void* d, int c, SIZE_T n) { u8* p = (u8*)d; while (n--) *p++ = (u8)c; return d; }
extern "C" __attribute__((optnone)) void* memcpy(void* d, const void* s, SIZE_T n) { u8* a = (u8*)d; const u8* b = (const u8*)s; while (n--) *a++ = *b++; return d; }

static void wcopy(WCHAR* d, const WCHAR* s) { while ((*d++ = *s++)) {} }
static void wcat(WCHAR* d, const WCHAR* s) { while (*d) d++; wcopy(d, s); }
static int  wlen(const WCHAR* s) { int n = 0; while (s[n]) n++; return n; }
static WCHAR wlower(WCHAR c) { return (c >= 'A' && c <= 'Z') ? (WCHAR)(c + 32) : c; }
static bool wieq(const WCHAR* a, const WCHAR* b) { while (*a && wlower(*a) == wlower(*b)) { a++; b++; } return wlower(*a) == wlower(*b); }
static float fabsf_(float f) { return f < 0 ? -f : f; }

// ---------------------------------------------------------------------------
// globals / config
static HMODULE g_self, g_exe;
static WCHAR   g_dir[520], g_ini[560], g_log[560];
static int     cfg_fps = 120, cfg_borderless = 1, cfg_stretch = 1, cfg_hotkey = 0x7A /*F11*/,
               cfg_cammode = 1 /*0=game default,1=mouse only,2=always*/, cfg_log = 1,
               cfg_fov = 0, cfg_camreset = 0, cfg_resW = 0, cfg_resH = 0;

static volatile u8* g_camFlag;      // 1 = auto-rotate suppressed, 0 = vanilla
static volatile bool g_mouseActive = true;
static volatile DWORD g_mouseEvents;
static HWND g_gameWnd;

static void LogRaw(const char* s, int n)
{
    if (!cfg_log) return;
    HANDLE h = CreateFileW(g_log, 4 /*FILE_APPEND_DATA*/, 1 | 2, 0, 4 /*OPEN_ALWAYS*/, 0x80, 0);
    if (h == (HANDLE)-1) return;
    DWORD w; WriteFile(h, s, (DWORD)n, &w, 0); CloseHandle(h);
}
#define LOG(...) do { char _b[512]; int _n = wsprintfA(_b, __VA_ARGS__); _b[_n++]='\r'; _b[_n++]='\n'; LogRaw(_b, _n); } while (0)

static void LoadConfig()
{
    GetModuleFileNameW(g_self, g_dir, 512);
    for (int i = wlen(g_dir) - 1; i >= 0; i--) if (g_dir[i] == '\\' || g_dir[i] == '/') { g_dir[i + 1] = 0; break; }
    wcopy(g_ini, g_dir); wcat(g_ini, L"SekiroTweaks.ini");
    wcopy(g_log, g_dir); wcat(g_log, L"SekiroTweaks.log");

    const WCHAR* S = L"SekiroTweaks";
    cfg_fps        = (int)GetPrivateProfileIntW(S, L"FpsLimit", 120, g_ini);
    cfg_borderless = (int)GetPrivateProfileIntW(S, L"Borderless", 1, g_ini);
    cfg_stretch    = (int)GetPrivateProfileIntW(S, L"BorderlessFillMonitor", 1, g_ini);
    cfg_hotkey     = (int)GetPrivateProfileIntW(S, L"BorderlessToggleKey", 0x7A, g_ini);
    cfg_cammode    = (int)GetPrivateProfileIntW(S, L"DisableCameraAutoRotate", 1, g_ini);
    cfg_log        = (int)GetPrivateProfileIntW(S, L"Log", 1, g_ini);
    cfg_fov        = (int)GetPrivateProfileIntW(S, L"FovChangePercent", 0, g_ini);
    cfg_camreset   = (int)GetPrivateProfileIntW(S, L"DisableCameraResetOnLockOn", 0, g_ini);
    cfg_resW       = (int)GetPrivateProfileIntW(S, L"CustomResolutionWidth", 0, g_ini);
    cfg_resH       = (int)GetPrivateProfileIntW(S, L"CustomResolutionHeight", 0, g_ini);
    if (cfg_fov < -95) cfg_fov = -95;
    if (cfg_fov > 95) cfg_fov = 95;
    if (cfg_resW && (cfg_resW < 800 || cfg_resW > 7680 || cfg_resH < 450 || cfg_resH > 4320)) cfg_resW = cfg_resH = 0;
    if (cfg_fps != 0 && cfg_fps < 30) cfg_fps = 30;
    if (cfg_fps > 300) cfg_fps = 300;
}

// ---------------------------------------------------------------------------
// pattern scanning over the game's image
struct Pat { u8 b[128]; u8 m[128]; int n; };
static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }
static Pat ParsePat(const char* s)
{
    Pat p; p.n = 0;
    while (*s) {
        if (*s == ' ') { s++; continue; }
        if (*s == '?') { p.b[p.n] = 0; p.m[p.n++] = 0; s += (s[1] == '?') ? 2 : 1; continue; }
        p.b[p.n] = (u8)(hexv(s[0]) * 16 + hexv(s[1])); p.m[p.n++] = 1; s += 2;
    }
    return p;
}
static u8* Scan(const char* pattern)
{
    Pat p = ParsePat(pattern);
    u8* base = (u8*)g_exe;
    i32 peoff = *(i32*)(base + 0x3C);
    u32 imageSize = *(u32*)(base + peoff + 0x18 + 0x38);
    u8* end = base + imageSize;
    u8* cur = base;
    while (cur < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(cur, &mbi, sizeof(mbi))) break;
        u8* rs = (u8*)mbi.BaseAddress; u8* re = rs + mbi.RegionSize; if (re > end) re = end;
        bool readable = mbi.State == 0x1000 && !(mbi.Protect & 0x101) /*NOACCESS|GUARD*/;
        if (readable && re - rs >= p.n) {
            for (u8* q = rs; q <= re - p.n; q++) {
                int i = 0;
                for (; i < p.n; i++) if (p.m[i] && q[i] != p.b[i]) break;
                if (i == p.n) return q;
            }
        }
        cur = re;
    }
    return 0;
}

static bool WriteCode(void* at, const void* src, SIZE_T n)
{
    DWORD old;
    if (!VirtualProtect(at, n, 0x40, &old)) return false;
    memcpy(at, src, n);
    VirtualProtect(at, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, n);
    return true;
}

// one RWX block within +-2GB of the exe for caves and data
static u8* g_cave; static u32 g_caveUsed;
static u8* AllocNear(u8* target, SIZE_T size)
{
    SYSTEM_INFO si; GetSystemInfo(&si);
    u64 gran = si.dwAllocationGranularity;
    u64 lo = (u64)target > 0x70000000ull ? (u64)target - 0x70000000ull : (u64)si.lpMinimumApplicationAddress;
    u64 hi = (u64)target + 0x70000000ull;
    for (u64 a = (lo + gran - 1) & ~(gran - 1); a < hi; ) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void*)a, &mbi, sizeof(mbi))) break;
        if (mbi.State == 0x10000 /*MEM_FREE*/) {
            u64 start = ((u64)mbi.BaseAddress + gran - 1) & ~(gran - 1);
            if (start + size <= (u64)mbi.BaseAddress + mbi.RegionSize) {
                void* r = VirtualAlloc((void*)start, size, 0x3000, 0x40);
                if (r) return (u8*)r;
            }
        }
        u64 next = (u64)mbi.BaseAddress + mbi.RegionSize;
        a = next > a ? next : a + gran;
    }
    return 0;
}
static u8* CaveAlloc(u32 n) { u32 at = (g_caveUsed + 15) & ~15u; if (at + n > 0x1000) return 0; g_caveUsed = at + n; return g_cave + at; }
static bool InRel32(u8* from, u8* to) { i64 d = (i64)(to - from); return d > -0x7FFFFFF0ll && d < 0x7FFFFFF0ll; }
static void PutRel32(u8* p, u8* rip, u8* target) { *(i32*)p = (i32)(target - rip); }

// ---------------------------------------------------------------------------
// FPS unlock
static const float kSpeedFix[] = { 15,16,16.6667f,18,18.6875f,18.8516f,20,24,25,27.5f,30,32,38.5f,40,48,49.5f,50,57.2958f,60,64,66.75f,67,78.8438f,80,84,90,93.8f,100,120,127,128,130,140,144,150 };
static bool g_fpsDone;
static bool PatchFps()
{
    if (cfg_fps == 0) return true;
    u8* fl = Scan("88 88 3C 4C 89 AB");
    if (fl) fl -= 1;
    else { fl = Scan("C7 43 ?? ?? ?? ?? ?? 4C 89 AB"); if (fl) fl += 3; }
    u8* sp = Scan("F3 0F 58 ?? 0F C6 ?? 00 0F 51 ?? F3 0F 59 ?? ?? ?? ?? ?? 0F 2F");
    if (!fl || !sp) return false;
    sp += 15; // rel32 operand of mulss xmm0,[rip+X]

    float dt = 1.0f / (float)cfg_fps;
    float ideal = cfg_fps / 2.0f, best = 30.0f;
    for (unsigned i = 0; i < sizeof(kSpeedFix) / sizeof(float); i++)
        if (fabsf_(ideal - kSpeedFix[i]) < fabsf_(ideal - best)) best = kSpeedFix[i];

    float* data = (float*)CaveAlloc(4);
    if (!data || !InRel32(sp + 4, (u8*)data)) return false;
    *data = best;
    u8 rel[4]; PutRel32(rel, sp + 4, (u8*)data);
    WriteCode(fl, &dt, 4);
    WriteCode(sp, rel, 4);
    LOG("FPS: limit %d at %p, speed-fix ptr at %p", cfg_fps, fl, sp);
    return true;
}

// ---------------------------------------------------------------------------
// camera auto-rotate: caves are installed permanently but gated by a 1-byte flag,
// so switching between mouse/controller at runtime is a single atomic byte write.
struct CamPatch { const char* pat; int off; int len; const u8* sc; int scLen; const char* name; };
static const u8 SC_PITCH[]    = { 0x0F,0x28,0xA6,0x70,0x01,0x00,0x00, 0x0F,0x29,0xA5,0x70,0x08,0x00,0x00 };
static const u8 SC_YAW_Z[]    = { 0xF3,0x0F,0x10,0x86,0x74,0x01,0x00,0x00, 0xF3,0x0F,0x11,0x86,0x74,0x01,0x00,0x00 };
static const u8 SC_PITCH_XY[] = { 0xF3,0x0F,0x10,0x86,0x70,0x01,0x00,0x00, 0xF3,0x0F,0x11,0x00, 0xF3,0x0F,0x10,0x00, 0xF3,0x0F,0x11,0x86,0x70,0x01,0x00,0x00 };
static const u8 SC_YAW_XY[]   = { 0xF3,0x0F,0x10,0x86,0x74,0x01,0x00,0x00, 0xF3,0x0F,0x11,0x86,0x74,0x01,0x00,0x00 };
static const CamPatch kCam[] = {
    { "0F 29 ?? ?? ?? 00 00 0F 29 ?? ?? ?? 00 00 0F 29 ?? ?? ?? 00 00 EB ?? F3", 0, 7,  SC_PITCH,    sizeof(SC_PITCH),    "pitch" },
    { "E8 ?? ?? ?? ?? F3 ?? ?? ?? ?? ?? 00 00 80 ?? ?? ?? 00 00 00 0F 84",       5, 8,  SC_YAW_Z,    sizeof(SC_YAW_Z),    "yawZ" },
    { "F3 ?? ?? ?? F3 ?? ?? ?? 70 01 00 00 F3 ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F", 0, 12, SC_PITCH_XY, sizeof(SC_PITCH_XY), "pitchXY" },
    { "E8 ?? ?? ?? ?? F3 0F 11 86 ?? ?? 00 00 E9",                               5, 8,  SC_YAW_XY,   sizeof(SC_YAW_XY),   "yawXY" },
};
static bool g_camDone;
static bool PatchCamera()
{
    if (cfg_cammode == 0) return true;
    u8* at[4];
    for (int i = 0; i < 4; i++) { at[i] = Scan(kCam[i].pat); if (!at[i]) return false; at[i] += kCam[i].off; }
    // overwritten instructions get copied into the cave; refuse if any is RIP-relative
    for (int i = 0; i < 4; i++) {
        u8* p = at[i];
        // all four sites are [prefix] 0F xx modrm ... ; check modrm of each copied instruction
        int pos = 0;
        while (pos < kCam[i].len) {
            int pfx = (p[pos] == 0xF3 || p[pos] == 0x66) ? 1 : 0;
            if (p[pos + pfx] != 0x0F) { LOG("camera: unexpected bytes at %s", kCam[i].name); return false; }
            u8 modrm = p[pos + pfx + 2];
            if ((modrm & 0xC7) == 0x05) { LOG("camera: rip-relative at %s", kCam[i].name); return false; }
            int mod = modrm >> 6, rm = modrm & 7;
            int sz = pfx + 3 + (rm == 4 && mod != 3 ? 1 : 0) + (mod == 1 ? 1 : mod == 2 ? 4 : 0);
            pos += sz;
        }
        if (pos != kCam[i].len) { LOG("camera: length mismatch at %s", kCam[i].name); return false; }
    }

    g_camFlag = (volatile u8*)CaveAlloc(1);
    if (!g_camFlag) return false;
    *g_camFlag = (cfg_cammode == 2) ? 1 : 0;

    for (int i = 0; i < 4; i++) {
        const CamPatch& c = kCam[i];
        u8* back = at[i] + c.len;
        u8* cave = CaveAlloc(7 + 6 + c.scLen + 5 + c.len + 5);
        if (!cave || !InRel32(at[i], cave) || !InRel32(cave, (u8*)g_camFlag)) return false;
        u8* w = cave;
        // cmp byte ptr [rip+flag], 0
        w[0] = 0x80; w[1] = 0x3D; PutRel32(w + 2, w + 7, (u8*)g_camFlag); w[6] = 0x00; w += 7;
        // je orig
        u8* je = w; w[0] = 0x0F; w[1] = 0x84; w += 6;
        memcpy(w, c.sc, c.scLen); w += c.scLen;
        w[0] = 0xE9; PutRel32(w + 1, w + 5, back); w += 5;
        PutRel32(je + 2, je + 6, w);
        memcpy(w, at[i], c.len); w += c.len;
        w[0] = 0xE9; PutRel32(w + 1, w + 5, back); w += 5;

        u8 jmp[16]; jmp[0] = 0xE9; PutRel32(jmp + 1, at[i] + 5, cave);
        for (int k = 5; k < c.len; k++) jmp[k] = 0x90;
        WriteCode(at[i], jmp, c.len);
        LOG("camera: %s hooked at %p -> %p", c.name, at[i], cave);
    }
    return true;
}


// ---------------------------------------------------------------------------
// optional extras (all off unless set in the .ini)
static bool g_fovDone, g_resetDone, g_resDone;
static bool PatchFov()
{
    if (cfg_fov == 0) return true;
    u8* p = Scan("F3 0F 10 08 F3 0F 59 0D ?? ?? ?? ?? F3 0F 5C 4E");
    if (!p) return false;
    p += 8; // rel32 of mulss xmm1,[rip+X]
    float* data = (float*)CaveAlloc(4);
    if (!data || !InRel32(p + 4, (u8*)data)) return false;
    *data = 0.0174532925f * (cfg_fov / 100.0f + 1.0f);
    u8 rel[4]; PutRel32(rel, p + 4, (u8*)data);
    WriteCode(p, rel, 4);
    LOG("FOV: %d%% at %p", cfg_fov, p);
    return true;
}
static bool PatchCamReset()
{
    if (!cfg_camreset) return true;
    u8* p = Scan("C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00");
    if (!p) return false;
    u8 zero = 0; WriteCode(p + 6, &zero, 1);
    LOG("camera reset on lock-on disabled at %p", p + 6);
    return true;
}
static bool PatchResolution()
{
    if (!cfg_resW) return true;
    u8* fix = Scan("85 C9 74 ?? 47 8B ?? ?? ?? ?? ?? ?? 45 ?? ?? 74");
    u8* entry = Scan("80 07 00 00 38 04 00 00 00 08 00 00 80 04 00 00");          // 1920x1080 slot
    const char* slot = "1920x1080";
    if (!entry) { entry = Scan("00 05 00 00 D0 02 00 00 A0 05 00 00 2A 03 00 00"); slot = "1280x720"; }
    if (!fix || !entry) return false;
    i32 wh[2] = { cfg_resW, cfg_resH };
    WriteCode(entry, wh, 8);
    static const u8 jmp[3] = { 0x90, 0x90, 0xEB };
    WriteCode(fix, jmp, 3);
    LOG("resolution: %dx%d replaces the %s entry in the game's list", cfg_resW, cfg_resH, slot);
    return true;
}

// ---------------------------------------------------------------------------
// window helpers
static BOOL WINAPI EnumCb(HWND h, LONG_PTR)
{
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(h) && !GetWindow(h, 4 /*GW_OWNER*/)) {
        RECT r; GetClientRect(h, &r);
        if (r.right - r.left >= 320) { g_gameWnd = h; return 0; }
    }
    return 1;
}
static bool IsGameForeground() { HWND f = GetForegroundWindow(); return f && f == g_gameWnd; }

#define WS_POPUP 0x80000000L
#define WS_VISIBLE 0x10000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_THICKFRAME 0x00040000L
#define WS_EX_TOPMOST 0x8L
#define WS_WINDOWED (0x00C00000L | 0x00080000L | 0x00020000L | 0x04000000L | WS_VISIBLE) /*caption|sysmenu|minbox|clipsiblings*/

static bool g_borderlessOn;
static RECT g_savedRect; static bool g_haveSaved;

static bool IsExclusiveFullscreen(HWND h)
{
    LONG_PTR st = GetWindowLongPtrW(h, -16), ex = GetWindowLongPtrW(h, -20);
    return (ex & WS_EX_TOPMOST) && !(st & WS_POPUP) && !(st & WS_CAPTION) && !(st & WS_BORDER);
}
static void ApplyBorderless()
{
    HWND h = g_gameWnd; if (!h || IsIconic(h)) return;
    static bool warned;
    if (IsExclusiveFullscreen(h)) { if (!warned) { warned = true; LOG("borderless: skipped, game is in exclusive fullscreen - set Screen Mode to Windowed"); } return; }
    warned = false;
    LONG_PTR st = GetWindowLongPtrW(h, -16);
    RECT cr; GetClientRect(h, &cr);
    int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
    MONITORINFO mi; mi.cbSize = sizeof(mi);
    GetMonitorInfoW(MonitorFromWindow(h, 2), &mi);
    RECT m = mi.rcMonitor; int mw = m.right - m.left, mh = m.bottom - m.top;
    int x, y, w, hh;
    if (cfg_stretch) { x = m.left; y = m.top; w = mw; hh = mh; }
    else { w = cw; hh = ch; x = m.left + (mw - w) / 2; y = m.top + (mh - hh) / 2; if (x < m.left) x = m.left; if (y < m.top) y = m.top; }

    bool styled = (st & WS_POPUP) && !(st & (WS_CAPTION | WS_THICKFRAME));
    RECT wr; GetWindowRect(h, &wr);
    bool placed = wr.left == x && wr.top == y && (cfg_stretch ? (wr.right - wr.left == w && wr.bottom - wr.top == hh) : true);
    if (styled && placed) return;
    if (!styled) { g_savedRect = wr; g_haveSaved = true; }
    SetWindowLongPtrW(h, -16, WS_POPUP | WS_VISIBLE);
    SetWindowPos(h, 0, x, y, w, hh, 0x0020 | 0x0040 | 0x0004 /*FRAMECHANGED|SHOWWINDOW|NOZORDER*/);
    LOG("borderless: %dx%d at %d,%d", w, hh, x, y);
}
static void ApplyWindowed()
{
    HWND h = g_gameWnd; if (!h) return;
    LONG_PTR st = GetWindowLongPtrW(h, -16);
    if (!(st & WS_POPUP) || (st & WS_CAPTION)) return;
    int x, y, w, hh;
    if (g_haveSaved) {
        x = g_savedRect.left; y = g_savedRect.top;
        w = g_savedRect.right - g_savedRect.left; hh = g_savedRect.bottom - g_savedRect.top;
    } else {
        RECT r = { 0, 0, 1280, 720 }; AdjustWindowRectEx(&r, WS_WINDOWED, 0, 0);
        x = 64; y = 64; w = r.right - r.left; hh = r.bottom - r.top;
    }
    SetWindowLongPtrW(h, -16, WS_WINDOWED);
    SetWindowPos(h, (HWND)-2 /*NOTOPMOST*/, x, y, w, hh, 0x0020 | 0x0040);
    LOG("windowed: %dx%d", w, hh);
}

// ---------------------------------------------------------------------------
// input detection: low-level mouse hook (doesn't interfere with the game's own input)
static LONG_PTR WINAPI MouseHook(int code, UINT_PTR wp, LONG_PTR lp)
{
    if (code >= 0) {
        MSLLHOOKSTRUCT* m = (MSLLHOOKSTRUCT*)lp;
        if (!(m->flags & 1 /*LLMHF_INJECTED*/)) g_mouseEvents++;
    }
    return CallNextHookEx(0, code, wp, lp);
}
static DWORD WINAPI HookThread(void*)
{
    void* hk = SetWindowsHookExW(14 /*WH_MOUSE_LL*/, MouseHook, g_self, 0);
    LOG("mouse hook %s", hk ? "installed" : "FAILED");
    MSG msg; while (GetMessageW(&msg, 0, 0, 0) > 0) {}
    return 0;
}

static XInputGetState_t g_xget;
static void LoadXInput()
{
    const WCHAR* names[] = { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" };
    for (int i = 0; i < 3 && !g_xget; i++) {
        HMODULE m = GetModuleHandleW(names[i]);
        if (!m) m = LoadLibraryW(names[i]);
        if (m) g_xget = (XInputGetState_t)GetProcAddress(m, "XInputGetState");
    }
}
static int iabs(int v) { return v < 0 ? -v : v; }
static bool PadActive(u64 now)
{
    static u64 nextProbe[4]; static bool connected[4];
    if (!g_xget) return false;
    bool active = false;
    for (DWORD i = 0; i < 4; i++) {
        if (!connected[i] && now < nextProbe[i]) continue;
        XINPUT_STATE s; memset(&s, 0, sizeof(s));
        if (g_xget(i, &s) != 0) { connected[i] = false; nextProbe[i] = now + 2000; continue; }
        connected[i] = true;
        XINPUT_GAMEPAD& g = s.Gamepad;
        if (g.wButtons || g.bLeftTrigger > 30 || g.bRightTrigger > 30 ||
            iabs(g.sThumbRX) > 8689 || iabs(g.sThumbRY) > 8689 ||
            iabs(g.sThumbLX) > 7849 || iabs(g.sThumbLY) > 7849) active = true;
    }
    return active;
}

// ---------------------------------------------------------------------------
static DWORD WINAPI MainThread(void*)
{
    // patches: retry in case the exe is still being unpacked
    for (int t = 0; t < 600; t++) {
        if (!g_fpsDone) g_fpsDone = PatchFps();
        if (!g_camDone) g_camDone = PatchCamera();
        if (!g_fovDone) g_fovDone = PatchFov();
        if (!g_resetDone) g_resetDone = PatchCamReset();
        if (!g_resDone) g_resDone = PatchResolution();
        if (g_fpsDone && g_camDone && g_fovDone && g_resetDone && g_resDone) break;
        Sleep(100);
    }
    if (!g_fovDone) LOG("FOV patch: pattern not found");
    if (!g_resetDone) LOG("lock-on reset patch: pattern not found");
    if (!g_resDone) LOG("resolution patch: pattern not found");
    if (!g_fpsDone) LOG("FPS patch: pattern not found (unsupported game version?)");
    if (!g_camDone) LOG("camera patch: pattern not found (unsupported game version?)");

    if (cfg_cammode == 1 && g_camFlag) { LoadXInput(); CreateThread(0, 0, HookThread, 0, 0, 0); }
    g_borderlessOn = cfg_borderless != 0;

    DWORD lastMouse = g_mouseEvents; u64 lastWin = 0, lastMouseCheck = 0; bool keyDown = false;
    for (;;) {
        Sleep(10);
        u64 now = GetTickCount64();

        if (now - lastWin > 500) {
            lastWin = now;
            if (!g_gameWnd || !IsWindowVisible(g_gameWnd)) { g_gameWnd = 0; EnumWindows(EnumCb, 0); if (g_gameWnd) { LONG_PTR st = GetWindowLongPtrW(g_gameWnd, -16), ex = GetWindowLongPtrW(g_gameWnd, -20); LOG("game window found: style=%08X ex=%08X", (u32)st, (u32)ex); } }
            if (g_gameWnd && g_borderlessOn) ApplyBorderless();
        }

        if (g_gameWnd && cfg_hotkey && IsGameForeground()) {
            bool down = (GetAsyncKeyState(cfg_hotkey) & 0x8000) != 0;
            if (down && !keyDown) {
                g_borderlessOn = !g_borderlessOn;
                if (g_borderlessOn) ApplyBorderless(); else ApplyWindowed();
            }
            keyDown = down;
        }

        if (cfg_cammode == 1 && g_camFlag) {
            if (PadActive(now)) { g_mouseActive = false; lastMouse = g_mouseEvents; lastMouseCheck = now; }
            else if (now - lastMouseCheck >= 100) {
                DWORD me = g_mouseEvents, delta = me - lastMouse; lastMouse = me; lastMouseCheck = now;
                if (delta >= 6 && IsGameForeground()) g_mouseActive = true;  // ignore small desk bumps
            }
            u8 want = g_mouseActive ? 1 : 0;
            if (*g_camFlag != want) { *g_camFlag = want; LOG("input: %s -> camera auto-rotate %s", want ? "mouse" : "controller", want ? "OFF" : "ON (vanilla)"); }
        }
    }
}

// ---------------------------------------------------------------------------
// dinput8 forwarding
static HMODULE g_real;
static void* RealProc(const char* name)
{
    if (!g_real) {
        WCHAR p[520];
        // optional chain-load (e.g. Sekiro Mod Engine renamed to modengine_dinput8.dll)
        WCHAR chain[260]; chain[0] = 0;
        GetPrivateProfileStringW(L"SekiroTweaks", L"ChainLoadDll", L"", chain, 260, g_ini);
        if (chain[0]) { wcopy(p, g_dir); wcat(p, chain); g_real = LoadLibraryW(p); }
        if (!g_real) { GetSystemDirectoryW(p, 260); wcat(p, L"\\dinput8.dll"); g_real = LoadLibraryW(p); }
    }
    return g_real ? GetProcAddress(g_real, name) : 0;
}
extern "C" HRESULT WINAPI Proxy_DirectInput8Create(void* a, DWORD b, void* c, void** d, void* e)
{ typedef HRESULT(WINAPI* F)(void*, DWORD, void*, void**, void*); F f = (F)RealProc("DirectInput8Create"); return f ? f(a, b, c, d, e) : (HRESULT)0x80004005; }
extern "C" HRESULT WINAPI Proxy_DllCanUnloadNow()
{ typedef HRESULT(WINAPI* F)(); F f = (F)RealProc("DllCanUnloadNow"); return f ? f() : 1; }
extern "C" HRESULT WINAPI Proxy_DllGetClassObject(void* a, void* b, void** c)
{ typedef HRESULT(WINAPI* F)(void*, void*, void**); F f = (F)RealProc("DllGetClassObject"); return f ? f(a, b, c) : (HRESULT)0x80040111; }
extern "C" HRESULT WINAPI Proxy_DllRegisterServer()
{ typedef HRESULT(WINAPI* F)(); F f = (F)RealProc("DllRegisterServer"); return f ? f() : (HRESULT)0x80004005; }
extern "C" HRESULT WINAPI Proxy_DllUnregisterServer()
{ typedef HRESULT(WINAPI* F)(); F f = (F)RealProc("DllUnregisterServer"); return f ? f() : (HRESULT)0x80004005; }
extern "C" void* WINAPI Proxy_GetdfDIJoystick()
{ typedef void*(WINAPI* F)(); F f = (F)RealProc("GetdfDIJoystick"); return f ? f() : 0; }

extern "C" BOOL WINAPI DllMain(HMODULE h, DWORD reason, void*)
{
    if (reason == 1) {
        g_self = h; g_exe = GetModuleHandleW(0);
        DisableThreadLibraryCalls(h);
        LoadConfig();
        // only act inside sekiro.exe
        WCHAR exe[520]; GetModuleFileNameW(0, exe, 520);
        const WCHAR* name = exe; for (const WCHAR* q = exe; *q; q++) if (*q == '\\' || *q == '/') name = q + 1;
        if (!wieq(name, L"sekiro.exe")) return 1;
        LOG("---- SekiroTweaks loaded (fps=%d borderless=%d cam=%d) ----", cfg_fps, cfg_borderless, cfg_cammode);
        g_cave = AllocNear((u8*)g_exe, 0x1000);
        if (!g_cave) { LOG("could not allocate cave memory"); return 1; }
        // patch as early as possible; the thread retries if the exe isn't unpacked yet
        g_fpsDone = PatchFps();
        g_camDone = PatchCamera();
        g_fovDone = PatchFov();
        g_resetDone = PatchCamReset();
        g_resDone = PatchResolution();
        CreateThread(0, 0, MainThread, 0, 0, 0);
    }
    return 1;
}
