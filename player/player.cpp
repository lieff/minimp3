#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include "glad.h"
#include <GLFW/glfw3.h>
#if defined(__APPLE__)
#define USE_GLES3
#include <OpenGL/gl3.h>
#endif
#include "FreeSans.h"
#include "../minimp3.h"

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
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"
#include "style.h"

#define MAX_VERTEX_BUFFER  512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#ifdef USE_GLES3
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
#endif
void       *_ctx;
GLFWwindow *_mainWindow;
std::map<std::string, int> _previews;
std::vector<std::string> _playlist;

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
    glGenerateMipmap(GL_TEXTURE_2D);
#endif
    return (int)tex;
}

static void close()
{
    glfwMakeContextCurrent(_mainWindow);
    for (auto it = _previews.begin(); it != _previews.end(); it++)
    {
        GLuint tex = it->second;
        glDeleteTextures(1, &tex);
    }
    glfwDestroyWindow(_mainWindow);
    glfwTerminate();
}

static int init()
{
    if (!glfwInit())
    {
        printf("error: glfw init failed\n");
        return false;
    }
#ifdef USE_GLES3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 1);
    _mainWindow = glfwCreateWindow(400, 600, "Lion Audio Player", NULL, NULL);
    if (!_mainWindow)
    {
        printf("error: create window failed\n"); fflush(stdout);
        exit(1);
    }
    glfwMakeContextCurrent(_mainWindow);
#ifdef USE_GLES3
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)glfwGetProcAddress("glGenerateMipmap");
#endif
    glfwSetInputMode(_mainWindow, GLFW_STICKY_MOUSE_BUTTONS, 1);

    gladLoadGL();
    _ctx = (void*)nk_glfw3_init(_mainWindow, NK_GLFW3_INSTALL_CALLBACKS);
    struct nk_font_atlas *atlas;
    struct nk_font_config fconfig = nk_font_config(20);
    fconfig.range = nk_font_cyrillic_glyph_ranges();
    nk_glfw3_font_stash_begin(&atlas);
    atlas->default_font = nk_font_atlas_add_from_memory(atlas, FreeSans, FreeSansLen, 20, &fconfig);;
    nk_glfw3_font_stash_end();
}

static void tick()
{
    //glfwMakeContextCurrent(_mainWindow);
    float bg[4];
    nk_color_fv(bg, nk_rgb(28,48,62));
    int width, height, i;
    glfwGetWindowSize(_mainWindow, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(bg[0], bg[1], bg[2], bg[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwPollEvents();

    nk_context *ctx = (nk_context *)_ctx;
    nk_glfw3_new_frame();
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
                //play();
            }
        } else
        {
            if (nk_button_label(ctx, "Stop"))
            {
                _play_state = 0;
                //stop();
            }
        }
        nk_button_pop_behavior(ctx);

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
    nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(_mainWindow);
    //glfwMakeContextCurrent(0);
}

int main(int argc, char *argv[])
{
    init();
    while (!glfwWindowShouldClose(_mainWindow))
    {
        tick();
    }
    close();
}
