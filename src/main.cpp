#include <stdio.h>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>

#include <imgui.h>
#include <imgui_impl_ps2sdk.h>
#include <imgui_impl_ps2gskit.h>

const u64 DarkGrey = GS_SETREG_RGBAQ(0x40,0x40,0x40,0x80,0x00);
const u64 Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x80,0x00);
const u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);
const u64 Red = GS_SETREG_RGBAQ(0xFF,0x00,0x00,0x00,0x00);
const u64 Green = GS_SETREG_RGBAQ(0x00,0xFF,0x00,0x00,0x00);
const u64 Blue = GS_SETREG_RGBAQ(0x00,0x00,0xFF,0x00,0x00);

const u64 BlueTrans = GS_SETREG_RGBAQ(0x00,0x00,0xFF,0x40,0x00);
const u64 RedTrans = GS_SETREG_RGBAQ(0xFF,0x00,0x00,0x60,0x00);
const u64 GreenTrans = GS_SETREG_RGBAQ(0x00,0xFF,0x00,0x50,0x00);
const u64 WhiteTrans = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x50,0x00);


const int passCount = 3;

int main(int argc, char **argv) {
    GSGLOBAL *gsGlobal = gsKit_init_global();
    gsGlobal->PSM = GS_PSM_CT32;
    gsGlobal->PSMZ = GS_PSMZ_16S;
	gsGlobal->Dithering = GS_SETTING_ON;
	gsGlobal->DoubleBuffering = GS_SETTING_ON;
	gsGlobal->ZBuffering = GS_SETTING_ON;
	
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
	gsKit_set_test(gsGlobal, GS_ZTEST_ON);
	gsKit_set_test(gsGlobal, GS_ATEST_ON);

	gsKit_vram_clear(gsGlobal);
	gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);
    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup ImGui backends
    ImGui_ImplPs2Sdk_InitForGsKit(gsGlobal);
    ImGui_ImplPs2GsKit_Init(gsGlobal);

    // bool showDemoWindow = true;

    while(1)
	{
        // Start the Dear ImGui frame
        ImGui_ImplPs2Sdk_NewFrame();
        ImGui_ImplPs2GsKit_NewFrame();
        ImGui::NewFrame();

        // ImGui::ShowDemoWindow(&showDemoWindow);
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(300, 400));
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");

        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));
        ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80.0f));

        if (ImGui::Button("Save"))
            ;
        ImGui::End();
        ImGui::Render();

        gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;
		gsKit_clear(gsGlobal, Black);
        gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

        ImGui_ImplPs2GsKit_RenderDrawData(ImGui::GetDrawData());

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
		gsKit_TexManager_nextFrame(gsGlobal);
	}

	return 0;
}
