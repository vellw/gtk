/* GTK+ - accessibility implementations
 * Copyright 2011, F123 Consulting & Mais Diferenças
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkwindowaccessibleprivate.h"

#include "gtkaccessibility.h"
#include "gtktoplevelaccessible.h"
#include "gtkwidgetaccessibleprivate.h"

#include "gtklabel.h"
#include "gtkwidgetprivate.h"
#include "gtkwindowprivate.h"
#include "gtknative.h"

/* atkcomponent.h */

static void                  gtk_window_accessible_get_extents      (AtkComponent         *component,
                                                           int                  *x,
                                                           int                  *y,
                                                           int                  *width,
                                                           int                  *height,
                                                           AtkCoordType         coord_type);
static void                  gtk_window_accessible_get_size         (AtkComponent         *component,
                                                           int                  *width,
                                                           int                  *height);

static void atk_component_interface_init (AtkComponentIface *iface);
static void atk_window_interface_init (AtkWindowIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkWindowAccessible,
                         gtk_window_accessible,
                         GTK_TYPE_WIDGET_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                atk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_WINDOW,
                                                atk_window_interface_init));


static void
gtk_window_accessible_focus_event (AtkObject *obj,
                                   gboolean   focus_in)
{
  atk_object_notify_state_change (obj, ATK_STATE_ACTIVE, focus_in);
}

static void
on_title_changed (GObject             *gobject,
                  GParamSpec          *pspec,
                  GtkWindowAccessible *self)
{
  AtkObject *atk_obj = ATK_OBJECT (self);

  g_object_notify (G_OBJECT (atk_obj), "accessible-name");
  g_signal_emit_by_name (atk_obj, "visible-data-changed");
}

static const char *
gtk_window_accessible_get_name (AtkObject *accessible)
{
  const char * name;
  GtkWidget* widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (gtk_window_accessible_parent_class)->get_name (accessible);
  if (name != NULL)
    return name;

  if (GTK_IS_WINDOW (widget))
    {
      GtkWindow *window = GTK_WINDOW (widget);

      name = gtk_window_get_title (window);
    }
  return name;
}

static int
gtk_window_accessible_get_index_in_parent (AtkObject *accessible)
{
  GtkWidget* widget;
  AtkObject* atk_obj;
  int index = -1;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return -1;

  index = ATK_OBJECT_CLASS (gtk_window_accessible_parent_class)->get_index_in_parent (accessible);
  if (index != -1)
    return index;

  atk_obj = atk_get_root ();

  if (GTK_IS_WINDOW (widget))
    {
      GtkWindow *window = GTK_WINDOW (widget);
      if (GTK_IS_TOPLEVEL_ACCESSIBLE (atk_obj))
        {
          GtkToplevelAccessible *toplevel = GTK_TOPLEVEL_ACCESSIBLE (atk_obj);
          index = g_list_index (gtk_toplevel_accessible_get_children (toplevel), window);
        }
      else
        {
          int i, sibling_count;

          sibling_count = atk_object_get_n_accessible_children (atk_obj);
          for (i = 0; i < sibling_count && index == -1; ++i)
            {
              AtkObject *child = atk_object_ref_accessible_child (atk_obj, i);
              if (accessible == child)
                index = i;
              g_object_unref (G_OBJECT (child));
            }
        }
    }
  return index;
}

static AtkRelationSet *
gtk_window_accessible_ref_relation_set (AtkObject *obj)
{
  GtkWidget *widget;
  AtkRelationSet *relation_set;
  AtkObject *array[1];
  AtkRelation* relation;
  GtkWidget *current_widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (gtk_window_accessible_parent_class)->ref_relation_set (obj);

  if (atk_object_get_role (obj) == ATK_ROLE_TOOL_TIP)
    {
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_POPUP_FOR);
      if (relation)
        atk_relation_set_remove (relation_set, relation);

      if (0) /* FIXME need a way to go from tooltip window to widget */
        {
          array[0] = gtk_widget_get_accessible (current_widget);
          relation = atk_relation_new (array, 1, ATK_RELATION_POPUP_FOR);
          atk_relation_set_add (relation_set, relation);
          g_object_unref (relation);
        }
    }
  return relation_set;
}

static AtkStateSet *
gtk_window_accessible_ref_state_set (AtkObject *accessible)
{
  AtkStateSet *state_set;
  GtkWidget *widget;
  GtkWindow *window;
  GdkSurface *gdk_surface;
  GdkSurfaceState state;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (gtk_window_accessible_parent_class)->ref_state_set (accessible);

  window = GTK_WINDOW (widget);

  if (gtk_window_is_active (window))
    atk_state_set_add_state (state_set, ATK_STATE_ACTIVE);

  gdk_surface = gtk_native_get_surface (GTK_NATIVE (window));
  if (GDK_IS_TOPLEVEL (gdk_surface))
    {
      state = gdk_toplevel_get_state (GDK_TOPLEVEL (gdk_surface));
      if (state & GDK_SURFACE_STATE_MINIMIZED)
        atk_state_set_add_state (state_set, ATK_STATE_ICONIFIED);
    }
  if (gtk_window_get_modal (window))
    atk_state_set_add_state (state_set, ATK_STATE_MODAL);

  if (gtk_window_get_resizable (window))
    atk_state_set_add_state (state_set, ATK_STATE_RESIZABLE);

  return state_set;
}

static int
gtk_window_accessible_get_n_children (AtkObject *object)
{
  GtkWidget *window;
  GtkWidget *child;
  int count = 0;

  window = gtk_accessible_get_widget (GTK_ACCESSIBLE (object));
  for (child = gtk_widget_get_first_child (GTK_WIDGET (window));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    count++;

  return count;
}

static AtkObject *
gtk_window_accessible_ref_child (AtkObject *object,
                                 int        i)
{
  GtkWidget *window, *child;
  int pos;

  window = gtk_accessible_get_widget (GTK_ACCESSIBLE (object));
  for (child = gtk_widget_get_first_child (GTK_WIDGET (window)), pos = 0;
       child != NULL;
       child = gtk_widget_get_next_sibling (child), pos++)
    {
      if (pos == i)
        return g_object_ref (gtk_widget_get_accessible (child));
    }

  return NULL;
}

static void
gtk_window_accessible_initialize (AtkObject *object,
                                  gpointer   data)
{
  GtkWidget *window = data;

  g_signal_connect (window, "notify::title", G_CALLBACK (on_title_changed), object);
}

static void
gtk_window_accessible_class_init (GtkWindowAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->initialize = gtk_window_accessible_initialize;
  class->get_name = gtk_window_accessible_get_name;
  class->get_index_in_parent = gtk_window_accessible_get_index_in_parent;
  class->ref_relation_set = gtk_window_accessible_ref_relation_set;
  class->ref_state_set = gtk_window_accessible_ref_state_set;
  class->focus_event = gtk_window_accessible_focus_event;
  class->get_n_children = gtk_window_accessible_get_n_children;
  class->ref_child = gtk_window_accessible_ref_child;
}

static void
gtk_window_accessible_init (GtkWindowAccessible *accessible)
{
}

static void
gtk_window_accessible_get_extents (AtkComponent  *component,
                                   int           *x,
                                   int           *y,
                                   int           *width,
                                   int           *height,
                                   AtkCoordType   coord_type)
{
  GtkWidget *widget;
  GdkSurface *surface;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  surface = gtk_native_get_surface (GTK_NATIVE (widget));
  if (surface == NULL)
    return;

  *x = 0;
  *y = 0;
  *width = gdk_surface_get_width (surface);
  *height = gdk_surface_get_height (surface);
  if (!gtk_widget_is_drawable (widget))
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }
}

static void
gtk_window_accessible_get_size (AtkComponent *component,
                                int          *width,
                                int          *height)
{
  GtkWidget *widget;
  GdkSurface *surface;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  surface = gtk_native_get_surface (GTK_NATIVE (widget));
  if (surface == NULL)
    return;

  *width = gdk_surface_get_width (surface);
  *height = gdk_surface_get_height (surface);
}

void
_gtk_window_accessible_set_is_active (GtkWindow   *window,
                                      gboolean     is_active)
{
  AtkObject *accessible = _gtk_widget_peek_accessible (GTK_WIDGET (window));

  if (accessible == NULL)
    return;

  g_signal_emit_by_name (accessible, is_active ? "activate" : "deactivate");
}


static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = gtk_window_accessible_get_extents;
  iface->get_size = gtk_window_accessible_get_size;
}

static void
atk_window_interface_init (AtkWindowIface *iface)
{
  /* At this moment AtkWindow is just about signals */
}
