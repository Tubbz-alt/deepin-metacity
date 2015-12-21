/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * deepin-window-surface-manager.c
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
#include <math.h>
#include <util.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#ifdef HAVE_COMPOSITE_EXTENSIONS
#include <X11/extensions/Xrender.h>
#endif
#include "errors.h"
#include "../core/window-private.h"
#include "compositor.h"
#include "deepin-design.h"
#include "deepin-window-surface-manager.h"
#include "deepin-message-hub.h"

static DeepinWindowSurfaceManager* _the_manager = NULL;

/*
 * MetaWindow -> surface list
 *   windows[i] is a GTree, key is scale, value is surface 
 */
struct _DeepinWindowSurfaceManagerPrivate
{
    GHashTable* windows;
};

enum
{
    SIGNAL_SURFACE_INVALID,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (DeepinWindowSurfaceManager, deepin_window_surface_manager, G_TYPE_OBJECT);

static void deepin_window_surface_manager_init (DeepinWindowSurfaceManager *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DEEPIN_TYPE_WINDOW_SURFACE_MANAGER, DeepinWindowSurfaceManagerPrivate);

    self->priv->windows = g_hash_table_new_full(g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify)g_tree_unref);
}

static void deepin_window_surface_manager_finalize (GObject *object)
{
    DeepinWindowSurfaceManager* self = DEEPIN_WINDOW_SURFACE_MANAGER(object);
    g_hash_table_unref(self->priv->windows);

	G_OBJECT_CLASS (deepin_window_surface_manager_parent_class)->finalize (object);
}

static void deepin_window_surface_manager_class_init (DeepinWindowSurfaceManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DeepinWindowSurfaceManagerPrivate));

	object_class->finalize = deepin_window_surface_manager_finalize;
    
    signals[SIGNAL_SURFACE_INVALID] = g_signal_new ("surface-invalid",
            DEEPIN_TYPE_WINDOW_SURFACE_MANAGER,
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static gint scale_compare(gconstpointer a, gconstpointer b, gpointer data)
{
    gdouble f1 = *(gdouble*)a;
    gdouble f2 = *(gdouble*)b;
    if (f1 < f2) return -1;
    else if (f1 > f2) return 1;
    return 0;
}

cairo_surface_t* deepin_window_surface_manager_get_surface(MetaWindow* window,
        double scale)
{
    DeepinWindowSurfaceManager* self = deepin_window_surface_manager_get();

    GTree* t = (GTree*)g_hash_table_lookup(self->priv->windows, window);
    if (!t) {
        t = g_tree_new_full(scale_compare, NULL, g_free, 
                (GDestroyNotify)cairo_surface_destroy);
        g_hash_table_insert(self->priv->windows, window, t);
    }

    double* s = g_new(double, 1);
    *s = 1.0;
    cairo_surface_t* ref = (cairo_surface_t*)g_tree_lookup(t, s);
    if (!ref) {
        ref = meta_compositor_get_window_surface(window->display->compositor,
            window);
        if (!ref) {
            g_free(s);
            return NULL;
        }

        MetaRectangle r, r2;
        meta_window_get_input_rect(window, &r);
        meta_window_get_outer_rect(window, &r2);

        cairo_format_t format = CAIRO_FORMAT_RGB24;
        if (window->depth == 32)
            format = CAIRO_FORMAT_ARGB32;

#ifdef HAVE_COMPOSITE_EXTENSIONS
        XRenderPictFormat *render_fmt;
        render_fmt = XRenderFindVisualFormat(window->display->xdisplay,
                window->xvisual);

        if (render_fmt && render_fmt->type == PictTypeDirect 
                && render_fmt->direct.alphaMask)
            format = CAIRO_FORMAT_ARGB32;

#endif

        cairo_surface_t* ret = cairo_image_surface_create(format, 
                r2.width, r2.height);

        meta_error_trap_push (window->display);

        cairo_t* cr = cairo_create(ret);
        cairo_set_source_surface(cr, ref, r.x - r2.x, r.y - r2.y);
        cairo_paint(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(ref);

        int error_code = meta_error_trap_pop_with_return (window->display, FALSE);
        if (error_code != 0) {
            meta_verbose("draw surface error %d", error_code);
            g_free(s);
            cairo_surface_destroy(ret);
            return NULL;

        } else {
            ref = ret;
            meta_verbose("%s: clip visible rect", window->desc);
            g_tree_insert(t, s, ref);
        }
    } else {
        g_free(s);
    }

    if (scale == 1.0) return ref;

    cairo_surface_t* surface = (cairo_surface_t*)g_tree_lookup(t, &scale);
    if (!surface) {
        double width = cairo_image_surface_get_width(ref) * scale;
        double height = cairo_image_surface_get_height(ref) * scale;
        surface = cairo_image_surface_create(cairo_image_surface_get_format(ref),
                width, height);
        cairo_t* cr = cairo_create(surface);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, ref, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        s = g_new(double, 1);
        *s = scale;
        g_tree_insert(t, s, surface);
        meta_verbose("%s: (%s) new scale %f", __func__, window->desc, scale);
    }
    
    return surface;
}

cairo_surface_t* deepin_window_surface_manager_get_combined_surface(
        MetaWindow* win1, MetaWindow* win2, int x, int y, double scale)
{
    cairo_surface_t* surface1 = deepin_window_surface_manager_get_surface(win1, 1.0);
    cairo_surface_t* surface2 = deepin_window_surface_manager_get_surface(win2, 1.0);

    return deepin_window_surface_manager_get_combined3(
            surface1, surface2, x, y, NULL, 0, 0, scale);
}

cairo_surface_t* deepin_window_surface_manager_get_combined3(
        cairo_surface_t* ref,
        cairo_surface_t* surface1, int x1, int y1, 
        cairo_surface_t* surface2, int x2, int y2, 
        double scale)
{
    if (!ref) return NULL;

    cairo_surface_t* ret = cairo_image_surface_create(
            cairo_image_surface_get_format(ref),
            cairo_image_surface_get_width(ref),
            cairo_image_surface_get_height(ref));

    cairo_t *cr = cairo_create(ret);
    if (scale < 1.0) cairo_scale(cr, scale, scale);

    cairo_set_source_surface(cr, ref, 0, 0);
    cairo_paint(cr);

    if (surface1) {
        cairo_set_source_surface(cr, surface1, x1, y1);
        cairo_paint(cr);
    }

    if (surface2) {
        cairo_set_source_surface(cr, surface2, x2, y2);
        cairo_paint(cr);
    }
    cairo_destroy(cr);

    return ret;
}

void deepin_window_surface_manager_remove_window(MetaWindow* window)
{
    if (!window) return;

    DeepinWindowSurfaceManager* self = deepin_window_surface_manager_get();
    if (g_hash_table_contains(self->priv->windows, window)) {
        meta_verbose("%s: %s", __func__, window->desc);
        g_hash_table_remove(self->priv->windows, window);
        g_signal_emit(self, signals[SIGNAL_SURFACE_INVALID], 0, window);
    }
}

void deepin_window_surface_manager_flush()
{
    DeepinWindowSurfaceManager* self = deepin_window_surface_manager_get();
    DeepinWindowSurfaceManagerPrivate* priv = self->priv;

    GList* l = g_hash_table_get_keys(self->priv->windows);

    for (GList* t = l; t; t = t->next) {
        MetaWindow* win = (MetaWindow*)t->data;
        g_hash_table_remove(priv->windows, win);
        g_signal_emit(self, signals[SIGNAL_SURFACE_INVALID], 0, win);
        deepin_window_surface_manager_get_surface((MetaWindow*)t->data, 1.0);
    }
    g_list_free(l);
}

static void on_window_removed(DeepinMessageHub* hub, MetaWindow* window, 
        gpointer data)
{
    deepin_window_surface_manager_remove_window(window);
}

static void on_window_damaged(DeepinMessageHub* hub, MetaWindow* window, 
        gpointer data)
{
    deepin_window_surface_manager_remove_window(window);
}

DeepinWindowSurfaceManager* deepin_window_surface_manager_get(void)
{
    if (!_the_manager) {
        _the_manager = (DeepinWindowSurfaceManager*)g_object_new(
                DEEPIN_TYPE_WINDOW_SURFACE_MANAGER, NULL);

        g_object_connect(G_OBJECT(deepin_message_hub_get()), 
                "signal::window-removed", on_window_removed, NULL,
                "signal::window-damaged", on_window_damaged, NULL,
                NULL);
    }
    return _the_manager;
}

