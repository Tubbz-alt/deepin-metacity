/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * deepin-wm-background.c
 * Copyright (C) 2015 Sian Cao <yinshuiboy@gmail.com>
 *
 * metacity is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * metacity is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gdk/gdkx.h>
#include <prefs.h>
#include "deepin-wm-background.h"
#include "../core/screen-private.h"
#include "../core/display-private.h"
#include "deepin-design.h"
#include "deepin-shadow-workspace.h"
#include "deepin-window-surface-manager.h"
#include "deepin-fixed.h"

struct _DeepinWMBackgroundPrivate
{
    MetaScreen* screen;
    GdkScreen* gscreen;

    gint disposed: 1;
    GtkWidget* fixed;

    DeepinShadowWorkspace* active_workspace;
    GList* worskpaces;
    GList* worskpace_thumbs;
    /*DeepinShadowWorkspaceAdder* adder;*/

    /* calculation cache */
    int top_offset;
    int bottom_offset;
    float scale;
    gint width, height; /* for workspace */
    gint thumb_width, thumb_height; /* for top thumbs */
};


G_DEFINE_TYPE (DeepinWMBackground, deepin_wm_background, GTK_TYPE_WINDOW);

static void deepin_wm_background_init (DeepinWMBackground *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DEEPIN_TYPE_WM_BACKGROUND, DeepinWMBackgroundPrivate);

    DeepinWMBackgroundPrivate* priv = self->priv;
    priv->worskpaces = NULL;
}

static void deepin_wm_background_finalize (GObject *object)
{
    /* TODO: Add deinitalization code here */

    G_OBJECT_CLASS (deepin_wm_background_parent_class)->finalize (object);
}

static gboolean deepin_wm_background_real_draw(GtkWidget *widget, cairo_t* cr)
{
    return GTK_WIDGET_CLASS(deepin_wm_background_parent_class)->draw(widget, cr);
}

static void deepin_wm_background_class_init (DeepinWMBackgroundClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);

    g_type_class_add_private (klass, sizeof (DeepinWMBackgroundPrivate));
    widget_class->draw = deepin_wm_background_real_draw;

    object_class->finalize = deepin_wm_background_finalize;
}

static gint find_by_meta_workspace(gconstpointer a, gconstpointer b)
{
    DeepinShadowWorkspace* ws = (DeepinShadowWorkspace*)a;
    MetaWorkspace* meta = (MetaWorkspace*)b;
    if (deepin_shadow_workspace_get_workspace(ws) == meta) {
        return 0;
    }

    return 1;
}

static DeepinShadowWorkspace* _find_workspace(GList* l, MetaWorkspace* next)
{
    GList* tmp = g_list_find_custom(l, next, find_by_meta_workspace);
    g_assert(tmp->data);
    return (DeepinShadowWorkspace*)tmp->data;
}

void deepin_wm_background_switch_workspace(DeepinWMBackground* self, 
        MetaWorkspace* next, MetaMotionDirection dir)
{
    DeepinWMBackgroundPrivate* priv = self->priv;
    DeepinShadowWorkspace* next_ws = _find_workspace(priv->worskpaces, next);
    DeepinShadowWorkspace* next_thumb = _find_workspace(priv->worskpace_thumbs, next);

    DeepinShadowWorkspace* current_thumb = _find_workspace(priv->worskpace_thumbs,
                deepin_shadow_workspace_get_workspace(priv->active_workspace));
    DeepinShadowWorkspace* current = priv->active_workspace;

    //DO move animation
    deepin_shadow_workspace_set_current(current, FALSE);
    deepin_shadow_workspace_set_current(current_thumb, FALSE);

    deepin_shadow_workspace_set_current(next_ws, TRUE);
    deepin_shadow_workspace_set_current(next_thumb, TRUE);

    priv->active_workspace = next_ws;

    GdkRectangle geom;
    gint monitor_index = gdk_screen_get_monitor_at_window(priv->gscreen,
            gtk_widget_get_window(GTK_WIDGET(self)));
    gdk_screen_get_monitor_geometry(priv->gscreen, monitor_index, &geom);

    gint i = 0,
         current_pos = g_list_index(priv->worskpaces, next_ws),
         pad = FLOW_CLONE_DISTANCE_PERCENT * geom.width;

    GList* l = priv->worskpaces;
    while (l) {
        gint x = (geom.width - priv->width) / 2 +  (i - current_pos) * (priv->width + pad);
        deepin_fixed_move(DEEPIN_FIXED(priv->fixed), (GtkWidget*)l->data,
                x + priv->width/2, priv->top_offset + priv->height/2, TRUE);

        i++;
        l = l->next;
    }
}

void deepin_wm_background_setup(DeepinWMBackground* self)
{
    DeepinWMBackgroundPrivate* priv = self->priv;

    GdkRectangle geom;

    gint monitor_index = gdk_screen_get_monitor_at_window(priv->gscreen,
            gtk_widget_get_window(GTK_WIDGET(self)));
    gdk_screen_get_monitor_geometry(priv->gscreen, monitor_index, &geom);
            
    priv->fixed = deepin_fixed_new();
    gtk_container_add(GTK_CONTAINER(self), priv->fixed);

    priv->top_offset = (int)(geom.height * FLOW_CLONE_TOP_OFFSET_PERCENT);
    priv->bottom_offset = (int)(geom.height * HORIZONTAL_OFFSET_PERCENT);
    float scale = (float)(geom.height - priv->top_offset - priv->bottom_offset) / geom.height;

    priv->scale = scale;
    priv->width = geom.width * scale;
    priv->height = geom.height * scale;

    // calculate monitor width height ratio
    float monitor_whr = (float)geom.height / geom.width;
    priv->thumb_width = geom.width * WORKSPACE_WIDTH_PERCENT;
    priv->thumb_height = priv->thumb_width * monitor_whr;

    GList *l = priv->screen->workspaces;
    gint current = 0;

    while (l) {
        MetaWorkspace* ws = (MetaWorkspace*)l->data;
        {
            DeepinShadowWorkspace* dsw = 
                (DeepinShadowWorkspace*)deepin_shadow_workspace_new();
            deepin_shadow_workspace_set_scale(dsw, scale);
            deepin_shadow_workspace_populate(dsw, ws);

            if (ws == priv->screen->active_workspace) {
                current = g_list_index(priv->screen->workspaces, l->data);
                priv->active_workspace = dsw;
                deepin_shadow_workspace_set_presentation(dsw, TRUE);
                deepin_shadow_workspace_set_current(dsw, TRUE);
            }

            priv->worskpaces = g_list_append(priv->worskpaces, dsw);
        }

        /* for top workspace thumbnail */
        {
            DeepinShadowWorkspace* dsw = 
                (DeepinShadowWorkspace*)deepin_shadow_workspace_new();
            deepin_shadow_workspace_set_thumb_mode(dsw, TRUE);
            deepin_shadow_workspace_set_scale(dsw, WORKSPACE_WIDTH_PERCENT);
            deepin_shadow_workspace_populate(dsw, ws);

            if (ws == priv->screen->active_workspace) {
                deepin_shadow_workspace_set_current(dsw, TRUE);
            }

            priv->worskpace_thumbs = g_list_append(priv->worskpace_thumbs, dsw);
        }

        l = l->next;
    }

    gint i = 0, pad = FLOW_CLONE_DISTANCE_PERCENT * geom.width;
    l = priv->worskpaces;
    while (l) {
        gint x = (geom.width - priv->width) / 2 +  (i - current) * (priv->width + pad);
        deepin_fixed_put(DEEPIN_FIXED(priv->fixed), (GtkWidget*)l->data,
                x + priv->width/2, priv->top_offset + priv->height/2);

        i++;
        l = l->next;
    }

    i = 0;
    l = priv->worskpace_thumbs;
    int thumb_spacing = geom.width * SPACING_PERCENT;
    
    gint count = g_list_length(priv->worskpace_thumbs);
    int thumb_y = (int)(geom.height * HORIZONTAL_OFFSET_PERCENT);
    int thumb_x = (geom.width - count * (priv->thumb_width + thumb_spacing))/2;

    while (l) {
        int x = thumb_x + i * (priv->thumb_width + thumb_spacing);
        deepin_fixed_put(DEEPIN_FIXED(priv->fixed), (GtkWidget*)l->data,
                x + priv->thumb_width/2, thumb_y + priv->thumb_height/2);

        i++;
        l = l->next;
    }
}

static gboolean on_deepin_wm_background_event(DeepinWMBackground* self,
        GdkEvent* ev, gpointer data)
{
    DeepinWMBackgroundPrivate* priv = self->priv;

    switch(ev->type) {
        case GDK_BUTTON_PRESS:
            break;
        case GDK_BUTTON_RELEASE:
            break;
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE:
            return TRUE;

        default: break;
    }

    if (priv->active_workspace && gtk_widget_get_realized(priv->active_workspace))
        return gtk_widget_event(GTK_WIDGET(priv->active_workspace), ev);
    return FALSE;
}

GtkWidget* deepin_wm_background_new(MetaScreen* screen)
{
    deepin_window_surface_manager_flush();

    GtkWidget* widget = (GtkWidget*)g_object_new(DEEPIN_TYPE_WM_BACKGROUND,
            "type", GTK_WINDOW_POPUP, NULL);
    deepin_setup_style_class(widget, "deepin-window-manager"); 

    DeepinWMBackground* self = DEEPIN_WM_BACKGROUND(widget);

    MetaDisplay* display = meta_get_display();
    GdkDisplay* gdisplay = gdk_x11_lookup_xdisplay(display->xdisplay);
    self->priv->gscreen = gdk_display_get_default_screen(gdisplay);

    GdkVisual* visual = gdk_screen_get_rgba_visual (self->priv->gscreen);
    if (visual) gtk_widget_set_visual (widget, visual);

    gint w = screen->rect.width, h = screen->rect.height;
    gtk_window_set_position(GTK_WINDOW(widget), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_default_size(GTK_WINDOW(widget), w, h);
    gtk_widget_realize (widget);

    gtk_widget_add_events(widget, GDK_ALL_EVENTS_MASK);

    g_object_connect(G_OBJECT(widget),
            "signal::event", on_deepin_wm_background_event, NULL,
            NULL);

    self->priv->screen = screen;
    return widget;
}

void deepin_wm_background_handle_event(DeepinWMBackground* self, XEvent* event, 
        KeySym keysym, MetaKeyBindingAction action)
{
    DeepinWMBackgroundPrivate* priv = self->priv;

    if (keysym == XK_Left || keysym == XK_Right) {
        MetaMotionDirection dir = keysym == XK_Left ? META_MOTION_LEFT:META_MOTION_RIGHT;
        MetaWorkspace* current = deepin_shadow_workspace_get_workspace(priv->active_workspace); 
        MetaWorkspace* next = meta_workspace_get_neighbor(current, dir);
        if (next) {
            if (next == current) {
                //bouncing
                return;
            }

            meta_workspace_activate(next, event->xkey.time);
            deepin_wm_background_switch_workspace(self, next, dir);
        }

    } else { 
        /* pass through to active workspace */
        deepin_shadow_workspace_handle_event(priv->active_workspace,
                event, keysym, action);
    }
}

