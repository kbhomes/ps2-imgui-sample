// dear imgui: Renderer Backend for PS2 gsKit
// This needs to be used along with the PS2SDK backend

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_ps2gskit.h"

// TODO: ps2sdk and gsKit includes
#include <malloc.h>
#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <stdio.h>

const u64 Red = GS_SETREG_RGBAQ(0x80,0x00,0x00,0x80,0x00);
const u64 Green = GS_SETREG_RGBAQ(0x00,0x80,0x00,0x80,0x00);
const u64 Blue = GS_SETREG_RGBAQ(0x00,0x00,0x80,0x80,0x00);


struct ImGui_ImplPs2GsKit_Data
{
    GSGLOBAL *Global;
    GSTEXTURE *FontTexture;

    ImGui_ImplPs2GsKit_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplPs2GsKit_Data* ImGui_ImplPs2GsKit_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplPs2GsKit_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
}

// Functions
bool ImGui_ImplPs2GsKit_Init(GSGLOBAL *global)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplPs2GsKit_Data* bd = IM_NEW(ImGui_ImplPs2GsKit_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_ps2gskit";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    bd->Global = global;
    bd->FontTexture = NULL;

    gsKit_TexManager_init(global);

    return true;
}

void ImGui_ImplPs2GsKit_Shutdown()
{
    ImGui_ImplPs2GsKit_Data* bd = ImGui_ImplPs2GsKit_GetBackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplPs2GsKit_DestroyDeviceObjects();
    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

void ImGui_ImplPs2GsKit_NewFrame()
{
    ImGui_ImplPs2GsKit_Data* bd = ImGui_ImplPs2GsKit_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplPs2GsKit_Init()?");

    if (!bd->FontTexture) {
        ImGui_ImplPs2GsKit_CreateDeviceObjects();
    }
}

static void ImGui_ImplPs2GsKit_SetupRenderState(ImDrawData* draw_data)
{
    // TODO: Anything?
}

static u64 ImGui_ImplPs2GsKit_NormalizeImColor(ImU32 color)
{
    u8 r = (color >> IM_COL32_R_SHIFT) & 0xFF;
    u8 g = (color >> IM_COL32_G_SHIFT) & 0xFF;
    u8 b = (color >> IM_COL32_B_SHIFT) & 0xFF;
    u8 a = (color >> IM_COL32_A_SHIFT) & 0xFF;
    
    return GS_SETREG_RGBA(r >> 1, g >> 1, b >> 1, a >> 1);
}

void ImGui_ImplPs2GsKit_RenderDrawData(ImDrawData* draw_data)
{
    ImGui_ImplPs2GsKit_Data* bd = ImGui_ImplPs2GsKit_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplPs2GsKit_Init()?");

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }

    // Setup desired render state
    ImGui_ImplPs2GsKit_SetupRenderState(draw_data);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1)

    
    gsKit_TexManager_bind(bd->Global, bd->FontTexture);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            // GSTEXTURE *texture = (GSTEXTURE *)pcmd->GetTexID();
            
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplPs2GsKit_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                   continue;

                // TODO: Apply scissor/clipping rectangle
                // glScissor((int)clip_min.x, (int)(fb_height - clip_max.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));

                // TODO: Draw triangles
                for (size_t e = 0; e < pcmd->ElemCount; e += 3) {
                    ImDrawIdx idx1 = idx_buffer[pcmd->IdxOffset + e + 0];
                    ImDrawVert vtx1 = vtx_buffer[pcmd->VtxOffset + idx1];
                    ImDrawIdx idx2 = idx_buffer[pcmd->IdxOffset + e + 1];
                    ImDrawVert vtx2 = vtx_buffer[pcmd->VtxOffset + idx2];
                    ImDrawIdx idx3 = idx_buffer[pcmd->IdxOffset + e + 2];
                    ImDrawVert vtx3 = vtx_buffer[pcmd->VtxOffset + idx3];

                    gsKit_prim_triangle_goraud_texture(bd->Global, bd->FontTexture,
                        vtx1.pos.x, vtx1.pos.y, vtx1.uv.x, vtx1.uv.y,
                        vtx2.pos.x, vtx2.pos.y, vtx2.uv.x, vtx2.uv.y,
                        vtx3.pos.x, vtx3.pos.y, vtx3.uv.x, vtx3.uv.y,
                        100,
                        ImGui_ImplPs2GsKit_NormalizeImColor(vtx1.col), 
                        ImGui_ImplPs2GsKit_NormalizeImColor(vtx2.col), 
                        ImGui_ImplPs2GsKit_NormalizeImColor(vtx3.col));
                }

                gsKit_prim_sprite_texture(bd->Global, bd->FontTexture, 
                    50, 0, 0, 0, 
                    bd->FontTexture->Width + 50, bd->FontTexture->Height, bd->FontTexture->Width, bd->FontTexture->Height, 
                    50, 
                    GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00));
            }
        }
    }
}

int gsKit_texture2_finish(GSGLOBAL *gsGlobal, GSTEXTURE *Texture) {
    if (!Texture->Delayed) {
        Texture->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(Texture->Width, Texture->Height, Texture->PSM), GSKIT_ALLOC_USERBUFFER);
        if(Texture->Vram == GSKIT_ALLOC_ERROR)
        {
            printf("VRAM Allocation Failed. Will not upload texture.\n");
            return -1;
        }

        // Upload texture
        gsKit_texture_upload(gsGlobal, Texture);

        // Free texture
        free(Texture->Mem);
        Texture->Mem = NULL;
    } else {
        gsKit_setup_tbw(Texture);
    }

    return 0;
}

bool ImGui_ImplPs2GsKit_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPs2GsKit_Data* bd = ImGui_ImplPs2GsKit_GetBackendData();
    unsigned char* pixels;
    int width, height, bpp;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bpp);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders.

    // TODO: use gsKit to create a font texture
    bd->FontTexture = (GSTEXTURE *)calloc(1, sizeof(GSTEXTURE));
    bd->FontTexture->Delayed = 1;
    bd->FontTexture->Width = width;
    bd->FontTexture->Height = height;
    bd->FontTexture->PSM = GS_PSM_CT32;
    bd->FontTexture->Filter = GS_FILTER_NEAREST;
    bd->FontTexture->Clut = NULL;
    bd->FontTexture->VramClut = 0;

    // Copy the pixel data into the texture
    size_t textureSize = gsKit_texture_size_ee(bd->FontTexture->Width, bd->FontTexture->Height, bd->FontTexture->PSM);
    bd->FontTexture->Mem = (u32 *)memalign(128, textureSize);
    memcpy((void *)bd->FontTexture->Mem, (void *)pixels, textureSize);
    // memset((void *)bd->FontTexture->Mem, 0xFF, textureSize);

    u8 *texMem = (u8 *)bd->FontTexture->Mem;

    // Byte swap each byte?
    for (size_t i = 0; i < width * height * 4; i += 4) {
        u8 a = texMem[i],
            b = texMem[i + 1],
            g = texMem[i + 2],
            r = texMem[i + 3];
        
        texMem[i] = a;
        texMem[i + 1] = b;
        texMem[i + 2] = g;
        texMem[i + 3] = r;
    }

    gsKit_texture2_finish(bd->Global, bd->FontTexture);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->FontTexture);

    return true;
}

void ImGui_ImplPs2GsKit_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPs2GsKit_Data* bd = ImGui_ImplPs2GsKit_GetBackendData();
    if (bd->FontTexture)
    {
        // TODO: use gsKit to delete the font texture
        
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

bool    ImGui_ImplPs2GsKit_CreateDeviceObjects()
{
    return ImGui_ImplPs2GsKit_CreateFontsTexture();
}

void    ImGui_ImplPs2GsKit_DestroyDeviceObjects()
{
    ImGui_ImplPs2GsKit_DestroyFontsTexture();
}
