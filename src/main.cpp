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
const int passCount = 3;

int main(int argc, char **argv) {
    GSGLOBAL *gsGlobal = gsKit_init_global();
    // gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    // gsGlobal->DoubleBuffering = GS_SETTING_OFF;
    // gsGlobal->ZBuffering = GS_SETTING_OFF;
    // gsGlobal->Mode = GS_MODE_DTV_1080I;
    // gsGlobal->Interlace = GS_INTERLACED;
    // gsGlobal->Field = GS_FIELD;
    // gsGlobal->Width = 640;
    // gsGlobal->Height = 540;
    // gsGlobal->PSM = GS_PSM_CT16;
    // gsGlobal->PSMZ = GS_PSMZ_16;
	// gsGlobal->Dithering = GS_SETTING_ON;
	// gsGlobal->DoubleBuffering = GS_SETTING_ON;
	// gsGlobal->ZBuffering = GS_SETTING_ON;
	// gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    // gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
    gsKit_clear(gsGlobal, White);
	gsKit_set_test(gsGlobal, GS_ZTEST_OFF);

	gsKit_vram_clear(gsGlobal);
	gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 128), 0);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup ImGui backends
    ImGui_ImplPs2Sdk_InitForGsKit(gsGlobal);
    ImGui_ImplPs2GsKit_Init(gsGlobal);

    bool showDemoWindow = true;

    while(1)
	{
        // Start the Dear ImGui frame
        ImGui_ImplPs2Sdk_NewFrame();
        ImGui_ImplPs2GsKit_NewFrame();
        ImGui::NewFrame();

        // ImGui::ShowDemoWindow(&showDemoWindow);
        ImGui::SetNextWindowPos(ImVec2(15.0, 15.0));
        ImGui::SetNextWindowSize(ImVec2(200, 400));
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        ImGui::End();
        ImGui::Render();

		gsKit_clear(gsGlobal, White);
        ImGui_ImplPs2GsKit_RenderDrawData(ImGui::GetDrawData());

        // gsKit_prim_triangle_gouraud(gsGlobal, 280.0f, 200.0f,
        //                     280.0f, 350.0f,
        //                     180.0f, 350.0f, 5,
        //                     Blue, Red, Green);

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
		gsKit_TexManager_nextFrame(gsGlobal);
	}







    // GSGLOBAL *gsGlobal = gsKit_init_global();
    // gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    // // gsGlobal->Mode = GS_MODE_DTV_1080I;
    // // gsGlobal->Interlace = GS_INTERLACED;
    // // gsGlobal->Field = GS_FIELD;
    // // gsGlobal->Width = 640;
    // // gsGlobal->Height = 540;
    // // gsGlobal->PSM = GS_PSM_CT16;
    // // gsGlobal->PSMZ = GS_PSMZ_16;
	// // gsGlobal->Dithering = GS_SETTING_ON;
	// // gsGlobal->DoubleBuffering = GS_SETTING_ON;
	// // gsGlobal->ZBuffering = GS_SETTING_ON;
	// // gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	
    // dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	// dmaKit_chan_init(DMA_CHANNEL_GIF);
    // gsKit_init_screen(gsGlobal);
	// // gsKit_hires_init_screen(gsGlobal, passCount);
	// gsKit_vram_clear(gsGlobal);
	// // gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);
	// // gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 128), 0);

    // // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // ImGui::StyleColorsDark();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // // Setup ImGui backends
    // ImGui_ImplPs2Sdk_InitForGsKit(gsGlobal);
    // ImGui_ImplPs2GsKit_Init(gsGlobal);

    // bool showDemoWindow = true;

    // while(1)
	// {
    //     // Start the Dear ImGui frame
    //     ImGui_ImplPs2Sdk_NewFrame();
    //     ImGui_ImplPs2GsKit_NewFrame();
    //     ImGui::NewFrame();

    //     // ImGui::ShowDemoWindow(&showDemoWindow);
    //     ImGui::SetNextWindowPos(ImVec2(15.0, 15.0));
    //     ImGui::SetNextWindowSize(ImVec2(200, 400));
    //     ImGui::Begin("Hello, world!");
    //     ImGui::Text("This is some useful text.");
    //     ImGui::End();

    //     ImGui::Render();
	// 	gsKit_clear(gsGlobal, DarkGrey);
    //     ImGui_ImplPs2GsKit_RenderDrawData(ImGui::GetDrawData());

    //     gsKit_sync_flip(gsGlobal);
	// 	// gsKit_hires_sync(gsGlobal);
	// 	// gsKit_hires_flip(gsGlobal);
	// 	gsKit_TexManager_nextFrame(gsGlobal);
	// }

	return 0;
}
