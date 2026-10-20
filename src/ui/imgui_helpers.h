#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "misc/freetype/imgui_freetype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "app_icon.h"
#include "native_window.h"
#include "stb_image.h"

namespace UI::GPU
{
    static SDL_GPUDevice *g_GpuDevice = nullptr;
    static ImFont *g_MonoFont = nullptr;

    static void ApplyModernTheme()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // Exact VS Code Dark+ Theme Color
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImColor(204, 204, 204, 255);
        colors[ImGuiCol_TextDisabled] = ImColor(128, 128, 128, 255);
        colors[ImGuiCol_WindowBg] = ImColor(30, 30, 30, 255); // #1e1e1e
        colors[ImGuiCol_ChildBg] = ImColor(30, 30, 30, 255);  // #1e1e1e
        colors[ImGuiCol_PopupBg] = ImColor(37, 37, 38, 255);  // #252526
        colors[ImGuiCol_Border] = ImColor(62, 62, 66, 255);   // #3e3e42
        colors[ImGuiCol_BorderShadow] = ImColor(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg] = ImColor(45, 45, 48, 255);        // #2d2d30
        colors[ImGuiCol_FrameBgHovered] = ImColor(62, 62, 66, 255); // #3e3e42
        colors[ImGuiCol_FrameBgActive] = ImColor(62, 62, 66, 255);  // #3e3e42
        colors[ImGuiCol_TitleBg] = ImColor(37, 37, 38, 255);        // #252526
        colors[ImGuiCol_TitleBgActive] = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_TitleBgCollapsed] = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_MenuBarBg] = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_ScrollbarBg] = ImColor(30, 30, 30, 255);
        colors[ImGuiCol_ScrollbarGrab] = ImColor(62, 62, 66, 255); // #3e3e42
        colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(79, 79, 83, 255);
        colors[ImGuiCol_ScrollbarGrabActive] = ImColor(95, 95, 100, 255);
        colors[ImGuiCol_CheckMark] = ImColor(0, 122, 204, 255);  // #007acc
        colors[ImGuiCol_SliderGrab] = ImColor(0, 122, 204, 255); // #007acc
        colors[ImGuiCol_SliderGrabActive] = ImColor(0, 122, 204, 255);
        colors[ImGuiCol_Button] = ImColor(0, 122, 204, 255); // #007acc
        colors[ImGuiCol_ButtonHovered] = ImColor(28, 151, 234, 255);
        colors[ImGuiCol_ButtonActive] = ImColor(0, 122, 204, 255);
        colors[ImGuiCol_Header] = ImColor(45, 45, 48, 255);        // #2d2d30
        colors[ImGuiCol_HeaderHovered] = ImColor(62, 62, 66, 255); // #3e3e42
        colors[ImGuiCol_HeaderActive] = ImColor(45, 45, 48, 255);
        colors[ImGuiCol_Separator] = ImColor(62, 62, 66, 255); // #3e3e42
        colors[ImGuiCol_SeparatorHovered] = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_SeparatorActive] = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGrip] = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGripHovered] = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGripActive] = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_Tab] = ImColor(45, 45, 48, 255);        // #2d2d30
        colors[ImGuiCol_TabHovered] = ImColor(62, 62, 66, 255); // #3e3e42
        colors[ImGuiCol_TabActive] = ImColor(30, 30, 30, 255);  // #1e1e1e
        colors[ImGuiCol_TabUnfocused] = ImColor(45, 45, 48, 255);
        colors[ImGuiCol_TabUnfocusedActive] = ImColor(30, 30, 30, 255);

        // Styling
        style.WindowRounding = 8.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(8.0f, 6.0f);
        style.ItemSpacing = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    }

    static SDL_Window *Initialize(float *out_scale)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
            return nullptr;
        }

        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        if (main_scale <= 0.0f)
            main_scale = 1.0f;
        if (out_scale)
            *out_scale = main_scale;

        SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        SDL_Window *window = SDL_CreateWindow(
            "openrequest",
            static_cast<int>(1280 * main_scale),
            static_cast<int>(800 * main_scale),
            window_flags);

        if (!window)
        {
            fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return nullptr;
        }

        // Set Window and Taskbar Icon
        SDL_Surface *icon = SDL_CreateSurfaceFrom(
            APP_ICON_48_WIDTH, APP_ICON_48_HEIGHT,
            SDL_PIXELFORMAT_RGBA32,
            const_cast<unsigned char *>(APP_ICON_48_PIXELS),
            APP_ICON_48_WIDTH * 4);
        if (icon)
        {
            SDL_SetWindowIcon(window, icon);
            SDL_DestroySurface(icon);
        }

        UI::InitializeNativeWindow(window);

        // Create SDL GPU Device
        SDL_GPUShaderFormat format_flags = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB;
        g_GpuDevice = SDL_CreateGPUDevice(format_flags, false, nullptr);
        if (!g_GpuDevice)
        {
            fprintf(stderr, "Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
            return nullptr;
        }

        // Claim window for GPU Device
        if (!SDL_ClaimWindowForGPUDevice(g_GpuDevice, window))
        {
            fprintf(stderr, "Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
            return nullptr;
        }
        SDL_SetGPUSwapchainParameters(g_GpuDevice, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);

        // Setup Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Enable FreeType font rasterizer
        io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

#ifdef MONO_FONT_PATH
        ImFont *mono = io.Fonts->AddFontFromFileTTF(MONO_FONT_PATH, (float)EDITOR_FONT_SIZE);
        if (!mono)
            mono = io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", (float)EDITOR_FONT_SIZE);
        if (mono)
            g_MonoFont = mono;
#else
        ImFont *mono = io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", (float)EDITOR_FONT_SIZE);
        if (mono)
            g_MonoFont = mono;
#endif

        ApplyModernTheme();

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU(window);

        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device = g_GpuDevice;
        init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(g_GpuDevice, window);
        init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
        init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
        ImGui_ImplSDLGPU3_Init(&init_info);

        return window;
    }

    static void HandleResize(SDL_Window *)
    {
        // SDL_GPU handles swapchain resize automatically when acquiring swapchain texture
    }

    static void RenderFrame(SDL_Window *window, ImDrawData *draw_data, const ImVec4 &clear_color)
    {
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(g_GpuDevice);
        if (!command_buffer)
            return;

        SDL_GPUTexture *swapchain_texture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr))
        {
            SDL_CancelGPUCommandBuffer(command_buffer);
            return;
        }

        if (swapchain_texture != nullptr && !is_minimized)
        {
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{clear_color.x, clear_color.y, clear_color.z, clear_color.w};
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;

            SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            if (render_pass)
            {
                ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
                SDL_EndGPURenderPass(render_pass);
            }
        }

        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    static void Cleanup(SDL_Window *window)
    {
        if (g_GpuDevice)
        {
            SDL_WaitForGPUIdle(g_GpuDevice);
        }

        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();

        if (g_GpuDevice)
        {
            SDL_ReleaseWindowFromGPUDevice(g_GpuDevice, window);
            SDL_DestroyGPUDevice(g_GpuDevice);
            g_GpuDevice = nullptr;
        }

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    inline SDL_GPUTexture *g_openrequest_logo = nullptr;

    inline bool LoadTextureFromMemory(const void *data, size_t data_size, SDL_GPUTexture **out_texture, int *out_width, int *out_height)
    {
        SDL_GPUDevice *device = g_GpuDevice;
        // Load from disk into a raw RGBA buffer
        int image_width = 0;
        int image_height = 0;
        unsigned char *image_data = stbi_load_from_memory((const unsigned char *)data, (int)data_size, &image_width, &image_height, NULL, 4);
        if (image_data == NULL)
            return false;

        // Create texture
        SDL_GPUTextureCreateInfo texture_info = {};
        texture_info.type = SDL_GPU_TEXTURETYPE_2D;
        texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture_info.width = image_width;
        texture_info.height = image_height;
        texture_info.layer_count_or_depth = 1;
        texture_info.num_levels = 1;
        texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &texture_info);

        // Create transfer buffer
        // FIXME: A real engine would likely keep one around, see what the SDL_GPU backend is doing.
        SDL_GPUTransferBufferCreateInfo transferbuffer_info = {};
        transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferbuffer_info.size = image_width * image_height * 4;
        SDL_GPUTransferBuffer *transferbuffer = SDL_CreateGPUTransferBuffer(device, &transferbuffer_info);
        IM_ASSERT(transferbuffer != NULL);

        // Copy to transfer buffer
        uint32_t upload_pitch = image_width * 4;
        void *texture_ptr = SDL_MapGPUTransferBuffer(device, transferbuffer, true);
        for (int y = 0; y < image_height; y++)
            memcpy((void *)((uintptr_t)texture_ptr + y * upload_pitch), image_data + y * upload_pitch, upload_pitch);
        SDL_UnmapGPUTransferBuffer(device, transferbuffer);

        SDL_GPUTextureTransferInfo transfer_info = {};
        transfer_info.offset = 0;
        transfer_info.transfer_buffer = transferbuffer;

        SDL_GPUTextureRegion texture_region = {};
        texture_region.texture = texture;
        texture_region.x = (Uint32)0;
        texture_region.y = (Uint32)0;
        texture_region.w = (Uint32)image_width;
        texture_region.h = (Uint32)image_height;
        texture_region.d = 1;

        // Upload
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);
        SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(cmd);

        SDL_ReleaseGPUTransferBuffer(device, transferbuffer);

        *out_texture = texture;
        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    // Open and read a file, then forward to LoadTextureFromMemory()
    inline bool LoadTextureFromFile(const char *file_name, SDL_GPUTexture **out_texture, int *out_width, int *out_height)
    {
        FILE *f = fopen(file_name, "rb");
        if (f == NULL)
            return false;
        fseek(f, 0, SEEK_END);
        size_t file_size = (size_t)ftell(f);
        if (file_size == -1)
            return false;
        fseek(f, 0, SEEK_SET);
        void *file_data = IM_ALLOC(file_size);
        fread(file_data, 1, file_size, f);
        fclose(f);
        bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
        IM_FREE(file_data);
        return ret;
    }
    struct Texture{
        SDL_GPUTexture* texture;
        int image_width, image_height;
    };

    inline void DestroyTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture)
    {
        SDL_ReleaseGPUTexture(device, texture);
    }

    inline bool load_logo(std::string path ,Texture * tex)
    {
        return LoadTextureFromFile(path.c_str(), &(tex->texture), &(tex->image_width), &(tex->image_height));
    }
} // namespace UI::GPU

namespace UI
{
    namespace Vulkan = GPU;
}
