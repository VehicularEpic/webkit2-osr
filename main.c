#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <webkit2/webkit2.h>

typedef struct WebViewData
{
    GtkOffscreenWindow *gtkOffscreenWindow;
    WebKitWebView *webKitWebView;

    GLuint webViewTexture;
    GLuint webViewShaderProgram;
    GLuint webViewScreenQuad;
} WebViewData;

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error: %s\n", description);
}

static GLFWwindow *create_main_window()
{
    GLFWwindow *win_ptr;
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
        return NULL;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    win_ptr = glfwCreateWindow(
        800, 600, "WebKit2GTK Offscreen Rendering", NULL, NULL);

    if (!win_ptr)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
        return NULL;
    }

    glfwMakeContextCurrent(win_ptr);
    gladLoadGL(glfwGetProcAddress);

    glfwSwapInterval(1);
    return win_ptr;
}

static void create_offscreen_window(WebViewData *data_ptr)
{
    if (!gtk_init_check(NULL, NULL))
    {
        fprintf(stderr, "Unable to initialize GTK+\n");
        return exit(EXIT_FAILURE);
    }

    data_ptr->gtkOffscreenWindow = GTK_OFFSCREEN_WINDOW(gtk_offscreen_window_new());
    gtk_window_set_default_size(
        GTK_WINDOW(data_ptr->gtkOffscreenWindow), 800, 600);
}

static void create_webview(WebViewData *data_ptr)
{
    if (!data_ptr->gtkOffscreenWindow)
    {
        fprintf(stderr, "Offscreen Window not initialized, cannot create WebView\n");
        return exit(EXIT_FAILURE);
    }

    WebKitSettings *settings = webkit_settings_new();

    webkit_settings_set_enable_webgl(settings, true);
    webkit_settings_set_hardware_acceleration_policy(
        settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);

    data_ptr->webKitWebView = WEBKIT_WEB_VIEW(
        webkit_web_view_new_with_settings(settings));

    gtk_container_add(
        GTK_CONTAINER(data_ptr->gtkOffscreenWindow),
        GTK_WIDGET(data_ptr->webKitWebView));

    gtk_widget_grab_focus(GTK_WIDGET(data_ptr->webKitWebView));
}

static void create_webview_texture(WebViewData *data_ptr)
{
    glGenTextures(1, &data_ptr->webViewTexture);
    glBindTexture(GL_TEXTURE_2D, data_ptr->webViewTexture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, 800, 600, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void create_webview_shader(WebViewData *data_ptr)
{
    const char *vertexShaderText =
        "#version 330 core\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 TexturePosition;\n"
        "out vec2 Coordinates;\n"
        "void main() {\n"
        "    gl_Position = vec4(Position.xy, 0.0, 1.0);\n"
        "    Coordinates = TexturePosition;\n"
        "}\n";

    const char *fragmentShaderText =
        "#version 330 core\n"
        "in vec2 Coordinates;\n"
        "out vec4 OutputColor;\n"
        "uniform sampler2D Texture;\n"
        "void main() {\n"
        "    OutputColor = texture(Texture, Coordinates);\n"
        "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderText, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderText, NULL);
    glCompileShader(fragmentShader);

    data_ptr->webViewShaderProgram = glCreateProgram();
    glAttachShader(
        data_ptr->webViewShaderProgram, vertexShader);
    glAttachShader(
        data_ptr->webViewShaderProgram, fragmentShader);
    glLinkProgram(data_ptr->webViewShaderProgram);

    glDetachShader(data_ptr->webViewShaderProgram, vertexShader);
    glDeleteShader(vertexShader);

    glDetachShader(data_ptr->webViewShaderProgram, fragmentShader);
    glDeleteShader(fragmentShader);
}

static void create_webview_quad_object(WebViewData *data_ptr)
{
    GLuint indices[6] = {0, 1, 3, 0, 3, 2};
    GLfloat vertices[8] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    GLfloat textures[8] = {0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f};

    GLuint buffers[3];
    glGenBuffers(3, &buffers[0]);

    glGenVertexArrays(1, &data_ptr->webViewScreenQuad);
    glBindVertexArray(data_ptr->webViewScreenQuad);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textures), textures, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    WebViewData *data = (WebViewData *)glfwGetWindowUserPointer(window);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glViewport(0, 0, width, height);

    gtk_window_resize(
        GTK_WINDOW(data->gtkOffscreenWindow), width, height);
    gtk_widget_set_size_request(
        GTK_WIDGET(data->webKitWebView), width, height);
}

int main(int argc, char *argv[])
{
    WebViewData *data = calloc(1, sizeof(WebViewData));
    GLFWwindow *window = create_main_window();

    glfwSetWindowUserPointer(window, data);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    create_webview_texture(data);
    create_webview_shader(data);
    create_webview_quad_object(data);

    create_offscreen_window(data);
    create_webview(data);

    webkit_web_view_load_uri(data->webKitWebView, "https://madebyevan.com/webgl-water/");
    gtk_widget_show_all(GTK_WIDGET(data->gtkOffscreenWindow));

    glUseProgram(data->webViewShaderProgram);
    glViewport(0, 0, 800, 600);

    glBindTexture(GL_TEXTURE_2D, data->webViewTexture);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(data->webViewScreenQuad);
    glfwShowWindow(window);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        cairo_surface_t *surface = gtk_offscreen_window_get_surface(
            data->gtkOffscreenWindow);

        int surfaceWidth = cairo_image_surface_get_width(surface);
        int surfaceHeight = cairo_image_surface_get_height(surface);

        cairo_surface_flush(surface);
        uint8_t *buffer = cairo_image_surface_get_data(surface);

        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, surfaceWidth, surfaceHeight,
            GL_BGRA, GL_UNSIGNED_BYTE, buffer);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (gtk_events_pending())
            gtk_main_iteration();
    }

    gtk_widget_destroy(GTK_WIDGET(data->gtkOffscreenWindow));

    glDeleteProgram(data->webViewShaderProgram);
    glDeleteVertexArrays(1, &data->webViewScreenQuad);
    glDeleteTextures(1, &data->webViewTexture);

    glfwDestroyWindow(window);
    glfwTerminate();
    free(data);

    return EXIT_SUCCESS;
}
