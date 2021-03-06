/* Paned Widgets
 *
 * The GtkPaned Widget divides its content area into two panes
 * with a divider in between that the user can adjust. A separate
 * child is placed into each pane. GtkPaned widgets can be split
 * horizontally or vertially.
 *
 * There are a number of options that can be set for each pane.
 * This test contains both a horizontal and a vertical GtkPaned
 * widget, and allows you to adjust the options for each side of
 * each widget.
 */

#include <gtk/gtk.h>

static void
toggle_resize (GtkWidget *widget,
               GtkWidget *child)
{
  GtkWidget *parent;
  GtkPaned *paned;

  parent = gtk_widget_get_parent (child);
  paned = GTK_PANED (parent);

  if (child == gtk_paned_get_start_child (paned))
    gtk_paned_set_resize_start_child (paned, !gtk_paned_get_resize_start_child (paned));
  else
    gtk_paned_set_resize_end_child (paned, !gtk_paned_get_resize_end_child (paned));
}

static void
toggle_shrink (GtkWidget *widget,
               GtkWidget *child)
{
  GtkWidget *parent;
  GtkPaned *paned;

  parent = gtk_widget_get_parent (child);
  paned = GTK_PANED (parent);

  if (child == gtk_paned_get_start_child (paned))
    gtk_paned_set_shrink_start_child (paned, !gtk_paned_get_shrink_start_child (paned));
  else
    gtk_paned_set_shrink_end_child (paned, !gtk_paned_get_shrink_end_child (paned));
}

static GtkWidget *
create_pane_options (GtkPaned    *paned,
                     const char *frame_label,
                     const char *label1,
                     const char *label2)
{
  GtkWidget *child1, *child2;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *check_button;

  child1 = gtk_paned_get_start_child (paned);
  child2 = gtk_paned_get_end_child (paned);

  frame = gtk_frame_new (frame_label);
  gtk_widget_set_margin_start (frame, 4);
  gtk_widget_set_margin_end (frame, 4);
  gtk_widget_set_margin_top (frame, 4);
  gtk_widget_set_margin_bottom (frame, 4);

  table = gtk_grid_new ();
  gtk_frame_set_child (GTK_FRAME (frame), table);

  label = gtk_label_new (label1);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

  check_button = gtk_check_button_new_with_mnemonic ("_Resize");
  gtk_grid_attach (GTK_GRID (table), check_button, 0, 1, 1, 1);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_resize), child1);

  check_button = gtk_check_button_new_with_mnemonic ("_Shrink");
  gtk_grid_attach (GTK_GRID (table), check_button, 0, 2, 1, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_shrink), child1);

  label = gtk_label_new (label2);
  gtk_grid_attach (GTK_GRID (table), label, 1, 0, 1, 1);

  check_button = gtk_check_button_new_with_mnemonic ("_Resize");
  gtk_grid_attach (GTK_GRID (table), check_button, 1, 1, 1, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_resize), child2);

  check_button = gtk_check_button_new_with_mnemonic ("_Shrink");
  gtk_grid_attach (GTK_GRID (table), check_button, 1, 2, 1, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_shrink), child2);

  return frame;
}

GtkWidget *
do_panes (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *frame;
  GtkWidget *hpaned;
  GtkWidget *vpaned;
  GtkWidget *button;
  GtkWidget *vbox;

  if (!window)
    {
      window = gtk_window_new ();
      gtk_window_set_display (GTK_WINDOW (window),
                              gtk_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      gtk_window_set_title (GTK_WINDOW (window), "Paned Widgets");

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_window_set_child (GTK_WINDOW (window), vbox);

      vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
      gtk_widget_set_margin_start (vpaned, 5);
      gtk_widget_set_margin_end (vpaned, 5);
      gtk_widget_set_margin_top (vpaned, 5);
      gtk_widget_set_margin_bottom (vpaned, 5);
      gtk_box_append (GTK_BOX (vbox), vpaned);

      hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_paned_set_start_child (GTK_PANED (vpaned), hpaned);

      frame = gtk_frame_new (NULL);
      gtk_widget_set_size_request (frame, 60, 60);
      gtk_paned_set_start_child (GTK_PANED (hpaned), frame);

      button = gtk_button_new_with_mnemonic ("_Hi there");
      gtk_frame_set_child (GTK_FRAME (frame), button);

      frame = gtk_frame_new (NULL);
      gtk_widget_set_size_request (frame, 80, 60);
      gtk_paned_set_end_child (GTK_PANED (hpaned), frame);

      frame = gtk_frame_new (NULL);
      gtk_widget_set_size_request (frame, 60, 80);
      gtk_paned_set_end_child (GTK_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      gtk_box_append (GTK_BOX (vbox),
                          create_pane_options (GTK_PANED (hpaned),
                                               "Horizontal",
                                               "Left",
                                               "Right"));

      gtk_box_append (GTK_BOX (vbox),
                          create_pane_options (GTK_PANED (vpaned),
                                               "Vertical",
                                               "Top",
                                               "Bottom"));
    }

  if (!gtk_widget_get_visible (window))
    gtk_widget_show (window);
  else
    gtk_window_destroy (GTK_WINDOW (window));

  return window;
}
