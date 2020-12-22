#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

GtkWidget *window;
GtkWidget *container;
GtkWidget *label;
GtkWidget *entry;

typedef struct
{
  gint pos_x;
  gint pos_y;
  gint width;
  gint height;
  const gchar *command;
} WindowState;

WindowState window_state;

int working = 0;

void save_window_state()
{
  GKeyFile *keyfile = g_key_file_new();

  g_key_file_set_integer(keyfile, "WindowState", "PositionX", window_state.pos_x);
  g_key_file_set_integer(keyfile, "WindowState", "PositionY", window_state.pos_y);
  g_key_file_set_integer(keyfile, "WindowState", "Width", window_state.width);
  g_key_file_set_integer(keyfile, "WindowState", "Height", window_state.height);
  g_key_file_set_string(keyfile, "WindowState", "Command", window_state.command);

  const char *appid = g_application_get_application_id(g_application_get_default());
  char *path = g_build_filename(g_get_user_cache_dir(), appid, NULL);

  if (g_mkdir_with_parents(path, 0700) < 0)
  {
    g_key_file_unref(keyfile);
    g_free(path);
  }
  else
  {
    char *file = g_build_filename(path, "state.ini", NULL);
    g_key_file_save_to_file(keyfile, file, NULL);
    g_free(file);
    g_key_file_unref(keyfile);
    g_free(path);
  }
}

gboolean window_store_state(gpointer data)
{
  gtk_window_get_position(GTK_WINDOW(window), &window_state.pos_x, &window_state.pos_y);
  gtk_window_get_size(GTK_WINDOW(window), &window_state.width, &window_state.height);
  window_state.command = gtk_entry_get_text(GTK_ENTRY(entry));

  return FALSE;
}

gboolean window_load_state(gpointer data)
{
  const char *appid = g_application_get_application_id(g_application_get_default());
  char *file = g_build_filename(g_get_user_cache_dir(), appid, "state.ini", NULL);
  GKeyFile *keyfile = g_key_file_new();

  GError *error;
  error = NULL;

  if (!g_key_file_load_from_file(keyfile, file, G_KEY_FILE_NONE, &error))
  {
    goto out;
  }

  error = NULL;
  window_state.pos_x = g_key_file_get_integer(keyfile, "WindowState", "PositionX", &error);
  if (error != NULL)
  {
    window_state.pos_x = 90;
  }
  error = NULL;
  window_state.pos_y = g_key_file_get_integer(keyfile, "WindowState", "PositionY", &error);
  if (error != NULL)
  {
    window_state.pos_x = 90;
  }
  error = NULL;
  window_state.width = g_key_file_get_integer(keyfile, "WindowState", "Width", &error);
  if (error != NULL)
  {
    window_state.width = 200;
  }
  error = NULL;
  window_state.height = g_key_file_get_integer(keyfile, "WindowState", "Height", &error);
  if (error != NULL)
  {
    window_state.height = 120;
  }
  error = NULL;
  window_state.command = g_key_file_get_string(keyfile, "WindowState", "Command", &error);
  if (error != NULL)
  {
    window_state.command = "cat $file$";
  }

  gtk_window_move(GTK_WINDOW(window), window_state.pos_x, window_state.pos_y);
  gtk_window_resize(GTK_WINDOW(window), window_state.width, window_state.height);
  gtk_entry_set_text(GTK_ENTRY(entry), window_state.command);

out:
  g_key_file_unref(keyfile);
  g_free(file);

  return FALSE;
}

void configure_callback(GtkWindow *window, GdkEvent *event, gpointer data)
{
  gdk_threads_add_idle(window_store_state, NULL);
}

void entry_changed(GtkWidget *widget, gpointer data)
{
  gdk_threads_add_idle(window_store_state, NULL);
}

void window_destroy(GtkWidget *widget, gpointer data)
{
  save_window_state();
}

#define UI_DRAG_TARGETS_COUNT 3

enum
{
  DT_TEXT,
  DT_URI,
  DT_URI_LIST
};

GtkTargetEntry ui_drag_targets[UI_DRAG_TARGETS_COUNT] = {
  {(gchar *)"text/plain", 0, DT_TEXT},
  {(gchar *)"text/uri", 0, DT_URI},
  {(gchar *)"text/uri-list", 0, DT_URI_LIST}};

gboolean on_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time,
    gpointer user_data)
{
  gdk_drag_status(context, GDK_ACTION_LINK, time);
  return TRUE;
}

gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time,
    gpointer data)
{
  GList *targets;
  targets = gdk_drag_context_list_targets(context);
  if (targets == NULL)
  {
    return FALSE;
  }
  return TRUE;
}

size_t chopN(char *str, size_t n)
{
  g_assert(n != 0 && str != 0);
  size_t len = strlen(str);
  if (n > len)
    n = len;
  memmove(str, str + n, len - n + 1);
  return (len - n);
}

void updateLabel()
{
  char workingStr[5];
  sprintf(workingStr, "%d", working);
  gchar *display = g_strconcat("Drop file(s) here.\nWorking threads: ", workingStr, (char *)NULL);
  gtk_label_set_label(GTK_LABEL(label), display);
  g_free(display);
}

gboolean increment_working(gpointer data)
{
  working++;
  updateLabel();
  return FALSE;
}

gboolean decrement_working(gpointer data)
{
  working--;
  updateLabel();
  return FALSE;
}

typedef struct
{
  const gchar *command;
  gchar **files;
} WorkerData;

gpointer worker(gpointer data)
{
  gdk_threads_add_idle(increment_working, NULL);
  WorkerData *workerData = (WorkerData *)data;
  g_print("Command: '%s'\n", workerData->command);

  for (int i = 0; workerData->files[i] != NULL; i++)
  {
    gchar *file = workerData->files[i];
    g_print("Received: '%s'\n", file);
    if (g_str_has_prefix(file, "file://"))
    {
      chopN(file, 7);
    }

    const gchar *execute = g_strjoinv(file, g_strsplit(workerData->command, "$file$", -1));
    g_print("Executing: '%s'\n", execute);
    int status = system(execute);
    g_print("Executed: '%s' with status '%i'\n", execute, status);
  }

  g_strfreev(workerData->files);
  g_free(workerData);
  gdk_threads_add_idle(decrement_working, NULL);
  return NULL;
}

void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, int x, int y,
    GtkSelectionData *data, guint info, guint time)
{
  guchar *text = NULL;
  gchar **files = NULL;

  gtk_drag_finish(context, TRUE, FALSE, time);

  switch (info)
  {
    case DT_URI_LIST:
      files = gtk_selection_data_get_uris(data);
      break;

    case DT_TEXT:
      text = gtk_selection_data_get_text(data);
      g_strchomp((gchar *)text);
      files = g_strsplit((const gchar *)text, "\n", -1);
      break;

    default:
      g_print("Warning: not handled drag and drop target %u.", info);
      break;
  }

  if (text != NULL)
  {
    g_free(text);
  }
  if (files != NULL)
  {
    const gchar *command = gtk_entry_get_text(GTK_ENTRY(entry));

    WorkerData *wd;
    GThread *thread;

    wd = (WorkerData *)g_malloc(sizeof *wd);
    wd->command = command;
    wd->files = files;

    thread = g_thread_new("worker", worker, wd);
    g_thread_unref(thread);
  }
}

gboolean entry_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_tip,
    GtkTooltip *tooltip, gpointer user_data)
{
  gtk_tooltip_set_text(tooltip, "Use $file$ to replace with the dropped file path.");
  return TRUE;
}

void activate(GtkApplication *app, gpointer user_data)
{
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "DropRun");
  gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(window), 200, 120);
  gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);

  g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(configure_callback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(window_destroy), NULL);

  container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add(GTK_CONTAINER(window), container);
  gtk_container_set_border_width(GTK_CONTAINER(container), 8);

  label = gtk_label_new("Drop file(s) here.\nWorking threads: ");
  gtk_box_pack_start(GTK_BOX(container), label, TRUE, TRUE, 4);

  working = 0;
  updateLabel();

  gtk_drag_dest_set(label, GTK_DEST_DEFAULT_ALL, ui_drag_targets, UI_DRAG_TARGETS_COUNT, GDK_ACTION_LINK);
  g_signal_connect(label, "drag-data-received", G_CALLBACK(on_drag_data_received), NULL);
  g_signal_connect(label, "drag-drop", G_CALLBACK(on_drag_drop), NULL);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), "cat $file$");
  gtk_box_pack_end(GTK_BOX(container), entry, FALSE, FALSE, 0);

  g_object_set(GTK_ENTRY(entry), "has-tooltip", TRUE, NULL);
  g_signal_connect(GTK_ENTRY(entry), "query-tooltip", G_CALLBACK(entry_tooltip), NULL);
  g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_changed), NULL);

  window_load_state(GTK_WINDOW(window));

  gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
  if (argc > 1)
  {
    if (g_str_equal(argv[1], "--version") || g_str_equal(argv[1], "-v"))
    {
      g_print("DropRun version 0.1.0\n");
      g_print("Copyright (C) 2020 Everton Murilo Vieira.\n");
      g_print("        Contact in tonvieira@gmail.com\n");
      return 0;
    }
  }

  GtkApplication *app;
  int status;

  app = gtk_application_new("br.com.pointel.DropRun", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
