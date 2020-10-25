#include <gtk/gtk.h>
#include <sys/wait.h>
#include "dbus_client.h"
#include "general_helper.h"

typedef struct {
    GtkWidget *set_button;
    GtkWidget *restore_button;
    gchar *image_file;
} Data;

static void successful_dialog(const gchar *message)
{
    GtkWidget *info = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
	GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "GDM background successfully %s", message);
    gtk_dialog_run(GTK_DIALOG(info));
    gtk_widget_destroy(info);
}

static void
restart_gdm_dialog (const gchar *action_id)
{
    GtkWidget *question = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
	GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Do you want to restart GDM to apply change?");
    int reply = gtk_dialog_run(GTK_DIALOG(question));
    gtk_widget_destroy(question);
    if (reply == GTK_RESPONSE_YES) {
	if (restart_gdm_service(action_id) == -1) {
	    g_printerr("Could not restart gdm service\n");
	}
    }
}

static void
error_message_dialog (GError *error)
{
	GtkWidget *error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
	    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", error->message);
	gtk_dialog_run(GTK_DIALOG(error_dialog));
	gtk_widget_destroy(error_dialog);
	g_error_free(error);
}

static void
on_button_set_clicked (GtkWidget *widget, Data *data_struct)
{
    // Get the default theme and make a backup if there isn't one.
    char *default_theme = "/usr/share/gnome-shell/theme/Yaru/gnome-shell-theme.gresource";
    char *backup_theme = vector_strcat(default_theme, "~", NULL);
    if (access(backup_theme, F_OK) == -1) {
	if (set_background_image("backup") == -1) {
	    fprintf(stderr, "Could not create backup file\n");
	    exit(1);
	}
	gtk_widget_set_sensitive(data_struct->restore_button, TRUE);
    }

    // Get the list of resource inside the original gresource file.
    char *list_command_line = vector_strcat("gresource list ", backup_theme, NULL);
    FILE *piped_list = popen(list_command_line, "r");
    if (!piped_list) {
	perror("Could not get gresource list command output");
	exit(1);
    }

    // Go through the resource list line by line.
    char *list_line = NULL;
    size_t length = 0;
    while (getline(&list_line, &length, piped_list) != -1) {

	// Create the gresource directory structure.
	char *work_dir = malloc(length + 1);
	    if (!work_dir) {
		perror("Could not allocate memory");
		exit(1);
	}
	strcpy(work_dir, list_line);
	remove_substring(work_dir, "/org/gnome/shell/");
	remove_trailing_chars(work_dir, '/');
	work_dir = vector_strcat("/tmp/gdm3/", work_dir, NULL);
	recursive_mkdir(work_dir, 0755);
	free(work_dir);

	// Extract the resources from the original gresource.
    	remove_trailing_chars (list_line, '\n');
	char *resource_path = malloc(strlen(list_line) + 1);
	if (!resource_path) {
		perror("Could not allocate memory");
		exit(1);
	}
	strcpy(resource_path, list_line);
	char *extract_command_line[] = {"gresource", "extract", backup_theme, resource_path, NULL};
	char *resource_file = vector_strcat("/tmp/gdm3/", remove_substring(list_line,
	    "/org/gnome/shell/"), NULL);
	if (redirect_output(extract_command_line, resource_file) == -1) {
	    perror("Error extracting resource file");
	    exit(1);
	}
	free(resource_path);
	free(resource_file);
    }

    // Copy the selected image file to the work directory.
    char *img_name = remove_leading_chars(data_struct->image_file, '/');
    char *img_path =  vector_strcat("/tmp/gdm3/theme/", img_name, NULL);
    if (copy_file(data_struct->image_file, img_path) == -1) {
	perror("Could not move image file to work directory");
	exit(1);
    }
    free(img_path);

    // Put the gdm style file in a buffer for editing.
    char *work_dir = "/tmp/gdm3/theme/";
    char *gdm_path = vector_strcat(work_dir, "gdm3.css", NULL);
    char *buffer = vector_strcat(work_dir, "buffer", NULL);
    FILE *gdm_css = fopen(gdm_path, "r");
    if (!gdm_css) {
	perror("Could not open gdm.css file");
	exit(1);
    }
    FILE *tmp_file = fopen(buffer, "w");
    if (!buffer) {
	perror("Could not open gdm.css buffer");
	exit(1);
    }

    // Set the gdm background to the selected image file.
    char *old_img = "#lockDialogGroup {\n";
    char *new_img =
	vector_strcat("  background: url('resource:///org/gnome/shell/theme/" , img_name,
	    "');\n  background-size: cover; }\n", NULL);
    int skip = 0;
    char line[256];
    while (fgets(line, sizeof(line), gdm_css)) {
	if (strcmp(line, old_img) == 0) {
	    fputs(line, tmp_file);
	    fputs(new_img, tmp_file);
	    skip = 1;
	}
	else {
	    if (skip == 0)
		fputs(line, tmp_file);
	    else {
		skip = 0;
	    }
	}
    }
    fclose(gdm_css);
    fclose(tmp_file);
    remove(gdm_path);
    if (rename(buffer, gdm_path) == -1) {
	perror("Could not edit gdm3.css");
	exit(1);
    }
    free(gdm_path);
    free(buffer);
    free(new_img);

    // Create the gresource xml file.
    char *gresource_xml_file = vector_strcat(work_dir, "gnome-shell-theme.gresource", ".xml", NULL);
    FILE *xml_file = fopen(gresource_xml_file, "w");
    if (!xml_file) {
	perror("Could not open xml file for writing");
	exit(1);
    }
    char *header = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<gresources>\n\
    <gresource prefix=\"/org/gnome/shell/theme\">\n";
    fputs(header, xml_file);
    piped_list = popen(list_command_line, "r");
    free(backup_theme);
    list_line = NULL;
    length = 0;
    while (getline(&list_line, &length, piped_list) != -1) {
	remove_trailing_chars (list_line, '\n');
	char *resource_file = remove_substring(list_line, "/org/gnome/shell/theme/");
	char *resource_line = vector_strcat("	<file>", resource_file, "</file>\n", NULL);
	fputs(resource_line, xml_file);
        free(resource_line);
    }
    char *img_line = vector_strcat("	<file>", img_name, "</file>\n", NULL);
    fputs(img_line, xml_file);
    g_free(data_struct->image_file);
    char *footer = "\
    </gresource>\n\
</gresources>";
    fputs(footer, xml_file);
    free(list_line);
    free(list_command_line);
    pclose(piped_list);
    pclose(xml_file);

    // Compile the edited gresource file.
    char *glib_compile = "glib-compile-resources";
    char *source_dir = vector_strcat("--sourcedir=", work_dir, NULL);
    pid_t child_pid = fork();
    if (child_pid == -1) {
	perror ("Could not fork main process");
	exit(1);
    } else if (!child_pid) {
	if (execlp(glib_compile, glib_compile, source_dir, gresource_xml_file, NULL) == -1) {
	    perror("Could not compile gresource file");
	    exit(1);
	}
    }
    waitpid(child_pid, NULL, 0);
    free(gresource_xml_file);
    free(source_dir);

    // Move the edited gresource file to the default theme directory.
    char *compiled_gresource = vector_strcat(work_dir, "gnome-shell-theme.gresource", NULL);
    int result = set_background_image("set");
    if (result == -1) {
	perror("Could not set gresource file");
	exit(1);
    } else if (result == 1) {
	fprintf(stderr, "Operation canceled\n");
	gtk_widget_set_sensitive(widget, FALSE);
    } else {
	successful_dialog("set");
	restart_gdm_dialog("xyz.thiggy01.GDMBackground.SetImage");
	gtk_widget_set_sensitive(widget, FALSE);
    }
    free(compiled_gresource);

    // Remove temporary directories and files.
    if (recursive_rmdir("/tmp/gdm3") == -1) {
	fprintf(stderr, "Could not remove temporary data\n");
    }
}

static void
on_button_restore_clicked (GtkWidget *widget, Data *data_struct)
{
    int result = restore_backup_theme();
    if (result == -1) {
	fprintf(stderr, "Error restoring backup file");
	exit(1);
    } else if (result == 1) {
	fprintf(stderr, "Operation canceled\n");
	gtk_widget_set_sensitive(widget, FALSE);
    } else {
	successful_dialog("restored");
	restart_gdm_dialog("xyz.thiggy01.GDMBackground.RestoreBackup");
	gtk_widget_set_sensitive(widget, FALSE);
    }
}

static void
on_drag_data_received (GtkWidget *widget, GdkDragContext *context,
		       gint x, gint y, GtkSelectionData *data,
		       guint info, guint time, Data *data_struct)
{
    GString *g_string = g_string_new((gchar *)gtk_selection_data_get_data(data));
    g_string_erase(g_string, 0, 7);
    g_string->str[strcspn(g_string->str, "\r\n")] = '\0';
    g_string->str = replace_word(g_string->str, "%20", " ");
    data_struct->image_file = g_string->str;
    g_print("%s\n", data_struct->image_file);

    /* gtk_image_set_from_file(GTK_IMAGE(widget), data_struct->image_file); */
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(g_string->str, &error);
    if (!pixbuf) {
	error_message_dialog(error);
    }
    g_string_free(g_string, FALSE);
    guint default_width = gtk_widget_get_allocated_width(widget);
    guint default_height = gtk_widget_get_allocated_height(widget);
    pixbuf = gdk_pixbuf_scale_simple(pixbuf, default_width, default_height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(widget), pixbuf);

    gtk_drag_finish(context, TRUE, FALSE, time);
    gtk_widget_set_sensitive(data_struct->set_button, TRUE);
}

int
main(int argc,gchar *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 560, 315);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *image = gtk_image_new();
    GtkTargetEntry targets[] = {{"text/uri-list", 0, 1}};
    gtk_drag_dest_set(image, GTK_DEST_DEFAULT_ALL, targets, 1,
		      GDK_ACTION_COPY);
    gtk_container_add(GTK_CONTAINER(window), image);

    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Drag and Drop an Image File");
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    Data *data = g_slice_new(Data);

    data->set_button = gtk_button_new_with_label("Set");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), data->set_button);
    gtk_widget_set_sensitive(data->set_button, FALSE);

    data->restore_button = gtk_button_new_with_label("Restore");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), data->restore_button);
    if (!access(
	"/usr/share/gnome-shell/theme/Yaru/gnome-shell-theme.gresource~", F_OK))
	gtk_widget_set_sensitive(data->restore_button, TRUE);
    else
	gtk_widget_set_sensitive(data->restore_button, FALSE);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(data->set_button, "clicked", G_CALLBACK(on_button_set_clicked), data);
    g_signal_connect(data->restore_button, "clicked", G_CALLBACK(on_button_restore_clicked), data);
    g_signal_connect(image, "drag-data-received", G_CALLBACK(on_drag_data_received), data);

    gtk_widget_show_all(window);
    gtk_main();

    g_slice_free(Data, data);

    return 0;
}
