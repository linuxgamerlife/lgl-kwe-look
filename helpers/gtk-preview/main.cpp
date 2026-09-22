// lgl-kwe-look-gtk-preview
//
// Renders a set of GTK3 widgets with a given theme, colour scheme and font into a
// PNG, using an offscreen window. It is a separate process so GTK never shares an
// address space with Qt. Long-lived: the Qt app starts it once and talks to it over
// a line protocol on stdin/stdout.
//
//   in:   R <id> <dark 0|1> <width> <height> <hex theme> <hex font> <hex output path>
//   out:  READY | FATAL <hex message>
//         OK <id> | ERR <id> <hex message>
//
// Strings are hex-encoded so spaces in font names need no quoting.
#include <gtk/gtk.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string fromHex(const std::string &hex)
{
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        out.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    return out;
}

std::string toHex(const std::string &s)
{
    static const char *digits = "0123456789abcdef";
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s) {
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 0x0f]);
    }
    return out;
}

gboolean quitLoop(gpointer data)
{
    g_main_loop_quit(static_cast<GMainLoop *>(data));
    return G_SOURCE_REMOVE;
}

// Gives the frame clock time to lay out and draw the offscreen window.
void settle(unsigned ms)
{
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    g_timeout_add(ms, quitLoop, loop);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

void styleClass(GtkWidget *w, const char *cls)
{
    gtk_style_context_add_class(gtk_widget_get_style_context(w), cls);
}

GtkWidget *row(std::initializer_list<GtkWidget *> widgets)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    for (GtkWidget *w : widgets)
        gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
    return box;
}

GtkWidget *buildContent()
{
    GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(col), 12);

    GtkWidget *suggested = gtk_button_new_with_label("Suggested");
    styleClass(suggested, GTK_STYLE_CLASS_SUGGESTED_ACTION);
    GtkWidget *destructive = gtk_button_new_with_label("Destructive");
    styleClass(destructive, GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
    GtkWidget *toggle = gtk_toggle_button_new_with_label("Toggle");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);
    gtk_box_pack_start(GTK_BOX(col),
                       row({gtk_button_new_with_label("Button"), suggested, destructive, toggle}),
                       FALSE, FALSE, 0);

    GtkWidget *check = gtk_check_button_new_with_label("Check");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
    GtkWidget *radioA = gtk_radio_button_new_with_label(nullptr, "Radio A");
    GtkWidget *radioB = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radioA), "Radio B");
    GtkWidget *sw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sw), TRUE);
    gtk_box_pack_start(GTK_BOX(col), row({check, gtk_check_button_new_with_label("Unchecked"), radioA, radioB, sw}),
                       FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "Text entry");
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Combo box");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Second item");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    GtkWidget *spin = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 42);
    gtk_box_pack_start(GTK_BOX(col), row({entry, combo, spin}), FALSE, FALSE, 0);

    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(scale), 60);
    gtk_widget_set_size_request(scale, 220, -1);
    GtkWidget *progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.4);
    gtk_widget_set_size_request(progress, 160, -1);
    gtk_box_pack_start(GTK_BOX(col), row({scale, progress}), FALSE, FALSE, 0);

    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *listbox = gtk_list_box_new();
    for (const char *t : {"List row one", "List row two", "List row three"})
        gtk_list_box_insert(GTK_LIST_BOX(listbox), gtk_label_new(t), -1);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), listbox, gtk_label_new("Tab one"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gtk_label_new("Second page"), gtk_label_new("Tab two"));
    gtk_box_pack_start(GTK_BOX(col), notebook, TRUE, TRUE, 0);

    return col;
}

bool render(const std::string &theme, bool dark, const std::string &font, int width, int height,
            const std::string &outPath, std::string *err)
{
    GtkSettings *settings = gtk_settings_get_default();
    // Theme, colour scheme and font must be set before the widgets exist.
    g_object_set(settings, "gtk-theme-name", theme.c_str(), "gtk-application-prefer-dark-theme",
                 dark ? TRUE : FALSE, "gtk-font-name", font.c_str(), nullptr);

    GtkWidget *win = gtk_offscreen_window_new();
    gtk_widget_set_size_request(win, width, height);
    gtk_container_add(GTK_CONTAINER(win), buildContent());
    gtk_widget_show_all(win);
    settle(120);

    GdkPixbuf *pixbuf = gtk_offscreen_window_get_pixbuf(GTK_OFFSCREEN_WINDOW(win));
    if (!pixbuf) {
        *err = "the offscreen window produced no image";
        gtk_widget_destroy(win);
        return false;
    }

    GError *gerr = nullptr;
    const gboolean saved = gdk_pixbuf_save(pixbuf, outPath.c_str(), "png", &gerr, nullptr);
    if (!saved) {
        *err = gerr ? gerr->message : "cannot save the preview";
        if (gerr)
            g_error_free(gerr);
    }
    g_object_unref(pixbuf);
    gtk_widget_destroy(win);
    return saved != FALSE;
}

}  // namespace

int main(int argc, char **argv)
{
    if (!gtk_init_check(&argc, &argv)) {
        std::cout << "FATAL " << toHex("cannot open a display (WAYLAND_DISPLAY / DISPLAY not set)")
                  << std::endl;
        return 1;
    }
    std::cout << "READY" << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd, id, dark, w, h, theme, font, out;
        in >> cmd >> id >> dark >> w >> h >> theme >> font >> out;
        if (cmd == "Q")
            break;
        if (cmd != "R") {
            std::cout << "ERR " << id << ' ' << toHex("bad request") << std::endl;
            continue;
        }

        std::string err;
        try {
            if (render(fromHex(theme), dark == "1", fromHex(font), std::stoi(w), std::stoi(h), fromHex(out), &err))
                std::cout << "OK " << id << std::endl;
            else
                std::cout << "ERR " << id << ' ' << toHex(err) << std::endl;
        } catch (const std::exception &e) {
            std::cout << "ERR " << id << ' ' << toHex(e.what()) << std::endl;
        }
    }
    return 0;
}
