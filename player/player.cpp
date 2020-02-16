#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <SDL2/SDL.h>
#include "glad.h"
#if defined(__APPLE__)
#define USE_GLES3
#include <OpenGL/gl3.h>
#endif
#include "FreeSans.h"
#include "audio_sdl.h"
#include "decode.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//#define NK_INCLUDE_FIXED_TYPES
//#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"
#include "style.h"

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#ifdef USE_GLES3
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
#endif
static void       *_ctx;
static SDL_Window *_mainWindow;
static SDL_GLContext _mainContext;
static std::map<std::string, int> _previews;
static std::vector<std::string> _playlist;
static void *_render;
static bool _closeNextFrame;
decoder _dec;

static int load_image(const stbi_uc *data, int len)
{
    GLuint tex;
    int width, height, n;
    unsigned char *pix = stbi_load_from_memory(data, len, &width, &height, &n, 4);
    if (!pix)
        return 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
#ifndef USE_GLES3
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
#ifdef USE_GLES3
    if (glGenerateMipmap)
        glGenerateMipmap(GL_TEXTURE_2D);
#endif
    return (int)tex;
}

static void close()
{
    nk_sdl_shutdown();
    SDL_GL_MakeCurrent(_mainWindow, _mainContext);
    for (auto it = _previews.begin(); it != _previews.end(); it++)
    {
        GLuint tex = it->second;
        glDeleteTextures(1, &tex);
    }
    SDL_GL_DeleteContext(_mainContext);
    SDL_DestroyWindow(_mainWindow);
    SDL_Quit();
}

static int init()
{
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    if (!SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("error: sdl2 video init failed %s\n", SDL_GetError());
        return false;
    }
#ifdef USE_GLES3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // SDL_GL_SetAttribute(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    _mainWindow = SDL_CreateWindow("minimp3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 600, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    if (!_mainWindow)
    {
        printf("error: create window failed: %s\n", SDL_GetError()); 
        fflush(stdout);
        exit(1);
    }
    if((_mainContext = SDL_GL_CreateContext(_mainWindow)) == NULL)
    {
        printf("error: create context failed: %s\n", SDL_GetError()); 
        fflush(stdout);
        exit(1);
    }
    SDL_GL_MakeCurrent(_mainWindow, _mainContext);
    
#ifdef USE_GLES3
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)SDL_GL_GetProcAddress("glGenerateMipmap");
#endif

    gladLoadGL();
    _ctx = (void*)nk_sdl_init(_mainWindow);
    struct nk_font_atlas *atlas;
    struct nk_font_config fconfig = nk_font_config(20);
    fconfig.range = nk_font_cyrillic_glyph_ranges();
    nk_sdl_font_stash_begin(&atlas);
    atlas->default_font = nk_font_atlas_add_from_memory(atlas, FreeSans, FreeSansLen, 20, &fconfig);;
    nk_sdl_font_stash_end();
    return true;
}

static void tick()
{
    nk_context *ctx = (nk_context *)_ctx;
    
    float bg[4];
    nk_color_fv(bg, nk_rgb(28,48,62));
    int width, height, i;
    SDL_GL_GetDrawableSize(_mainWindow, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(bg[0], bg[1], bg[2], bg[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Run events
    SDL_Event event;
    nk_input_begin(ctx);
    while(SDL_PollEvent(&event))
    {
        nk_sdl_handle_event(&event);
        
        if(event.type == SDL_QUIT)
            _closeNextFrame = true;
    }
    nk_input_end(ctx);

    if (nk_begin(ctx, "Player", nk_rect(0, 0, width, height), 0))
    {
        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "Theme:", NK_TEXT_LEFT);
        static int _cur_theme;
        static const char *themes[] = { "black", "white", "red", "blue", "dark"};
        int res = nk_combo(ctx, themes, sizeof(themes)/sizeof(themes[0]), _cur_theme, 25, nk_vec2(200,200));
        if (_cur_theme != res)
            set_style(ctx, (theme)res);
        _cur_theme = res;

        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "Audio Playback Device:", NK_TEXT_LEFT);
        static int _cur_apdevice;
        const char **devs = (const char**)alloca(1*sizeof(char*));
        //for (i = 0; i < ; i++)
        //    devs[i] = ;
        devs[0] = "default";
        res = nk_combo(ctx, devs, 1, _cur_apdevice, 25, nk_vec2(200,200));
        //if (_cur_apdevice != res)
        //    change_device(res);
        _cur_apdevice = res;

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_button_push_behavior(ctx, NK_BUTTON_DEFAULT2);
        static int _play_state, _selected;
        if (!_play_state)
        {
            if (nk_button_label(ctx, "Play") && _selected < (int)_playlist.size())
            {
                _play_state = 1;
                sdl_audio_set_dec(_render, 0);
                open_dec(&_dec, _playlist[_selected].c_str());
                sdl_audio_set_dec(_render, &_dec);
            }
        } else
        {
            if (nk_button_label(ctx, "Stop"))
            {
                _play_state = 0;
                sdl_audio_set_dec(_render, 0);
                //stop();
            }
        }
        nk_button_pop_behavior(ctx);
        static size_t progress = 0;
        uint64_t cur_sample = _dec.mp3d.cur_sample;
        progress = cur_sample;
        nk_progress(ctx, &progress, _dec.mp3d.samples, NK_MODIFIABLE);
        if (progress != cur_sample)
        {
            SDL_PauseAudio(1);
            mp3dec_ex_seek(&_dec.mp3d, progress);
            SDL_PauseAudio(0);
        }

        if (nk_tree_push(ctx, NK_TREE_TAB, "Playlist", NK_MAXIMIZED))
        {
            for (i = 0; i < (int)_playlist.size(); i++)
            {
                nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
                nk_layout_row_push(ctx, 0.1f);
                auto av = _previews.find(_playlist[i].c_str());
                if (av != _previews.end())
                {
                    nk_image(ctx, nk_image_id(av->second));
                }
                nk_layout_row_push(ctx, 0.9f);
                const char *name = _playlist[i].c_str();
                if (nk_select_label(ctx, name, NK_TEXT_LEFT, _selected == i))
                    _selected = i;
                nk_layout_row_end(ctx);
            }
            nk_tree_pop(ctx);
        }
    }
    nk_end(ctx);
    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    SDL_GL_SwapWindow(_mainWindow);
}

int main(int argc, char *argv[])
{
    _closeNextFrame = false;
    sdl_audio_init(&_render, 44100, 2, 0, 0);
    init();
    for (int i = 1; i < argc; i++)
        _playlist.push_back(argv[i]);
    while (!_closeNextFrame)
    {
        tick();
    }
    close();
    sdl_audio_release(_render);
}
