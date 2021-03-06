/* GTK+ - accessibility implementations
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

#include "gtkwidgetaccessibleprivate.h"

#include "gtkaccellabel.h"
#include "gtkadjustment.h"
#include "gtkbox.h"
#include "gtkbutton.h"
#include "gtkcombobox.h"
#include "gtkdragicon.h"
#include "gtkdrawingarea.h"
#include "gtkglarea.h"
#include "gtkimage.h"
#include "gtklevelbar.h"
#include "gtkmediacontrols.h"
#include "gtknotebookpageaccessible.h"
#include "gtknotebook.h"
#include "gtkorientable.h"
#include "gtkpicture.h"
#include "gtkprogressbar.h"
#include "gtkscrollable.h"
#include "gtkscrollbar.h"
#include "gtkseparator.h"
#include "gtkshortcutlabel.h"
#include "gtkshortcutsshortcut.h"
#include "gtkspinner.h"
#include "gtkstacksidebar.h"
#include "gtkstatusbar.h"
#include "gtkvideo.h"
#include "gtkviewport.h"
#include "gtkwidgetprivate.h"

typedef struct
{
  AtkLayer layer;
} GtkWidgetAccessiblePrivate;

extern GtkWidget *_focus_widget;

static gboolean gtk_widget_accessible_on_screen           (GtkWidget *widget);
static gboolean gtk_widget_accessible_all_parents_visible (GtkWidget *widget);

static void atk_component_interface_init (AtkComponentIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkWidgetAccessible, gtk_widget_accessible, GTK_TYPE_ACCESSIBLE,
                         G_ADD_PRIVATE (GtkWidgetAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, atk_component_interface_init))

/*< private >
 * gtk_widget_accessible_update_bounds:
 * @self: a #GtkWidgetAccessible
 *
 * Updates the bounds of the widget's accessible implementation using
 * the widget's allocation.
 */
void
gtk_widget_accessible_update_bounds (GtkWidgetAccessible *self)
{
  GtkAllocation alloc;
  AtkRectangle rect;

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));
  gtk_widget_get_allocation (widget, &alloc);

  rect.x = alloc.x;
  rect.y = alloc.y;
  rect.width = alloc.width;
  rect.height = alloc.height;

  g_signal_emit_by_name (self, "bounds-changed", &rect);
}

/*< private >
 * gtk_widget_accessible_notify_showing:
 * @self: a #GtkWidgetAccessible
 *
 * Translates the #GtkWidget mapped state into the #AtkObject
 * showing state.
 */
void
gtk_widget_accessible_notify_showing (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));

  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_SHOWING,
                                  gtk_widget_get_mapped (widget));
}

void
gtk_widget_accessible_notify_tooltip (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));

  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_HAS_TOOLTIP,
                                  gtk_widget_get_has_tooltip (widget));
}

void
gtk_widget_accessible_notify_visible (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));

  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_VISIBLE,
                                  gtk_widget_get_visible (widget));
}

void
gtk_widget_accessible_notify_sensitive (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));
  gboolean is_sensitive = gtk_widget_get_sensitive (widget);

  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_SENSITIVE,
                                  is_sensitive);
  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_ENABLED,
                                  is_sensitive);
}

void
gtk_widget_accessible_notify_focus (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));

  atk_object_notify_state_change (ATK_OBJECT (self),
                                  ATK_STATE_FOCUSED,
                                  gtk_widget_has_focus (widget));
}

void
gtk_widget_accessible_notify_orientation (GtkWidgetAccessible *self)
{
  g_return_if_fail (GTK_IS_WIDGET_ACCESSIBLE (self));

  GtkWidget *widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (self));

  if (GTK_IS_ORIENTABLE (widget))
    {
      GtkOrientable *orientable = GTK_ORIENTABLE (widget);
      GtkOrientation orientation = gtk_orientable_get_orientation (orientable);

      atk_object_notify_state_change (ATK_OBJECT (self),
                                      ATK_STATE_HORIZONTAL,
                                      orientation == GTK_ORIENTATION_HORIZONTAL);
      atk_object_notify_state_change (ATK_OBJECT (self),
                                      ATK_STATE_VERTICAL,
                                      orientation == GTK_ORIENTATION_VERTICAL);
    }
}

static void
gtk_widget_accessible_initialize (AtkObject *object,
                                  gpointer   data)
{
  GtkWidgetAccessible *self = GTK_WIDGET_ACCESSIBLE (object);
  GtkWidgetAccessiblePrivate *priv = gtk_widget_accessible_get_instance_private (self);

  priv->layer = ATK_LAYER_WIDGET;

  atk_object_set_role (object, ATK_ROLE_UNKNOWN);
}

static const char *
gtk_widget_accessible_get_description (AtkObject *accessible)
{
  GtkWidget *widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  if (accessible->description != NULL)
    return accessible->description;

  return gtk_widget_get_tooltip_text (widget);
}

static AtkObject *
gtk_widget_accessible_get_parent (AtkObject *accessible)
{
  AtkObject *parent;
  GtkWidget *widget, *parent_widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  parent = accessible->accessible_parent;
  if (parent != NULL)
    return parent;

  parent_widget = gtk_widget_get_parent (widget);
  if (parent_widget == NULL)
    return NULL;

  /* For a widget whose parent is a GtkNoteBook, we return the
   * accessible object corresponding the GtkNotebookPage containing
   * the widget as the accessible parent.
   */
  if (GTK_IS_NOTEBOOK (parent_widget))
    {
      int page_num;
      GtkWidget *child;
      GtkNotebook *notebook;

      page_num = 0;
      notebook = GTK_NOTEBOOK (parent_widget);
      while (TRUE)
        {
          child = gtk_notebook_get_nth_page (notebook, page_num);
          if (!child)
            break;
          if (child == widget)
            {
              parent = gtk_widget_get_accessible (parent_widget);
              parent = atk_object_ref_accessible_child (parent, page_num);
              g_object_unref (parent);
              return parent;
            }
          page_num++;
        }
    }
  parent = gtk_widget_get_accessible (parent_widget);
  return parent;
}

static GtkWidget *
find_label (GtkWidget *widget)
{
  GList *labels;
  GtkWidget *label;
  GtkWidget *temp_widget;
  GList *ptr;

  labels = gtk_widget_list_mnemonic_labels (widget);
  label = NULL;
  ptr = labels;
  while (ptr)
    {
      if (ptr->data)
        {
          label = ptr->data;
          break;
        }
      ptr = ptr->next;
    }
  g_list_free (labels);

  /* Ignore a label within a button; bug #136602 */
  if (label && GTK_IS_BUTTON (widget))
    {
      temp_widget = label;
      while (temp_widget)
        {
          if (temp_widget == widget)
            {
              label = NULL;
              break;
            }
          temp_widget = gtk_widget_get_parent (temp_widget);
        }
    }
  return label;
}

static AtkRelationSet *
gtk_widget_accessible_ref_relation_set (AtkObject *obj)
{
  GtkWidget *widget;
  AtkRelationSet *relation_set;
  GtkWidget *label;
  AtkObject *array[1];
  AtkRelation* relation;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  relation_set = ATK_OBJECT_CLASS (gtk_widget_accessible_parent_class)->ref_relation_set (obj);

  if (GTK_IS_BOX (widget))
    return relation_set;

  if (!atk_relation_set_contains (relation_set, ATK_RELATION_LABELLED_BY))
    {
      label = find_label (widget);
      if (label == NULL)
        {
          if (GTK_IS_BUTTON (widget) && gtk_widget_get_mapped (widget))
            /*
             * Handle the case where GnomeIconEntry is the mnemonic widget.
             * The GtkButton which is a grandchild of the GnomeIconEntry
             * should really be the mnemonic widget. See bug #133967.
             */
            {
              GtkWidget *temp_widget;

              temp_widget = gtk_widget_get_parent (widget);
              if (GTK_IS_BOX (temp_widget))
                {
                  label = find_label (temp_widget);
                  if (!label)
                    label = find_label (gtk_widget_get_parent (temp_widget));
                }
            }
          else if (GTK_IS_COMBO_BOX (widget))
            /*
             * Handle the case when GtkFileChooserButton is the mnemonic
             * widget.  The GtkComboBox which is a child of the
             * GtkFileChooserButton should be the mnemonic widget.
             * See bug #359843.
             */
            {
              GtkWidget *temp_widget;

              temp_widget = gtk_widget_get_parent (widget);
              if (GTK_IS_BOX (temp_widget))
                {
                  label = find_label (temp_widget);
                }
            }
        }

      if (label)
        {
          array[0] = gtk_widget_get_accessible (label);

          relation = atk_relation_new (array, 1, ATK_RELATION_LABELLED_BY);
          atk_relation_set_add (relation_set, relation);
          g_object_unref (relation);
        }
    }

  return relation_set;
}

static gboolean
takes_focus (GtkWidget *widget)
{
  if (GTK_IS_NOTEBOOK (widget) ||
      GTK_IS_BUTTON (widget))
    return TRUE;

  if (GTK_IS_ACCEL_LABEL (widget) ||
      GTK_IS_DRAG_ICON (widget) ||
      GTK_IS_DRAWING_AREA (widget) ||
      GTK_IS_GL_AREA (widget) ||
      GTK_IS_IMAGE (widget) ||
      GTK_IS_LEVEL_BAR (widget) ||
      GTK_IS_MEDIA_CONTROLS (widget) ||
      GTK_IS_PICTURE (widget) ||
      GTK_IS_PROGRESS_BAR (widget) ||
      GTK_IS_SCROLLBAR (widget) ||
      GTK_IS_SEPARATOR (widget) ||
      GTK_IS_SHORTCUT_LABEL (widget) ||
      GTK_IS_SHORTCUTS_SHORTCUT (widget) ||
      GTK_IS_SPINNER (widget) ||
      GTK_IS_STACK_SIDEBAR (widget) ||
      GTK_IS_STATUSBAR (widget) ||
      GTK_IS_VIDEO (widget))
    return FALSE;

  return gtk_widget_get_can_focus (widget);
}

static AtkStateSet *
gtk_widget_accessible_ref_state_set (AtkObject *accessible)
{
  GtkWidget *widget;
  AtkStateSet *state_set;

  state_set = ATK_OBJECT_CLASS (gtk_widget_accessible_parent_class)->ref_state_set (accessible);

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
  else
    {
      if (gtk_widget_is_sensitive (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
          atk_state_set_add_state (state_set, ATK_STATE_ENABLED);
        }
  
      if (takes_focus (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
        }
      /*
       * We do not currently generate notifications when an ATK object
       * corresponding to a GtkWidget changes visibility by being scrolled
       * on or off the screen.  The testcase for this is the main window
       * of the testgtk application in which a set of buttons in a GtkVBox
       * is in a scrolled window with a viewport.
       *
       * To generate the notifications we would need to do the following:
       * 1) Find the GtkViewport among the ancestors of the objects
       * 2) Create an accessible for the viewport
       * 3) Connect to the value-changed signal on the viewport
       * 4) When the signal is received we need to traverse the children
       *    of the viewport and check whether the children are visible or not
       *    visible; we may want to restrict this to the widgets for which
       *    accessible objects have been created.
       * 5) We probably need to store a variable on_screen in the
       *    GtkWidgetAccessible data structure so we can determine whether
       *    the value has changed.
       */
      if (gtk_widget_get_visible (widget))
        {
          atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);
          if (gtk_widget_accessible_on_screen (widget) &&
              gtk_widget_get_mapped (widget) &&
              gtk_widget_accessible_all_parents_visible (widget))
            atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
        }

      if (gtk_widget_has_focus (widget))
        atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);

      if (gtk_widget_has_default (widget))
        atk_state_set_add_state (state_set, ATK_STATE_DEFAULT);

      if (GTK_IS_ORIENTABLE (widget))
        {
          if (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)) == GTK_ORIENTATION_HORIZONTAL)
            atk_state_set_add_state (state_set, ATK_STATE_HORIZONTAL);
          else
            atk_state_set_add_state (state_set, ATK_STATE_VERTICAL);
        }

      if (gtk_widget_get_has_tooltip (widget))
        atk_state_set_add_state (state_set, ATK_STATE_HAS_TOOLTIP);
    }
  return state_set;
}

static int
gtk_widget_accessible_get_index_in_parent (AtkObject *accessible)
{
  GtkWidget *widget;
  GtkWidget *parent_widget;
  int index;
  GtkWidget *ch;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));

  if (widget == NULL)
    return -1;

  if (accessible->accessible_parent)
    {
      AtkObject *parent;

      parent = accessible->accessible_parent;

      if (GTK_IS_NOTEBOOK_PAGE_ACCESSIBLE (parent))
        return 0;
      else
        {
          int n_children, i;
          gboolean found = FALSE;

          n_children = atk_object_get_n_accessible_children (parent);
          for (i = 0; i < n_children; i++)
            {
              AtkObject *child;

              child = atk_object_ref_accessible_child (parent, i);
              if (child == accessible)
                found = TRUE;

              g_object_unref (child);
              if (found)
                return i;
            }
        }
    }

  parent_widget = gtk_widget_get_parent (widget);
  for (ch = gtk_widget_get_first_child (parent_widget), index = 0;
       ch != NULL;
       ch = gtk_widget_get_next_sibling (ch), index++)
    {
      if (ch == widget)
        break;
    }

  return index;
}

static AtkAttributeSet *
gtk_widget_accessible_get_attributes (AtkObject *obj)
{
  AtkAttributeSet *attributes;
  AtkAttribute *toolkit;

  toolkit = g_new (AtkAttribute, 1);
  toolkit->name = g_strdup ("toolkit");
  toolkit->value = g_strdup ("gtk");

  attributes = g_slist_append (NULL, toolkit);

  return attributes;
}

static int
gtk_widget_accessible_get_n_children (AtkObject *object)
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
gtk_widget_accessible_ref_child (AtkObject *object,
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
gtk_widget_accessible_class_init (GtkWidgetAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_description = gtk_widget_accessible_get_description;
  class->get_parent = gtk_widget_accessible_get_parent;
  class->ref_relation_set = gtk_widget_accessible_ref_relation_set;
  class->ref_state_set = gtk_widget_accessible_ref_state_set;
  class->get_index_in_parent = gtk_widget_accessible_get_index_in_parent;
  class->initialize = gtk_widget_accessible_initialize;
  class->get_attributes = gtk_widget_accessible_get_attributes;
  class->get_n_children = gtk_widget_accessible_get_n_children;
  class->ref_child = gtk_widget_accessible_ref_child;
}

static void
gtk_widget_accessible_init (GtkWidgetAccessible *accessible)
{
}

static void
gtk_widget_accessible_get_extents (AtkComponent   *component,
                                   int            *x,
                                   int            *y,
                                   int            *width,
                                   int            *height,
                                   AtkCoordType    coord_type)
{
  GtkWidget *widget;
  GtkAllocation allocation;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (widget == NULL)
    return;

  gtk_widget_get_allocation (widget, &allocation);
  *width = allocation.width;
  *height = allocation.height;
  if (!gtk_widget_accessible_on_screen (widget) || (!gtk_widget_is_drawable (widget)))
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  if (gtk_widget_get_parent (widget))
    {
      *x = allocation.x;
      *y = allocation.y;
    }
  else
    {
      *x = 0;
      *y = 0;
    }
}

static AtkLayer
gtk_widget_accessible_get_layer (AtkComponent *component)
{
  GtkWidgetAccessible *self = GTK_WIDGET_ACCESSIBLE (component);
  GtkWidgetAccessiblePrivate *priv = gtk_widget_accessible_get_instance_private (self);

  return priv->layer;
}

static gboolean
gtk_widget_accessible_grab_focus (AtkComponent *component)
{
  GtkWidget *widget;
  GtkWidget *toplevel;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (!widget)
    return FALSE;

  if (!gtk_widget_get_can_focus (widget))
    return FALSE;

  gtk_widget_grab_focus (widget);
  toplevel = GTK_WIDGET (gtk_widget_get_root (widget));
  if (GTK_IS_WINDOW (toplevel))
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gtk_window_present (GTK_WINDOW (toplevel));
      G_GNUC_END_IGNORE_DEPRECATIONS
    }

  return TRUE;
}

static gboolean
gtk_widget_accessible_set_extents (AtkComponent *component,
                                   int           x,
                                   int           y,
                                   int           width,
                                   int           height,
                                   AtkCoordType  coord_type)
{
  return FALSE;
}

static gboolean
gtk_widget_accessible_set_position (AtkComponent *component,
                                    int           x,
                                    int           y,
                                    AtkCoordType  coord_type)
{
  return FALSE;
}

static gboolean
gtk_widget_accessible_set_size (AtkComponent *component,
                                int           width,
                                int           height)
{
  return FALSE;
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = gtk_widget_accessible_get_extents;
  iface->get_layer = gtk_widget_accessible_get_layer;
  iface->grab_focus = gtk_widget_accessible_grab_focus;
  iface->set_extents = gtk_widget_accessible_set_extents;
  iface->set_position = gtk_widget_accessible_set_position;
  iface->set_size = gtk_widget_accessible_set_size;
}

/* This function checks whether the widget has an ancestor which is
 * a GtkViewport and, if so, whether any part of the widget intersects
 * the visible rectangle of the GtkViewport.
 */
static gboolean
gtk_widget_accessible_on_screen (GtkWidget *widget)
{
  GtkAllocation allocation;
  GtkWidget *viewport;
  gboolean return_value;

  gtk_widget_get_allocation (widget, &allocation);

  if (!gtk_widget_get_mapped (widget))
    return FALSE;

  viewport = gtk_widget_get_ancestor (widget, GTK_TYPE_VIEWPORT);
  if (viewport)
    {
      GtkAllocation viewport_allocation;
      GtkAdjustment *adjustment;
      GdkRectangle visible_rect;

      gtk_widget_get_allocation (viewport, &viewport_allocation);

      adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (viewport));
      visible_rect.y = gtk_adjustment_get_value (adjustment);
      adjustment = gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (viewport));
      visible_rect.x = gtk_adjustment_get_value (adjustment);
      visible_rect.width = viewport_allocation.width;
      visible_rect.height = viewport_allocation.height;

      if (((allocation.x + allocation.width) < visible_rect.x) ||
         ((allocation.y + allocation.height) < visible_rect.y) ||
         (allocation.x > (visible_rect.x + visible_rect.width)) ||
         (allocation.y > (visible_rect.y + visible_rect.height)))
        return_value = FALSE;
      else
        return_value = TRUE;
    }
  else
    {
      /* Check whether the widget has been placed of the screen.
       * The widget may be MAPPED as when toolbar items do not
       * fit on the toolbar.
       */
      if (allocation.x + allocation.width <= 0 &&
          allocation.y + allocation.height <= 0)
        return_value = FALSE;
      else
        return_value = TRUE;
    }

  return return_value;
}

/* Checks if all the predecessors (the parent widget, his parent, etc)
 * are visible Used to check properly the SHOWING state.
 */
static gboolean
gtk_widget_accessible_all_parents_visible (GtkWidget *widget)
{
  GtkWidget *iter_parent = NULL;
  gboolean result = TRUE;

  for (iter_parent = _gtk_widget_get_parent (widget);
       iter_parent != NULL;
       iter_parent = _gtk_widget_get_parent (iter_parent))
    {
      if (!_gtk_widget_get_visible (iter_parent))
        {
          result = FALSE;
          break;
        }
    }

  return result;
}

void
_gtk_widget_accessible_set_layer (GtkWidgetAccessible *self,
                                  AtkLayer             layer)
{
  GtkWidgetAccessiblePrivate *priv = gtk_widget_accessible_get_instance_private (self);

  priv->layer = layer;
}
