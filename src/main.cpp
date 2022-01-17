#include <stdio.h>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>

#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>

#include <imgui.h>
#include <imgui_impl_ps2sdk.h>
#include <imgui_impl_ps2gskit.h>

#include "drawing/drawing.h"
#include "custom_font.h"
#include "widgets/widget.h"


const u64 DarkGrey = GS_SETREG_RGBAQ(0x40,0x40,0x40,0x80,0x00);
const u64 Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x80,0x00);
const u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);

// pad_dma_buf is provided by the user, one buf for each pad
// contains the pad's current state
static char padBuf[256] __attribute__((aligned(64)));

GSGLOBAL *gfx_init(bool hires, bool textureManager) {
    GSGLOBAL *gsGlobal;
    int hiresPassCount;

    if (hires) {
        gsGlobal = gsKit_hires_init_global();
        gsGlobal->Mode = GS_MODE_DTV_720P;
        gsGlobal->Interlace = GS_NONINTERLACED;
        gsGlobal->Field = GS_FRAME;
        gsGlobal->Width = 1280;
        gsGlobal->Height = 720;
        hiresPassCount = 3;
    } else {
        gsGlobal = gsKit_init_global();
    }

    if ((gsGlobal->Interlace == GS_INTERLACED) && (gsGlobal->Field == GS_FRAME))
        gsGlobal->Height /= 2;

    // Setup GS global settings
    gsGlobal->PSM = GS_PSM_CT32;
    gsGlobal->PSMZ = GS_PSMZ_16S;
    gsGlobal->Dithering = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;
    gsGlobal->ZBuffering = GS_SETTING_ON;
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsKit_set_test(gsGlobal, GS_ZTEST_ON);
    gsKit_set_test(gsGlobal, GS_ATEST_ON);
    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 128), 0);
    gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);

    // Initialize DMA settings
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    if (hires) {
        gsKit_hires_init_screen(gsGlobal, hiresPassCount);
    } else {
        gsKit_init_screen(gsGlobal);
    }

    if (textureManager) {
        gsKit_vram_clear(gsGlobal);
        gsKit_TexManager_init(gsGlobal);
    }

    return gsGlobal;
}

void gfx_imgui_init(GSGLOBAL *gsGlobal) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.Fonts->AddFontFromMemoryCompressedTTF(custom_font_compressed_data, custom_font_compressed_size, 12);

    ImGuiStyle& style = ImGui::GetStyle();
    style.CellPadding = ImVec2(4, 2);
    style.ItemSpacing = ImVec2(6, 6);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.FrameRounding = 4;
    style.FramePadding = ImVec2(10, 2);
    style.ScrollbarSize = 14;
    style.GrabMinSize = 10;
    style.GrabRounding = 2;
    style.WindowBorderSize = 0;
    style.WindowRounding = 2;
    style.WindowPadding = ImVec2(6, 6);
    style.WindowTitleAlign = ImVec2(0.5, 0.5);
    style.MouseCursorScale = 0.8;

    // Setup ImGui backends
    ImGui_ImplPs2Sdk_InitForGsKit(gsGlobal);
    ImGui_ImplPs2GsKit_Init(gsGlobal);
}

void gfx_render(GSGLOBAL *gsGlobal, bool hires, bool textureManager) {
    gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;
    gsKit_clear(gsGlobal, DarkGrey);
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

    // Start the Dear ImGui frame
    ImGui_ImplPs2Sdk_NewFrame();
    ImGui_ImplPs2GsKit_NewFrame();
    ImGui::NewFrame();

    {
        int spacing = 10;

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(spacing, spacing));
        ImGui::SetNextWindowSize(ImVec2(gsGlobal->Width/2 - 1.5*spacing, gsGlobal->Height - 2*spacing));
        ImGui::SetNextWindowFocus();
        ImGui::Begin("PS2 + ImGui", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImGui::ShowStyleEditor();
        ImGui::End();

        // Draw the controller
        ImGui::SetNextWindowPos(ImVec2(gsGlobal->Width/2 + 0.5*spacing, spacing));
        ImGui::SetNextWindowSize(ImVec2(gsGlobal->Width/2 - 1.5*spacing, 180));
        ImGui::Begin("Gamepad", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
        {
            // Invert active-low button states, and determine which digital buttons are new since last frame
            padButtonStatus pad;
            padRead(0, 0, &pad);
            ImGui::GamePadVisualizer(&pad, ImGui::GetWindowWidth() * 0.95, ImGui::GetWindowHeight() * 0.55);
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplPs2GsKit_RenderDrawData(ImGui::GetDrawData());

    if (hires) {
        gsKit_hires_sync(gsGlobal);
        gsKit_hires_flip(gsGlobal);
    } else {
        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
    }

    if (textureManager) {
        gsKit_TexManager_nextFrame(gsGlobal);
    }
}

static void load_modules(void) {
    int ret;

    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule sio failed: %d\n", ret);
    }

    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule pad failed: %d\n", ret);
    }
}

static int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n",
                       port, slot, stateString);
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 0;
}

static int initializePad(int port, int slot) {
    int ret;
    int i;
    int modes;

    if ((ret = padPortOpen(port, slot, padBuf)) == 0) {
        printf("padOpenPort failed: %d\n", ret);
    }

    waitPadReady(port, slot);

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0) {
        printf("( ");
        for (i = 0; i < modes; i++) {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n",
               padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller
    // (it has no actuator engines)
    if (modes == 0) {
        printf("This is a digital controller?\n");
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        printf("This is no Dual Shock controller\n");
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0) {
        printf("This is no Dual Shock controller??\n");
        return 1;
    }

    printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
    
    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

    return 1;
}

int main(int argc, char **argv) {
    SifInitRpc(0);
    load_modules();
    padInit(0);
    initializePad(0, 0);

    GSGLOBAL *gsGlobal = gfx_init(false, true);
    gfx_imgui_init(gsGlobal);

    while(1)
    {
       gfx_render(gsGlobal, false, true);
    }

    return 0;
}
