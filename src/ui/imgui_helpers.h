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

namespace UI::GPU
{
    static SDL_GPUDevice* g_GpuDevice = nullptr;
    static ImFont* g_MonoFont = nullptr;

    static void ApplyModernTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        
        // Exact VS Code Dark+ Theme Color
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImColor(204, 204, 204, 255); 
        colors[ImGuiCol_TextDisabled]           = ImColor(128, 128, 128, 255); 
        colors[ImGuiCol_WindowBg]               = ImColor(30, 30, 30, 255);    // #1e1e1e
        colors[ImGuiCol_ChildBg]                = ImColor(30, 30, 30, 255);    // #1e1e1e
        colors[ImGuiCol_PopupBg]                = ImColor(37, 37, 38, 255);    // #252526
        colors[ImGuiCol_Border]                 = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_BorderShadow]           = ImColor(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg]                = ImColor(45, 45, 48, 255);    // #2d2d30
        colors[ImGuiCol_FrameBgHovered]         = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_FrameBgActive]          = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_TitleBg]                = ImColor(37, 37, 38, 255);    // #252526
        colors[ImGuiCol_TitleBgActive]          = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_TitleBgCollapsed]       = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_MenuBarBg]              = ImColor(37, 37, 38, 255);
        colors[ImGuiCol_ScrollbarBg]            = ImColor(30, 30, 30, 255);
        colors[ImGuiCol_ScrollbarGrab]          = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImColor(79, 79, 83, 255); 
        colors[ImGuiCol_ScrollbarGrabActive]    = ImColor(95, 95, 100, 255); 
        colors[ImGuiCol_CheckMark]              = ImColor(0, 122, 204, 255);   // #007acc
        colors[ImGuiCol_SliderGrab]             = ImColor(0, 122, 204, 255);   // #007acc
        colors[ImGuiCol_SliderGrabActive]       = ImColor(0, 122, 204, 255);
        colors[ImGuiCol_Button]                 = ImColor(0, 122, 204, 255);   // #007acc
        colors[ImGuiCol_ButtonHovered]          = ImColor(28, 151, 234, 255);  
        colors[ImGuiCol_ButtonActive]           = ImColor(0, 122, 204, 255);  
        colors[ImGuiCol_Header]                 = ImColor(45, 45, 48, 255);    // #2d2d30
        colors[ImGuiCol_HeaderHovered]          = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_HeaderActive]           = ImColor(45, 45, 48, 255);
        colors[ImGuiCol_Separator]              = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_SeparatorHovered]       = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_SeparatorActive]        = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGrip]             = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGripHovered]      = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_ResizeGripActive]       = ImColor(62, 62, 66, 255);
        colors[ImGuiCol_Tab]                    = ImColor(45, 45, 48, 255);    // #2d2d30
        colors[ImGuiCol_TabHovered]             = ImColor(62, 62, 66, 255);    // #3e3e42
        colors[ImGuiCol_TabActive]              = ImColor(30, 30, 30, 255);    // #1e1e1e
        colors[ImGuiCol_TabUnfocused]           = ImColor(45, 45, 48, 255);
        colors[ImGuiCol_TabUnfocusedActive]     = ImColor(30, 30, 30, 255);

        // Styling
        style.WindowRounding    = 8.0f; 
        style.ChildRounding     = 6.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 6.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 4.0f;

        style.WindowPadding     = ImVec2(10.0f, 10.0f);
        style.FramePadding      = ImVec2(8.0f, 6.0f);
        style.ItemSpacing       = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);
    }

    static SDL_Window* Initialize(float* out_scale)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
            return nullptr;
        }

        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        if (main_scale <= 0.0f)
            main_scale = 1.0f;
        if (out_scale) *out_scale = main_scale;

        SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        SDL_Window* window = SDL_CreateWindow(
            "openrequest",
            static_cast<int>(1280 * main_scale),
            static_cast<int>(800 * main_scale),
            window_flags
        );

        if (!window)
        {
            fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return nullptr;
        }

        // Set Window and Taskbar Icon
        SDL_Surface* icon = SDL_CreateSurfaceFrom(
            APP_ICON_48_WIDTH, APP_ICON_48_HEIGHT,
            SDL_PIXELFORMAT_RGBA32,
            const_cast<unsigned char*>(APP_ICON_48_PIXELS),
            APP_ICON_48_WIDTH * 4
        );
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
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Enable FreeType font rasterizer
        io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

#ifdef MONO_FONT_PATH
        ImFont* mono = io.Fonts->AddFontFromFileTTF(MONO_FONT_PATH, (float)EDITOR_FONT_SIZE);
        if (!mono)
            mono = io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", (float)EDITOR_FONT_SIZE);
        if (mono)
            g_MonoFont = mono;
#else
        ImFont* mono = io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", (float)EDITOR_FONT_SIZE);
        if (mono)
            g_MonoFont = mono;
#endif

        ApplyModernTheme();

        ImGuiStyle& style = ImGui::GetStyle();
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

    static void HandleResize(SDL_Window*)
    {
        // SDL_GPU handles swapchain resize automatically when acquiring swapchain texture
    }

    static void RenderFrame(SDL_Window* window, ImDrawData* draw_data, const ImVec4& clear_color)
    {
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(g_GpuDevice);
        if (!command_buffer)
            return;

        SDL_GPUTexture* swapchain_texture = nullptr;
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
            target_info.clear_color = SDL_FColor{ clear_color.x, clear_color.y, clear_color.z, clear_color.w };
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;

            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            if (render_pass)
            {
                ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
                SDL_EndGPURenderPass(render_pass);
            }
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    static void Cleanup(SDL_Window* window)
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
} // namespace UI::GPU

namespace UI
{
    namespace Vulkan = GPU;
}