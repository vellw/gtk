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

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "gtkbuttonaccessible.h"


static void atk_action_interface_init (AtkActionIface *iface);
static void atk_image_interface_init  (AtkImageIface  *iface);

G_DEFINE_TYPE_WITH_CODE (GtkButtonAccessible, gtk_button_accessible, GTK_TYPE_WIDGET_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE, atk_image_interface_init))

static void
gtk_button_accessible_initialize (AtkObject *obj,
                                  gpointer   data)
{
  GtkWidget *parent;

  ATK_OBJECT_CLASS (gtk_button_accessible_parent_class)->initialize (obj, data);

  parent = gtk_widget_get_parent (gtk_accessible_get_widget (GTK_ACCESSIBLE (obj)));
  if (GTK_IS_TREE_VIEW (parent))
    {
      /* Even though the accessible parent of the column header will
       * be reported as the table because the parent widget of the
       * GtkTreeViewColumn's button is the GtkTreeView we set
       * the accessible parent for column header to be the table
       * to ensure that atk_object_get_index_in_parent() returns
       * the correct value; see gail_widget_get_index_in_parent().
       */
      atk_object_set_parent (obj, gtk_widget_get_accessible (parent));
      obj->role = ATK_ROLE_TABLE_COLUMN_HEADER;
    }
  else
    obj->role = ATK_ROLE_PUSH_BUTTON;
}

static GtkWidget *
get_image_from_button (GtkWidget *button)
{
  GtkWidget *image;

  image = gtk_button_get_child (GTK_BUTTON (button));
  if (GTK_IS_IMAGE (image))
    return image;

  return NULL;
}

static GtkWidget *
find_label_child (GtkWidget *widget)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (widget))
    {
      if (GTK_IS_LABEL (child))
        return child;
      else
        {
          GtkWidget *w = find_label_child (child);
          if (w)
            return w;
        }
    }

  return NULL;
}

static GtkWidget *
get_label_from_button (GtkWidget *button)
{
  GtkWidget *child;

  child = gtk_button_get_child (GTK_BUTTON (button));

  if (!GTK_IS_LABEL (child))
    child = find_label_child (child);

  return child;
}

static const char *
gtk_button_accessible_get_name (AtkObject *obj)
{
  const char *name = NULL;
  GtkWidget *widget;
  GtkWidget *child;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (gtk_button_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  child = get_label_from_button (widget);
  if (GTK_IS_LABEL (child))
    name = gtk_label_get_text (GTK_LABEL (child));
  else
    {
      GtkWidget *image;

      image = get_image_from_button (widget);
      if (GTK_IS_IMAGE (image))
        {
          AtkObject *atk_obj;

          atk_obj = gtk_widget_get_accessible (image);
          name = atk_object_get_name (atk_obj);
        }
    }

  return name;
}

static int
gtk_button_accessible_get_n_children (AtkObject* obj)
{
  return 0;
}

static AtkObject *
gtk_button_accessible_ref_child (AtkObject *obj,
                                 int        i)
{
  return NULL;
}

static AtkStateSet *
gtk_button_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  GtkWidget *widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (gtk_button_accessible_parent_class)->ref_state_set (obj);

  if ((gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE) != 0)
    atk_state_set_add_state (state_set, ATK_STATE_ARMED);

  if (!gtk_widget_get_can_focus (widget))
    atk_state_set_remove_state (state_set, ATK_STATE_SELECTABLE);

  return state_set;
}

void
gtk_button_accessible_update_label (GtkButtonAccessible *self)
{
  g_return_if_fail (GTK_IS_BUTTON_ACCESSIBLE (self));

  /* If we don't have an overridden name, we use the label as the
   * accessible name
   */
  if (atk_object_get_name (ATK_OBJECT (self)) == NULL)
    g_object_notify (G_OBJECT (self), "accessible-name");

  g_signal_emit_by_name (self, "visible-data-changed");
}

static void
gtk_button_accessible_class_init (GtkButtonAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_name = gtk_button_accessible_get_name;
  class->get_n_children = gtk_button_accessible_get_n_children;
  class->ref_child = gtk_button_accessible_ref_child;
  class->ref_state_set = gtk_button_accessible_ref_state_set;
  class->initialize = gtk_button_accessible_initialize;
}

static void
gtk_button_accessible_init (GtkButtonAccessible *button)
{
}

static gboolean
gtk_button_accessible_do_action (AtkAction *action,
                                 int        i)
{
  GtkWidget *widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!gtk_widget_is_sensitive (widget) || !gtk_widget_get_visible (widget))
    return FALSE;

  g_signal_emit_by_name (widget, "clicked");
  return TRUE;
}

static int
gtk_button_accessible_get_n_actions (AtkAction *action)
{
  return 1;
}

static const char *
gtk_button_accessible_get_keybinding (AtkAction *action,
                                      int        i)
{
  char *return_value = NULL;
  GtkWidget *widget;
  GtkWidget *label;
  guint key_val;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (action));
  if (widget == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  label = get_label_from_button (widget);
  if (GTK_IS_LABEL (label))
    {
      key_val = gtk_label_get_mnemonic_keyval (GTK_LABEL (label));
      if (key_val != GDK_KEY_VoidSymbol)
        return_value = gtk_accelerator_name (key_val, GDK_ALT_MASK);
    }
  if (return_value == NULL)
    {
      /* Find labelled-by relation */
      AtkRelationSet *set;
      AtkRelation *relation;
      GPtrArray *target;
      gpointer target_object;

      set = atk_object_ref_relation_set (ATK_OBJECT (action));
      if (set)
        {
          relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_LABELLED_BY);
          if (relation)
            {
              target = atk_relation_get_target (relation);
              target_object = g_ptr_array_index (target, 0);
              label = gtk_accessible_get_widget (GTK_ACCESSIBLE (target_object));
            }
          g_object_unref (set);
        }

      if (GTK_IS_LABEL (label))
        {
          key_val = gtk_label_get_mnemonic_keyval (GTK_LABEL (label));
          if (key_val != GDK_KEY_VoidSymbol)
            return_value = gtk_accelerator_name (key_val, GDK_ALT_MASK);
        }
    }
  return return_value;
}

static const char *
gtk_button_accessible_action_get_name (AtkAction *action,
                                       int        i)
{
  if (i == 0)
    return "click";
  return NULL;
}

static const char *
gtk_button_accessible_action_get_localized_name (AtkAction *action,
                                                 int        i)
{
  if (i == 0)
    return C_("Action name", "Click");
  return NULL;
}

static const char *
gtk_button_accessible_action_get_description (AtkAction *action,
                                              int        i)
{
  if (i == 0)
    return C_("Action description", "Clicks the button");
  return NULL;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = gtk_button_accessible_do_action;
  iface->get_n_actions = gtk_button_accessible_get_n_actions;
  iface->get_keybinding = gtk_button_accessible_get_keybinding;
  iface->get_name = gtk_button_accessible_action_get_name;
  iface->get_localized_name = gtk_button_accessible_action_get_localized_name;
  iface->get_description = gtk_button_accessible_action_get_description;
}

static const char *
gtk_button_accessible_get_image_description (AtkImage *image)
{
  GtkWidget *widget;
  GtkWidget  *button_image;
  AtkObject *obj;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (image));
  if (widget == NULL)
    return NULL;

  button_image = get_image_from_button (widget);
  if (GTK_IS_IMAGE (button_image))
    {
      obj = gtk_widget_get_accessible (button_image);
      return atk_image_get_image_description (ATK_IMAGE (obj));
    }

  return NULL;
}

static void
gtk_button_accessible_get_image_position (AtkImage     *image,
                                          int          *x,
                                          int          *y,
                                          AtkCoordType  coord_type)
{
  GtkWidget *widget;
  GtkWidget *button_image;
  AtkObject *obj;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (image));
  if (widget == NULL)
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  button_image = get_image_from_button (widget);
  if (button_image != NULL)
    {
      obj = gtk_widget_get_accessible (button_image);
      atk_component_get_extents (ATK_COMPONENT (obj), x, y, NULL, NULL,
                                 coord_type);
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static void
gtk_button_accessible_get_image_size (AtkImage *image,
                                      int      *width,
                                      int      *height)
{
  GtkWidget *widget;
  GtkWidget *button_image;
  AtkObject *obj;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (image));
  if (widget == NULL)
    {
      *width = -1;
      *height = -1;
      return;
    }

  button_image = get_image_from_button (widget);
  if (GTK_IS_IMAGE (button_image))
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (button_image));
      atk_image_get_image_size (ATK_IMAGE (obj), width, height);
    }
  else
    {
      *width = -1;
      *height = -1;
    }
}

static gboolean
gtk_button_accessible_set_image_description (AtkImage    *image,
                                             const char *description)
{
  GtkWidget *widget;
  GtkWidget *button_image;
  AtkObject *obj;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (image));

  if (widget == NULL)
    return FALSE;

  button_image = get_image_from_button (widget);
  if (GTK_IMAGE (button_image))
    {
      obj = gtk_widget_get_accessible (GTK_WIDGET (button_image));
      return atk_image_set_image_description (ATK_IMAGE (obj), description);
    }

  return FALSE;
}

static void
atk_image_interface_init (AtkImageIface *iface)
{
  iface->get_image_description = gtk_button_accessible_get_image_description;
  iface->get_image_position = gtk_button_accessible_get_image_position;
  iface->get_image_size = gtk_button_accessible_get_image_size;
  iface->set_image_description = gtk_button_accessible_set_image_description;
}
