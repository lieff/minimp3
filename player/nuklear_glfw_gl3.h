/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GLFW_GL3_H_
#define NK_GLFW_GL3_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void GLvoid;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef char GLchar;
typedef unsigned char GLboolean;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define GL_FALSE 0
#define GL_TRUE	1

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893

#define GL_TEXTURE_2D				0x0DE1
#define GL_TEXTURE_WRAP_S			0x2802
#define GL_TEXTURE_WRAP_T			0x2803
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801

#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#define GL_LINEAR_MIPMAP_LINEAR			0x2703

#define GL_LINEAR				0x2601
#define GL_NEAREST				0x2600
#define GL_REPEAT				0x2901
#define GL_CLAMP				0x2900

#define GL_FLOAT				0x1406
#define GL_UNSIGNED_BYTE			0x1401
#define GL_UNSIGNED_SHORT			0x1403
#define GL_RGBA					0x1908

#define GL_BLEND				0x0BE2
#define GL_FUNC_ADD				0x8006
#define GL_SRC_ALPHA				0x0302
#define GL_ONE_MINUS_SRC_ALPHA			0x0303

#define GL_CULL_FACE				0x0B44
#define GL_DEPTH_TEST				0x0B71
#define GL_SCISSOR_TEST				0x0C11

#define GL_TEXTURE0				0x84C0

#define GL_STREAM_DRAW                    0x88E0
#define GL_WRITE_ONLY                     0x88B9

#define GL_TRIANGLES				0x0004
#define GL_TRIANGLE_STRIP			0x0005
#define GL_TRIANGLE_FAN				0x0006

#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020

/*GLuint glCreateProgram (void);
void glDeleteProgram (GLuint program);
void glGetProgramiv (GLuint program, GLenum pname, GLint *params);

GLuint glCreateShader (GLenum type);
void glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
void glCompileShader (GLuint shader);
void glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
void glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void glAttachShader (GLuint program, GLuint shader);
void glLinkProgram (GLuint program);
void glUseProgram (GLuint program);
void glDeleteShader (GLuint shader);
void glDetachShader (GLuint program, GLuint shader);

GLint glGetUniformLocation (GLuint program, const GLchar *name);
GLint glGetAttribLocation (GLuint program, const GLchar *name);

void glGenBuffers (GLsizei n, GLuint *buffers);
void glDeleteBuffers (GLsizei n, const GLuint *buffers);
void glBindBuffer (GLenum target, GLuint buffer);

void glGenVertexArrays (GLsizei n, GLuint *arrays);
void glDeleteVertexArrays (GLsizei n, const GLuint *arrays);
void glBindVertexArray (GLuint array);

void glDisableVertexAttribArray (GLuint index);
void glEnableVertexAttribArray (GLuint index);
void glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

void glGenTextures( GLsizei n, GLuint *textures );
void glDeleteTextures( GLsizei n, const GLuint *textures);
void glBindTexture( GLenum target, GLuint texture );
void glTexParameteri( GLenum target, GLenum pname, GLint param );
void glTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                   GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void glActiveTexture( GLenum texture );

void glEnable( GLenum cap );
void glDisable( GLenum cap );

void glAlphaFunc( GLenum func, GLclampf ref );
void glBlendFunc( GLenum sfactor, GLenum dfactor );
void glBlendEquation( GLenum mode );

void glScissor( GLint x, GLint y, GLsizei width, GLsizei height);
void glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

void glUniform1f (GLint location, GLfloat v0);
void glUniform2f (GLint location, GLfloat v0, GLfloat v1);
void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform1i (GLint location, GLint v0);
void glUniform2i (GLint location, GLint v0, GLint v1);
void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
void glUniform2fv (GLint location, GLsizei count, const GLfloat *value);
void glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
void glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
void glUniform1iv (GLint location, GLsizei count, const GLint *value);
void glUniform2iv (GLint location, GLsizei count, const GLint *value);
void glUniform3iv (GLint location, GLsizei count, const GLint *value);
void glUniform4iv (GLint location, GLsizei count, const GLint *value);
void glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

void glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
#ifndef EMSCRIPTEN
void *glMapBuffer (GLenum target, GLenum access);
#endif
void *glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
GLboolean glUnmapBuffer (GLenum target);*/

enum nk_glfw_init_state{
    NK_GLFW3_DEFAULT=0,
    NK_GLFW3_INSTALL_CALLBACKS
};

NK_API struct nk_context*   nk_glfw3_init(GLFWwindow *win, enum nk_glfw_init_state);
NK_API void                 nk_glfw3_shutdown(void);
NK_API void                 nk_glfw3_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void                 nk_glfw3_font_stash_end(void);
NK_API void                 nk_glfw3_new_frame(void);
NK_API void                 nk_glfw3_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);

NK_API void                 nk_glfw3_device_destroy(void);
NK_API void                 nk_glfw3_device_create(void);

NK_API void                 nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint);
NK_API void                 nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff);
NK_API void                 nk_glfw3_mouse_button_callback(GLFWwindow *win, int button, int action, int mods);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GLFW_GL3_IMPLEMENTATION

#ifndef NK_GLFW_TEXT_MAX
#define NK_GLFW_TEXT_MAX 256
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_LO
#define NK_GLFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_HI
#define NK_GLFW_DOUBLE_CLICK_HI 0.2
#endif

struct nk_glfw_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};

struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

static struct nk_glfw {
    GLFWwindow *win;
    int width, height;
    int display_width, display_height;
    struct nk_glfw_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;
    unsigned int text[NK_GLFW_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
    double last_button_click;
} glfw;

#if defined(USE_GLES3)
#if defined(__APPLE__)
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif
#define ATTR "in "
#define VARYING "out "
#define INVARYNG "in "
#else
#define NK_SHADER_VERSION ""
#define ATTR "attribute "
#define VARYING "varying "
#define INVARYNG "varying "
#endif

static GLuint LoadShader(GLenum type, const char *shaderSrc)
{
    GLint compiled;
    GLuint shader = glCreateShader(type);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
       GLint infoLen = 0;
       glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
       if (infoLen > 1)
       {
           char *infoLog = (char*)malloc(sizeof(char)*infoLen);
           glGetShaderInfoLog (shader, infoLen, NULL, infoLog);
           printf("error: compiling shader failed:\n%s\n", infoLog);
           free(infoLog);
       }
       glDeleteShader(shader);
       return 0;
    }
    return shader;
}

NK_API void
nk_glfw3_device_create(void)
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        ATTR "vec2 Position;\n"
        ATTR "vec2 TexCoord;\n"
        ATTR "vec4 Color;\n"
        VARYING "vec2 Frag_UV;\n"
        VARYING "vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
#if defined(USE_GLES3)
        "precision mediump float;\n"
#endif
        "uniform sampler2D Texture;\n"
        INVARYNG "vec2 Frag_UV;\n"
        INVARYNG "vec4 Frag_Color;\n"
#if defined(USE_GLES3)
        "out vec4 Out_Color;\n"
#endif
        "void main(){\n"
#if defined(USE_GLES3)
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
#else
        "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
#endif
        "}\n";

    struct nk_glfw_device *dev = &glfw.ogl;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = LoadShader(GL_VERTEX_SHADER, vertex_shader);
    dev->frag_shdr = LoadShader(GL_FRAGMENT_SHADER, fragment_shader);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_glfw_vertex);
        size_t vp = offsetof(struct nk_glfw_vertex, position);
        size_t vt = offsetof(struct nk_glfw_vertex, uv);
        size_t vc = offsetof(struct nk_glfw_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NK_INTERN void
nk_glfw3_device_upload_atlas(const void *image, int width, int height)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_glfw3_device_destroy(void)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

NK_API void
nk_glfw3_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)glfw.width;
    ortho[1][1] /= (GLfloat)glfw.height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    //glViewport(0,0,(GLsizei)glfw.display_width,(GLsizei)glfw.display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        //vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        //elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        vertices = glMapBufferRange(GL_ARRAY_BUFFER, 0, max_vertex_buffer, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        elements = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, max_element_buffer, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_glfw_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(&glfw.ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &glfw.ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * glfw.fb_scale.x),
                (GLint)((glfw.height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * glfw.fb_scale.y),
                (GLint)(cmd->clip_rect.w * glfw.fb_scale.x),
                (GLint)(cmd->clip_rect.h * glfw.fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&glfw.ctx);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API void
nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint)
{
    (void)win;
    if (glfw.text_len < NK_GLFW_TEXT_MAX)
        glfw.text[glfw.text_len++] = codepoint;
}

NK_API void
nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff)
{
    (void)win; (void)xoff;
    glfw.scroll.x += (float)xoff;
    glfw.scroll.y += (float)yoff;
}

NK_API void
nk_glfw3_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    glfwGetCursorPos(window, &x, &y);
    if (action == GLFW_PRESS)  {
        double dt = glfwGetTime() - glfw.last_button_click;
        if (dt > NK_GLFW_DOUBLE_CLICK_LO && dt < NK_GLFW_DOUBLE_CLICK_HI)
            nk_input_button(&glfw.ctx, NK_BUTTON_DOUBLE, (int)x, (int)y, nk_true);
        glfw.last_button_click = glfwGetTime();
    } else nk_input_button(&glfw.ctx, NK_BUTTON_DOUBLE, (int)x, (int)y, nk_false);
}

NK_INTERN void
nk_glfw3_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = glfwGetClipboardString(glfw.win);
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

NK_INTERN void
nk_glfw3_clipbard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    glfwSetClipboardString(glfw.win, str);
    free(str);
}

NK_API struct nk_context*
nk_glfw3_init(GLFWwindow *win, enum nk_glfw_init_state init_state)
{
    glfw.win = win;
    if (init_state == NK_GLFW3_INSTALL_CALLBACKS) {
        glfwSetScrollCallback(win, nk_gflw3_scroll_callback);
        glfwSetCharCallback(win, nk_glfw3_char_callback);
        glfwSetMouseButtonCallback(win, nk_glfw3_mouse_button_callback);
    }
    nk_init_default(&glfw.ctx, 0);
    glfw.ctx.clip.copy = nk_glfw3_clipbard_copy;
    glfw.ctx.clip.paste = nk_glfw3_clipbard_paste;
    glfw.ctx.clip.userdata = nk_handle_ptr(0);
    glfw.last_button_click = 0;
    nk_glfw3_device_create();
    return &glfw.ctx;
}

NK_API void
nk_glfw3_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&glfw.atlas);
    nk_font_atlas_begin(&glfw.atlas);
    *atlas = &glfw.atlas;
}

NK_API void
nk_glfw3_font_stash_end(void)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&glfw.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_glfw3_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&glfw.atlas, nk_handle_id((int)glfw.ogl.font_tex), &glfw.ogl.null);
    if (glfw.atlas.default_font)
        nk_style_set_font(&glfw.ctx, &glfw.atlas.default_font->handle);
}

NK_API void
nk_glfw3_new_frame(void)
{
    int i;
    double x, y;
    struct nk_context *ctx = &glfw.ctx;
    struct GLFWwindow *win = glfw.win;

    /*glfw.width  = winWidth;
    glfw.height = winHeight;
    glfw.display_width = width;
    glfw.display_height = height;*/
    glfwGetWindowSize(win, &glfw.width, &glfw.height);
    glfwGetFramebufferSize(win, &glfw.display_width, &glfw.display_height);
    glfw.fb_scale.x = (float)glfw.display_width/(float)glfw.width;
    glfw.fb_scale.y = (float)glfw.display_height/(float)glfw.height;

    nk_input_begin(ctx);
    for (i = 0; i < glfw.text_len; ++i)
        nk_input_unicode(ctx, glfw.text[i]);

#if NK_GLFW_GL3_MOUSE_GRABBING
    /* optional grabbing behavior */
    if (ctx->input.mouse.grab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else if (ctx->input.mouse.ungrab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif

    nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SHIFT, glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS||
                                    glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    glfwGetCursorPos(win, &x, &y);
    //x = mx;
    //y = my;
    nk_input_motion(ctx, (int)x, (int)y);
#if NK_GLFW_GL3_MOUSE_GRABBING
    if (ctx->input.mouse.grabbed) {
        glfwSetCursorPos(glfw.win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
#endif
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    /*nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, mkeys & 1);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, mkeys & 2);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, mkeys & 4);*/

    nk_input_scroll(ctx, glfw.scroll);
    nk_input_end(&glfw.ctx);
    glfw.text_len = 0;
    glfw.scroll = nk_vec2(0,0);
}

NK_API
void nk_glfw3_shutdown(void)
{
    nk_font_atlas_clear(&glfw.atlas);
    nk_free(&glfw.ctx);
    nk_glfw3_device_destroy();
    memset(&glfw, 0, sizeof(glfw));
}

#ifdef __cplusplus
}
#endif
#endif
