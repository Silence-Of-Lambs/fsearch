#include <limits.h>
#include <linux/limits.h>

#include "fsearch_window_actions.h"
#include "clipboard.h"
#include "database_search.h"
#include "utils.h"
#include "config.h"

static void
action_set_active_bool (GActionGroup *group, const gchar *action_name, bool value)
{
    g_assert (G_IS_ACTION_GROUP (group));
    g_assert (G_IS_ACTION_MAP (group));

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (group), action_name);

    if (action) {
        g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (value));
    }
}

static void
action_set_enabled (GActionGroup *group, const gchar *action_name, bool value)
{
    g_assert (G_IS_ACTION_GROUP (group));
    g_assert (G_IS_ACTION_MAP (group));

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (group), action_name);

    if (action) {
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), value);
    }
}

static void
copy_file (GtkTreeModel *model,
           GtkTreePath  *path,
           GtkTreeIter  *iter,
           gpointer      userdata)
{
    DatabaseSearchEntry *entry = (DatabaseSearchEntry *)iter->user_data;
    GList **file_list = (GList **)userdata;
    if (entry) {
        GNode * node = db_search_entry_get_node (entry);
        char path[PATH_MAX] = "";
        bool res = db_node_get_path_full (node, path, sizeof (path));
        if (res) {
            *file_list = g_list_prepend (*file_list, g_strdup (path));
        }
    }
}

static void
fsearch_window_action_delete (GSimpleAction *action,
                              GVariant      *variant,
                              gpointer       user_data)
{
}

static void
fsearch_window_action_cut (GSimpleAction *action,
                           GVariant      *variant,
                           gpointer       user_data)
{
}

static void
fsearch_window_action_invert_selection (GSimpleAction *action,
                                        GVariant      *variant,
                                        gpointer       user_data)
{
    // TODO: can be very slow. Find a way how to optimize that.
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (!selection) {
        return;
    }
    GtkTreeModel *model = NULL;
    GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
    if (!selected_rows) {
        return;
    }
    if (!model) {
        return;
    }
    gtk_tree_selection_select_all (selection);

    GList *temp  = selected_rows;
    while (temp) {
        GtkTreePath *path = temp->data;
        GtkTreeIter iter = {0};
        if (gtk_tree_model_get_iter (model, &iter, path)) {
            gtk_tree_selection_unselect_iter (selection, &iter);
        }
        temp = temp->next;
    }
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
fsearch_window_action_deselect_all (GSimpleAction *action,
                                  GVariant      *variant,
                                  gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (selection) {
        gtk_tree_selection_unselect_all (selection);
    }
}

static void
fsearch_window_action_select_all (GSimpleAction *action,
                                  GVariant      *variant,
                                  gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (selection) {
        gtk_tree_selection_select_all (selection);
    }
}

static void
fsearch_window_action_copy (GSimpleAction *action,
                            GVariant      *variant,
                            gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (selection) {
        //guint selected_rows = gtk_tree_selection_count_selected_rows (selection);
        GList *file_list = NULL;
        gtk_tree_selection_selected_foreach (selection, copy_file, &file_list);
        file_list = g_list_reverse (file_list);
        clipboard_copy_file_list (file_list, 1);
    }
}

static void
open_cb (GtkTreeModel *model,
         GtkTreePath *path,
         GtkTreeIter *iter,
         gpointer data)
{
    DatabaseSearchEntry *entry = (DatabaseSearchEntry *)iter->user_data;
    if (entry) {
        GNode *node = db_search_entry_get_node (entry);
        if (node) {
            launch_node (node);
        }
    }
}

static void
fsearch_window_action_open (GSimpleAction *action,
                            GVariant      *variant,
                            gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (selection) {
        guint selected_rows = gtk_tree_selection_count_selected_rows (selection);
        if (selected_rows <= 10) {
            gtk_tree_selection_selected_foreach (selection, open_cb, NULL);
        }
    }
}

static void
open_folder_cb (GtkTreeModel *model,
         GtkTreePath *path,
         GtkTreeIter *iter,
         gpointer data)
{
    DatabaseSearchEntry *entry = (DatabaseSearchEntry *)iter->user_data;
    if (entry) {
        GNode *node = db_search_entry_get_node (entry);
        if (node) {
            launch_node_path (node);
        }
    }
}

static void
fsearch_window_action_open_folder (GSimpleAction *action,
                            GVariant      *variant,
                            gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    if (selection) {
        guint selected_rows = gtk_tree_selection_count_selected_rows (selection);
        if (selected_rows <= 10) {
            gtk_tree_selection_selected_foreach (selection, open_folder_cb, NULL);
        }
    }
}

static void
fsearch_window_action_focus_search (GSimpleAction *action,
                                    GVariant      *variant,
                                    gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    GtkWidget *entry = GTK_WIDGET (fsearch_application_window_get_search_entry (self));
    gtk_widget_grab_focus (entry);
}

static void
fsearch_window_action_hide_window (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    gtk_window_iconify (GTK_WINDOW (self));
}

static void
fsearch_window_action_show_filter (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    GtkWidget *filter = GTK_WIDGET (fsearch_application_window_get_filter_combobox (self));
    gtk_widget_set_visible (filter, g_variant_get_boolean (variant));
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    config->show_filter = g_variant_get_boolean (variant);
}

static void
fsearch_window_action_show_search_button (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    GtkWidget *button = GTK_WIDGET (fsearch_application_window_get_search_button (self));
    gtk_widget_set_visible (button, g_variant_get_boolean (variant));
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    config->show_search_button = g_variant_get_boolean (variant);
}

static void
fsearch_window_action_show_statusbar (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    GtkWidget *statusbar = GTK_WIDGET (fsearch_application_window_get_statusbar (self));
    gtk_widget_set_visible (statusbar, g_variant_get_boolean (variant));
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    config->show_statusbar = g_variant_get_boolean (variant);
}

static void
fsearch_window_action_show_menubar (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    GtkWidget *menubar = GTK_WIDGET (fsearch_application_window_get_menubar (self));
    gtk_widget_set_visible (menubar, g_variant_get_boolean (variant));
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    config->show_menubar = g_variant_get_boolean (variant);
}

static void
fsearch_window_action_search_in_path (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    bool search_in_path_old = config->search_in_path;
    config->search_in_path = g_variant_get_boolean (variant);
    GtkWidget *revealer = fsearch_application_window_get_search_in_path_revealer (self);
    if (config->search_in_path) {
        gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
    }
    else {
        gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
    }
    if (search_in_path_old != config->search_in_path) {
        g_idle_add (fsearch_application_window_update_search, self);
    }
}

static void
fsearch_window_action_search_mode (GSimpleAction *action,
                                   GVariant      *variant,
                                   gpointer       user_data)
{
    FsearchApplicationWindow *self = user_data;
    g_simple_action_set_state (action, variant);
    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    bool enable_regex_old = config->enable_regex;
    config->enable_regex = g_variant_get_boolean (variant);
    GtkWidget *revealer = fsearch_application_window_get_search_mode_revealer (self);
    if (config->enable_regex) {
        gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
    }
    else {
        gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
    }
    if (enable_regex_old != config->enable_regex) {
        g_idle_add (fsearch_application_window_update_search, self);
    }
}

static void
fsearch_window_action_update_database (GSimpleAction *action,
                                       GVariant      *variant,
                                       gpointer       user_data)
{
}

static void
action_toggle_state_cb (GSimpleAction *saction,
                        GVariant *parameter,
                        gpointer user_data)
{
    GAction *action = G_ACTION (saction);

    GVariant *state = g_action_get_state (action);
    g_action_change_state (action, g_variant_new_boolean (!g_variant_get_boolean (state)));
    g_variant_unref (state);
}

static GActionEntry FsearchWindowActions[] = {
    { "open",     fsearch_window_action_open },
    { "open_folder",     fsearch_window_action_open_folder },
    { "copy_clipboard",     fsearch_window_action_copy },
    { "cut_clipboard",     fsearch_window_action_cut },
    { "delete_selection",     fsearch_window_action_delete },
    { "select_all",     fsearch_window_action_select_all },
    { "deselect_all",     fsearch_window_action_deselect_all },
    { "invert_selection",     fsearch_window_action_invert_selection },
    { "focus_search",     fsearch_window_action_focus_search },
    { "hide_window",     fsearch_window_action_hide_window },
    //{ "update_database",     fsearch_window_action_update_database },
    // View
    { "show_statusbar", action_toggle_state_cb, NULL, "true", fsearch_window_action_show_statusbar },
    { "show_menubar", action_toggle_state_cb, NULL, "true", fsearch_window_action_show_menubar },
    { "show_filter", action_toggle_state_cb, NULL, "true", fsearch_window_action_show_filter },
    { "show_search_button", action_toggle_state_cb, NULL, "true", fsearch_window_action_show_search_button },
    // Search
    { "search_in_path", action_toggle_state_cb, NULL, "true", fsearch_window_action_search_in_path },
    { "search_mode", action_toggle_state_cb, NULL, "true", fsearch_window_action_search_mode },
};

void
fsearch_window_actions_update   (FsearchApplicationWindow *self)
{
    GtkTreeSelection *selection = fsearch_application_window_get_listview_selection (self);
    GtkTreeView *treeview = gtk_tree_selection_get_tree_view (selection);

    gint num_rows = 0;
    if (treeview) {
        GtkTreeModel *model = gtk_tree_view_get_model (treeview);
        if (model) {
            num_rows = gtk_tree_model_iter_n_children (model, NULL);
        }
    }

    GActionGroup *group = gtk_widget_get_action_group (GTK_WIDGET (self), "win");
    g_assert (G_IS_SIMPLE_ACTION_GROUP (group));

    gint num_rows_selected = gtk_tree_selection_count_selected_rows (selection);
    action_set_enabled (group, "select_all", num_rows);
    action_set_enabled (group, "deselect_all", num_rows_selected);
    action_set_enabled (group, "invert_selection", num_rows_selected);
    action_set_enabled (group, "copy_clipboard", num_rows_selected);
    action_set_enabled (group, "cut_clipboard", num_rows_selected);
    action_set_enabled (group, "delete_selection", num_rows_selected);
    action_set_enabled (group, "open", num_rows_selected);
    action_set_enabled (group, "open_folder", num_rows_selected);
    action_set_enabled (group, "focus_search", TRUE);
    action_set_enabled (group, "hide_window", TRUE);
    action_set_enabled (group, "update_database", TRUE);
    action_set_enabled (group, "show_menubar", TRUE);
    action_set_enabled (group, "show_statusbar", TRUE);
    action_set_enabled (group, "show_filter", TRUE);
    action_set_enabled (group, "show_search_button", TRUE);

    FsearchConfig *config = fsearch_application_get_config (FSEARCH_APPLICATION_DEFAULT);
    action_set_active_bool (group, "show_menubar", config->show_menubar);
    action_set_active_bool (group, "show_statusbar", config->show_statusbar);
    action_set_active_bool (group, "show_filter", config->show_filter);
    action_set_active_bool (group, "show_search_button", config->show_search_button);
    action_set_active_bool (group, "search_in_path", config->search_in_path);
    action_set_active_bool (group, "search_mode", config->enable_regex);
}

void
fsearch_window_actions_init   (FsearchApplicationWindow *self)
{
    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     FsearchWindowActions,
                                     G_N_ELEMENTS (FsearchWindowActions),
                                     self);

    fsearch_window_actions_update (self);
}