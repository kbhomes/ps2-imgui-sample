// dear imgui: Platform Backend for PlayStation 2 consoles using ps2sdk
// This needs to be used along with the PS2 gsKit renderer

// Features:
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [ ] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_ps2sdk.h"

// TODO: ps2sdk includes
#include <gsKit.h>
#include <dmaKit.h>
#include <tamtypes.h>
#include <ps2sdkapi.h>
#include <libpad.h>

// gsKit Data
struct ImGui_ImplPs2Sdk_Data
{
    GSGLOBAL* Global;
    u64 Time;
    u16 PreviousGamePadData;

    ImGui_ImplPs2Sdk_Data()   { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplPs2Sdk_Data* ImGui_ImplPs2Sdk_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplPs2Sdk_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Functions

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplPs2Sdk_ProcessEvent(const Ps2Event* event)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPs2Sdk_Data* bd = ImGui_ImplPs2Sdk_GetBackendData();

    // TODO: Do we even need to process any event data other than gamepad (done elsewhere)?
    return true;
}

static bool ImGui_ImplPs2Sdk_Init(GSGLOBAL* global)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplPs2Sdk_Data* bd = IM_NEW(ImGui_ImplPs2Sdk_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_ps2sdk";
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)

    bd->Global = global;

    io.SetClipboardTextFn = NULL;
    io.GetClipboardTextFn = NULL;
    io.ClipboardUserData = NULL;

    return true;
}

bool ImGui_ImplPs2Sdk_InitForGsKit(GSGLOBAL* global)
{
    return ImGui_ImplPs2Sdk_Init(global);
}

void ImGui_ImplPs2Sdk_Shutdown()
{
    ImGui_ImplPs2Sdk_Data* bd = ImGui_ImplPs2Sdk_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplPs2Sdk_UpdateMouseData()
{
    ImGui_ImplPs2Sdk_Data* bd = ImGui_ImplPs2Sdk_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    // TODO: Emulate mouse data with the right joystick and R3 button
}

static void ImGui_ImplPs2Sdk_UpdateGamepads(ImGui_ImplPs2Sdk_Data* bd)
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Update gamepad state
    int gamepadState = padGetState(0, 0);
    if (gamepadState != PAD_STATE_STABLE && gamepadState != PAD_STATE_FINDCTP1) {
        // Controller is not active, so disable it in imgui
        bd->PreviousGamePadData = 0;
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }
    
    // Read gamepad data
    padButtonStatus buttons;
    if (padRead(0, 0, &buttons)) {
        // Invert active-low button states, and determine which digital buttons are new since last frame
        u16 padData = buttons.btns ^ 0xFFFF;
        u16 newPadData = padData & ~bd->PreviousGamePadData;
        bd->PreviousGamePadData = padData;

        #define MAP_DIGITAL_BUTTON(NAV_NO, BUTTON_MASK) { io.NavInputs[NAV_NO] = (newPadData & BUTTON_MASK) ? 1.0f : 0.0f; }
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Activate,   PAD_CROSS);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Cancel,     PAD_CIRCLE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Menu,       PAD_TRIANGLE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Input,      PAD_SQUARE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadLeft,   PAD_LEFT);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadRight,  PAD_RIGHT);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadUp,     PAD_UP);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadDown,   PAD_DOWN);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_FocusPrev,  PAD_L1);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_FocusNext,  PAD_R1);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_TweakSlow,  PAD_L2);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_TweakFast,  PAD_R2);
        // TODO: Joystick inputs
        #undef MAP_DIGITAL_BUTTON
        
        // Indicate that the gamepad is present
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }
}

void ImGui_ImplPs2Sdk_NewFrame()
{
    ImGui_ImplPs2Sdk_Data* bd = ImGui_ImplPs2Sdk_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplPs2Sdk_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Update the framebuffer to handle resizing
    io.DisplaySize = ImVec2((float)bd->Global->Width, (float)bd->Global->Height);
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);

    // TODO: Update the imgui DeltaTime
    io.DeltaTime = (float)(1.0f / 60.0f);
    // u64 current_time = ps2_clock();
    // io.DeltaTime = bd->Time > 0 ? (float)((double)(current_time - bd->Time) / PS2_CLOCKS_PER_SEC) : (float)(1.0f / 60.0f);
    // bd->Time = current_time;

    // ImGui_ImplPs2Sdk_UpdateMouseData();
    // ImGui_ImplPs2Sdk_UpdateGamepads(bd);
}
