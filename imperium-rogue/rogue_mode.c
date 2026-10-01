#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "rogue_mode.h"
#include "sound.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

static void Rogue_MainCB(void);
static void Rogue_VBlankCB(void);
static bool8 Rogue_SetupScreen(void);
static void Task_RogueMenu(u8 taskId);
static void Rogue_DrawMenu(void);
static void Rogue_ReturnToTitle(void);

static const u8 sTextTitle[] = _("IMPERIUM ROGUE");
static const u8 sTextPrototype[] = _("ROGUE MODE v0.1");
static const u8 sTextNewRun[] = _("NEW RUN");
static const u8 sTextContinue[] = _("CONTINUE RUN");
static const u8 sTextRecords[] = _("RECORDS");
static const u8 sTextBack[] = _("BACK");
static const u8 sTextStatus[] = _("Build test: menu hook active.");
static const u8 sTextNext[] = _("Next: starters, waves, rewards.");

static const struct BgTemplate sRogueBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sRogueWindows[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 6,
        .width = 26,
        .height = 12,
        .paletteNum = 15,
        .baseBlock = 105,
    },
    DUMMY_WIN_TEMPLATE,
};

static u8 sCursor;

void CB2_InitRogueMode(void)
{
    if (Rogue_SetupScreen())
    {
        sCursor = 0;
        Rogue_DrawMenu();
        CreateTask(Task_RogueMenu, 0);
    }
}

static bool8 Rogue_SetupScreen(void)
{
    u16 i;

    switch (gMain.state)
    {
    case 0:
    default:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG3HOFS, 0);
        SetGpuReg(REG_OFFSET_BG3VOFS, 0);
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);

        DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
        DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
        DmaFill16(3, 0, (void *)(PLTT + 2), PLTT_SIZE - 2);

        ResetPaletteFade();
        gPlttBufferUnfaded[0] = RGB_WHITE;
        gPlttBufferFaded[0] = RGB_WHITE;
        gPlttBufferUnfaded[1] = RGB(5, 10, 14);
        gPlttBufferFaded[1] = RGB(5, 10, 14);

        for (i = 0; i < 0x10; i++)
            ((u16 *)(VRAM + 0x20))[i] = 0x1111;
        for (i = 0; i < 0x400; i++)
            ((u16 *)(BG_SCREEN_ADDR(30)))[i] = 0x0001;

        ResetTasks();
        ResetSpriteData();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sRogueBgTemplates, ARRAY_COUNT(sRogueBgTemplates));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(3);

        InitWindows(sRogueWindows);
        DeactivateAllTextPrinters();
        FillWindowPixelBuffer(0, PIXEL_FILL(0));
        FillWindowPixelBuffer(1, PIXEL_FILL(0));
        LoadWindowGfx(0, 0, 2, BG_PLTT_ID(14));
        LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);

        BeginNormalPaletteFade(PALETTES_BG, 0, 0x10, 0, RGB_WHITEALPHA);
        EnableInterrupts(INTR_FLAG_VBLANK);
        SetVBlankCallback(Rogue_VBlankCB);
        gMain.state = 1;
        break;
    case 1:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            SetMainCallback2(Rogue_MainCB);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static void Rogue_DrawMenu(void)
{
    static const u8 *const sOptions[] =
    {
        sTextNewRun,
        sTextContinue,
        sTextRecords,
        sTextBack,
    };
    u32 i;

    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    FillWindowPixelBuffer(1, PIXEL_FILL(0));
    DrawStdFrameWithCustomTileAndPalette(0, FALSE, 2, 14);
    DrawStdFrameWithCustomTileAndPalette(1, FALSE, 2, 14);

    AddTextPrinterParameterized(0, FONT_NORMAL, sTextTitle, 4, 1, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(0, FONT_SMALL, sTextPrototype, 4, 18, TEXT_SKIP_DRAW, NULL);

    for (i = 0; i < ARRAY_COUNT(sOptions); i++)
    {
        u8 y = 8 + i * 16;
        if (i == sCursor)
            AddTextPrinterParameterized(1, FONT_NORMAL, COMPOUND_STRING(">"), 4, y, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(1, FONT_NORMAL, sOptions[i], 20, y, TEXT_SKIP_DRAW, NULL);
    }

    AddTextPrinterParameterized(1, FONT_SMALL, sTextStatus, 4, 78, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(1, FONT_SMALL, sTextNext, 4, 92, TEXT_SKIP_DRAW, NULL);

    PutWindowTilemap(0);
    PutWindowTilemap(1);
    CopyWindowToVram(0, COPYWIN_FULL);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static void Task_RogueMenu(u8 taskId)
{
    if (JOY_NEW(DPAD_UP))
    {
        sCursor = (sCursor + 3) % 4;
        PlaySE(SE_SELECT);
        Rogue_DrawMenu();
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        sCursor = (sCursor + 1) % 4;
        PlaySE(SE_SELECT);
        Rogue_DrawMenu();
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (sCursor == 3)
        {
            DestroyTask(taskId);
            Rogue_ReturnToTitle();
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        DestroyTask(taskId);
        Rogue_ReturnToTitle();
    }
}

static void Rogue_ReturnToTitle(void)
{
    FreeAllWindowBuffers();
    DoSoftReset();
}

static void Rogue_MainCB(void)
{
    RunTasks();
    UpdatePaletteFade();
}

static void Rogue_VBlankCB(void)
{
    TransferPlttBuffer();
}
