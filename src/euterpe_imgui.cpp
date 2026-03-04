// euterpe_imgui.cpp
//
// Dear ImGui + GLFW entry point for euterpe.
// Supports both line-in (PortAudio) and WAV file input.
//
// Pipeline:
//   AudioSource -> inBuffer -> HPSS_long -> HPSS_short
//                -> StreamAdder -> TempoPitch -> outBuffer -> PortAudio

// macOS: silence OpenGL deprecation warnings (we target OpenGL 3.3 core)
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#ifndef NO_NFD
#include <nfd.h>
#endif

#include <fftw3.h>          // must precede HPSS_pipe_long.h

#include "guiBase.h"
#include "streamBuffer.h"
#include "HPSS_pipe_long.h"
#include "streamAdder.h"
#include "tempoPitch.h"
#include "lineAudioSource.h"
#include "fileAudioSource.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    // --- GLFW + OpenGL 3.3 core ---
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(460, 320, "Euterpe", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync

    // Content scale (2.0 on Retina/HiDPI displays)
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    // --- ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // disable imgui.ini

    // Load a system TrueType font at HiDPI-scaled size, fall back to default
    const float BASE_PX = 15.0f;
    const float FONT_PX = BASE_PX * xscale;
    bool fontLoaded = false;
#ifdef __APPLE__
    for (const char* path : {"/System/Library/Fonts/Helvetica.ttc",
                              "/System/Library/Fonts/Arial.ttf"}) {
        if (io.Fonts->AddFontFromFileTTF(path, FONT_PX)) { fontLoaded = true; break; }
    }
#elif defined(__linux__)
    for (const char* path : {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                              "/usr/share/fonts/TTF/DejaVuSans.ttf",
                              "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"}) {
        if (io.Fonts->AddFontFromFileTTF(path, FONT_PX)) { fontLoaded = true; break; }
    }
#endif
    if (!fontLoaded) {
        ImFontConfig cfg;
        cfg.SizePixels = FONT_PX;
        io.Fonts->AddFontDefault(&cfg);
    }
    // Scale UI back to logical size so widgets remain the same visual size
    io.FontGlobalScale = 1.0f / xscale;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // --- Audio pipeline ---
    constexpr int freq = 16000;
    constexpr int spb  = 1024;

    StreamBuffer<float> inBuffer, Buffer_H_short, Buffer_P_short;
    StreamBuffer<float> Buffer_H_long, Buffer_P_long, Buffer_karaoke, outBuffer;

    GUIBase params;

    HPSS_pipe      HPSS_short(7,  512,  256, 10, 10);
    HPSS_short.setBuffer(&Buffer_P_long, &Buffer_H_short, &Buffer_P_short);

    HPSS_pipe_long HPSS_long(7, 2048, 1024, 10, 10);
    HPSS_long.setBuffer(&inBuffer, &Buffer_H_long, &Buffer_P_long);

    StreamAdder adder(2048, &params);
    adder.add_input_stream(&Buffer_P_short); adder.set_multiply(0, 1.0);
    adder.add_input_stream(&Buffer_H_long);  adder.set_multiply(1, 1.0);
    adder.add_input_stream(&Buffer_H_short); adder.set_multiply(2, 0.0);
    adder.set_output_stream(&Buffer_karaoke);

    TempoPitch converter;
    converter.init(1024 * 2, 128 * 2, 1, 7, 15, freq, &params);
    converter.setBuffer(&Buffer_karaoke, &outBuffer);

    // --- UI state ---
    int   key      = 0;
    int   volume   = 100;
    int   mode     = 0;       // 0 = line-in, 1 = file
    char  filePath[2048] = {};
    bool  running  = false;
    char  statusMsg[128] = "Ready";
    std::unique_ptr<AudioSource> source;

    // --- Main loop ---
    while (!glfwWindowShouldClose(window) && !params.getquit()) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Fullscreen panel
        const auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##main", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoSavedSettings);

        ImGui::TextUnformatted("Euterpe – Automatic Karaoke Generator");
        ImGui::Separator();

        // Key & volume sliders (active only while running)
        ImGui::BeginDisabled(!running);
        if (ImGui::SliderInt("Key (semitones)", &key, -12, 12))
            params.key.set(key);
        if (ImGui::SliderInt("Volume", &volume, 0, 200))
            params.volume.set(volume);
        ImGui::EndDisabled();

        ImGui::Separator();

        // Input source (disabled while running)
        ImGui::BeginDisabled(running);
        ImGui::TextUnformatted("Input source:");
        ImGui::RadioButton("Line-in", &mode, 0); ImGui::SameLine();
        ImGui::RadioButton("Audio file", &mode, 1);

        if (mode == 1) {
            ImGui::InputText("##file", filePath, sizeof(filePath));
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
#ifndef NO_NFD
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filters[] = {{"Audio files", "wav,flac,ogg,aif,aiff"}};
                if (NFD_Init() == NFD_OKAY) {
                    if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY) {
                        strncpy(filePath, outPath, sizeof(filePath) - 1);
                        NFD_FreePathU8(outPath);
                    }
                    NFD_Quit();
                }
#endif
            }
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        // Start / Quit
        if (!running) {
            if (ImGui::Button("Start", ImVec2(90, 0))) {
                bool ok = true;
                if (mode == 1 && filePath[0] == '\0') {
                    snprintf(statusMsg, sizeof(statusMsg), "No file selected.");
                    ok = false;
                }
                if (ok) {
                    HPSS_short.start();
                    HPSS_long.start();
                    adder.start();
                    converter.start();
                    if (mode == 0) {
                        source = std::make_unique<LineAudioSource>(freq, spb, outBuffer);
                    } else {
                        source = std::make_unique<FileAudioSource>(
                            std::filesystem::path(filePath), freq, spb, outBuffer);
                    }
                    source->start(inBuffer);
                    running = true;
                    snprintf(statusMsg, sizeof(statusMsg), "Running...");
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit", ImVec2(90, 0))) {
            params.setquit();
        }

        // Status line
        ImGui::Spacing();
        if (running) {
            ImGui::Text("buf: in=%dms  kar=%dms  out=%dms",
                static_cast<int>(inBuffer.size()       * 1000 / freq),
                static_cast<int>(Buffer_karaoke.size() * 1000 / freq),
                static_cast<int>(outBuffer.size()      * 1000 / freq));
        } else {
            ImGui::TextUnformatted(statusMsg);
        }

        ImGui::End();

        // Render
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    if (source) source->stop();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
