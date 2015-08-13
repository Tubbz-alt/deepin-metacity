/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * deepin-shadow-workspace.c
 * Copyright (C) 2015 Sian Cao <yinshuiboy@gmail.com>
 *
 * gtk-skeleton is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gtk-skeleton is distributed in the hope that it will be useful, but
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

#include "../core/workspace.h"
#include "deepin-cloned-widget.h"
#include "compositor.h"
#include "deepin-design.h"
#include "deepin-shadow-workspace.h"
#include "deepin-background-cache.h"

static const int SMOOTH_SCROLL_DELAY = 500;

struct _DeepinShadowWorkspacePrivate
{
    gint disposed: 1;
    gint dynamic: 1; /* if animatable */

    gint fixed_width, fixed_height;
    gdouble scale; 
    
    GPtrArray* clones;
    MetaWorkspace* workspace;
};


G_DEFINE_TYPE (DeepinShadowWorkspace, deepin_shadow_workspace, DEEPIN_TYPE_FIXED);

static void place_window(DeepinShadowWorkspace* self,
        MetaDeepinClonedWidget* clone, MetaRectangle rect)
{
    GtkRequisition req;
    gtk_widget_get_preferred_size(GTK_WIDGET(clone), &req, NULL);
    
    float fscale = (float)rect.width / req.width;
    g_debug("%s: bound: %d,%d,%d,%d, scale %f", __func__,
            rect.x, rect.y, rect.width, rect.height,
            fscale);

    deepin_fixed_move(DEEPIN_FIXED(self), GTK_WIDGET(clone),
            rect.x + req.width * fscale /2, rect.y + req.height * fscale /2);
    meta_deepin_cloned_widget_set_scale(clone, fscale, fscale);

    /*meta_deepin_cloned_widget_set_scale(clone, 1.0, 1.0);*/
    /*meta_deepin_cloned_widget_push_state(clone);*/
    /*meta_deepin_cloned_widget_set_scale(clone, fscale, fscale);*/
    /*meta_deepin_cloned_widget_pop_state(clone);*/
}

static const int GAPS = 10;
static const int MAX_TRANSLATIONS = 100000;
static const int ACCURACY = 20;

//some math utilities
static gboolean rect_is_overlapping_any(MetaRectangle rect, MetaRectangle* rects, gint n, MetaRectangle border)
{
    if (!meta_rectangle_contains_rect(&border, &rect))
        return TRUE;

    for (int i = 0; i < n; i++) {
        if (meta_rectangle_equal(&rects[i], &rect))
            continue;

        if (meta_rectangle_overlap(&rects[i], &rect))
            return TRUE;
    }

    return FALSE;
}

static MetaRectangle rect_adjusted(MetaRectangle rect, int dx1, int dy1, int dx2, int dy2)
{
    return (MetaRectangle){rect.x + dx1, rect.y + dy1, rect.width + (-dx1 + dx2), rect.height + (-dy1 + dy2)};
}

static GdkPoint rect_center(MetaRectangle rect)
{
    return (GdkPoint){rect.x + rect.width / 2, rect.y + rect.height / 2};
}

static void natural_placement (DeepinShadowWorkspace* self, MetaRectangle area)
{
    /*g_debug("%s: geom: %d,%d,%d,%d", __func__, area.x, area.y, area.width, area.height);*/
    DeepinShadowWorkspacePrivate* priv = self->priv;
    GPtrArray* clones = priv->clones;

    MetaRectangle bounds = {area.x, area.y, area.width, area.height};

    int direction = 0;
    int* directions = (int*)g_malloc(sizeof(int)*clones->len);
    MetaRectangle* rects = (MetaRectangle*)g_malloc(sizeof(MetaRectangle)*clones->len);

    for (int i = 0; i < clones->len; i++) {
        // save rectangles into 4-dimensional arrays representing two corners of the rectangular: [left_x, top_y, right_x, bottom_y]
        MetaRectangle rect;
        MetaDeepinClonedWidget* clone = g_ptr_array_index(clones, i);
        MetaWindow* win = meta_deepin_cloned_widget_get_window(clone);

        meta_window_get_outer_rect(win, &rect);
        rect = rect_adjusted(rect, -GAPS, -GAPS, GAPS, GAPS);
        rects[i] = rect;
        /*g_debug("%s: frame: %d,%d,%d,%d", __func__, rect.x, rect.y, rect.width, rect.height);*/

        meta_rectangle_union(&bounds, &rect, &bounds);

        // This is used when the window is on the edge of the screen to try to use as much screen real estate as possible.
        directions[i] = direction;
        direction++;
        if (direction == 4)
            direction = 0;
    }

    int loop_counter = 0;
    gboolean overlap = FALSE;
    do {
        overlap = FALSE;
        for (int i = 0; i < clones->len; i++) {
            for (int j = 0; j < clones->len; j++) {
                if (i == j)
                    continue;

                MetaRectangle rect = rects[i];
                MetaRectangle comp = rects[j];

                if (!meta_rectangle_overlap(&rect, &comp))
                    continue;

                loop_counter ++;
                overlap = TRUE;

                // Determine pushing direction
                GdkPoint i_center = rect_center (rect);
                GdkPoint j_center = rect_center (comp);
                GdkPoint diff = {j_center.x - i_center.x, j_center.y - i_center.y};

                // Prevent dividing by zero and non-movement
                if (diff.x == 0 && diff.y == 0)
                    diff.x = 1;

                // Approximate a vector of between 10px and 20px in magnitude in the same direction
                float length = sqrtf (diff.x * diff.x + diff.y * diff.y);
                diff.x = (int)floorf (diff.x * ACCURACY / length);
                diff.y = (int)floorf (diff.y * ACCURACY / length);
                // Move both windows apart
                rect.x += -diff.x;
                rect.y += -diff.y;
                comp.x += diff.x;
                comp.y += diff.y;

                // Try to keep the bounding rect the same aspect as the screen so that more
                // screen real estate is utilised. We do this by splitting the screen into nine
                // equal sections, if the window center is in any of the corner sections pull the
                // window towards the outer corner. If it is in any of the other edge sections
                // alternate between each corner on that edge. We don't want to determine it
                // randomly as it will not produce consistant locations when using the filter.
                // Only move one window so we don't cause large amounts of unnecessary zooming
                // in some situations. We need to do this even when expanding later just in case
                // all windows are the same size.
                // (We are using an old bounding rect for this, hopefully it doesn't matter)
                int x_section = (int)roundf ((rect.x - bounds.x) / (bounds.width / 3.0f));
                int y_section = (int)roundf ((comp.y - bounds.y) / (bounds.height / 3.0f));

                i_center = rect_center (rect);
                diff.x = 0;
                diff.y = 0;
                if (x_section != 1 || y_section != 1) { // Remove this if you want the center to pull as well
                    if (x_section == 1)
                        x_section = (directions[i] / 2 == 1 ? 2 : 0);
                    if (y_section == 1)
                        y_section = (directions[i] % 2 == 1 ? 2 : 0);
                }
                if (x_section == 0 && y_section == 0) {
                    diff.x = bounds.x - i_center.x;
                    diff.y = bounds.y - i_center.y;
                }
                if (x_section == 2 && y_section == 0) {
                    diff.x = bounds.x + bounds.width - i_center.x;
                    diff.y = bounds.y - i_center.y;
                }
                if (x_section == 2 && y_section == 2) {
                    diff.x = bounds.x + bounds.width - i_center.x;
                    diff.y = bounds.y + bounds.height - i_center.y;
                }
                if (x_section == 0 && y_section == 2) {
                    diff.x = bounds.x - i_center.x;
                    diff.y = bounds.y + bounds.height - i_center.y;
                }
                if (diff.x != 0 || diff.y != 0) {
                    length = sqrtf (diff.x * diff.x + diff.y * diff.y);
                    diff.x *= (int)floorf (ACCURACY / length / 2.0f);
                    diff.y *= (int)floorf (ACCURACY / length / 2.0f);
                    rect.x += diff.x;
                    rect.y += diff.y;
                }

                // Update bounding rect
                meta_rectangle_union(&bounds, &rect, &bounds);
                meta_rectangle_union(&bounds, &comp, &bounds);

                //we took copies from the rects from our list so we need to reassign them
                rects[i] = rect;
                rects[j] = comp;
            }
        }
    } while (overlap && loop_counter < MAX_TRANSLATIONS);

    // Work out scaling by getting the most top-left and most bottom-right window coords.
    float scale = fminf (fminf (area.width / (float)bounds.width,
                area.height / (float)bounds.height), 1.0f);

    // Make bounding rect fill the screen size for later steps
    bounds.x = (int)floorf (bounds.x - (area.width - bounds.width * scale) / 2);
    bounds.y = (int)floorf (bounds.y - (area.height - bounds.height * scale) / 2);
    bounds.width = (int)floorf (area.width / scale);
    bounds.height = (int)floorf (area.height / scale);

    // Move all windows back onto the screen and set their scale
    int index = 0;
    for (; index < clones->len; index++) {
        MetaRectangle rect = rects[index];
        rects[index] = (MetaRectangle){
            (int)floorf ((rect.x - bounds.x) * scale + area.x),
            (int)floorf ((rect.y - bounds.y) * scale + area.y),
            (int)floorf (rect.width * scale),
            (int)floorf (rect.height * scale)
        };
    }

    // fill gaps by enlarging windows
    gboolean moved = FALSE;
    MetaRectangle border = area;
    do {
        moved = FALSE;

        index = 0;
        for (; index < clones->len; index++) {
            MetaRectangle rect = rects[index];

            int width_diff = ACCURACY;
            int height_diff = (int)floorf ((((rect.width + width_diff) - rect.height) /
                        (float)rect.width) * rect.height);
            int x_diff = width_diff / 2;
            int y_diff = height_diff / 2;

            //top right
            MetaRectangle old = rect;
            rect = (MetaRectangle){ rect.x + x_diff, rect.y - y_diff - height_diff, rect.width + width_diff, rect.height + width_diff };
            if (rect_is_overlapping_any (rect, rects, clones->len, border))
                rect = old;
            else moved = TRUE;

            //bottom right
            old = rect;
            rect = (MetaRectangle){rect.x + x_diff, rect.y + y_diff, rect.width + width_diff, rect.height + width_diff};
            if (rect_is_overlapping_any (rect, rects, clones->len, border))
                rect = old;
            else moved = TRUE;

            //bottom left
            old = rect;
            rect = (MetaRectangle){rect.x - x_diff, rect.y + y_diff, rect.width + width_diff, rect.height + width_diff};
            if (rect_is_overlapping_any (rect, rects, clones->len, border))
                rect = old;
            else moved = TRUE;

            //top left
            old = rect;
            rect = (MetaRectangle){rect.x - x_diff, rect.y - y_diff - height_diff, rect.width + width_diff, rect.height + width_diff};
            if (rect_is_overlapping_any (rect, rects, clones->len, border))
                rect = old;
            else moved = TRUE;

            rects[index] = rect;
        }
    } while (moved);

    index = 0;
    for (; index < clones->len; index++) {
        MetaRectangle rect = rects[index];

        MetaDeepinClonedWidget* clone = (MetaDeepinClonedWidget*)g_ptr_array_index(clones, index);
        MetaWindow* window = meta_deepin_cloned_widget_get_window(clone);

        MetaRectangle window_rect;
        meta_window_get_outer_rect(window, &window_rect);


        rect = rect_adjusted(rect, GAPS, GAPS, -GAPS, -GAPS);
        scale = rect.width / (float)window_rect.width;

        if (scale > 2.0 || (scale > 1.0 && (window_rect.width > 300 || window_rect.height > 300))) {
            scale = (window_rect.width > 300 || window_rect.height > 300) ? 1.0f : 2.0f;
            rect = (MetaRectangle){rect_center (rect).x - (int)floorf (window_rect.width * scale) / 2,
                rect_center (rect).y - (int)floorf (window_rect.height * scale) / 2,
                (int)floorf (window_rect.width * scale),
                (int)floorf (window_rect.height * scale)};
        }

        place_window(self, clone, rect);
    }

    g_free(directions);
    g_free(rects);
}

/*static gint window_compare(gconstpointer a, gconstpointer b)*/
/*{*/
    /*MetaDeepinClonedWidget* aa = *(MetaDeepinClonedWidget**)a;*/
    /*MetaDeepinClonedWidget* bb = *(MetaDeepinClonedWidget**)b;*/

    /*MetaWindow* w1 = meta_deepin_cloned_widget_get_window(aa);*/
    /*MetaWindow* w2 = meta_deepin_cloned_widget_get_window(bb);*/
    /*return meta_window_get_stable_sequence(w1) - meta_window_get_stable_sequence(w2);*/
/*}*/

static int padding_top  = 12;
static int padding_left  = 12;
static int padding_right  = 12;
static int padding_bottom  = 12;
static void calculate_places(DeepinShadowWorkspace* self)
{
    DeepinShadowWorkspacePrivate* priv = self->priv;
    GPtrArray* clones = priv->clones;
    if (priv->clones->len) {
        /*g_ptr_array_sort(clones, window_compare);*/

        MetaRectangle area = {
            padding_top, padding_left, 
            priv->fixed_width - padding_left - padding_right,
            priv->fixed_height - padding_top - padding_bottom
        };

        natural_placement(self, area);
    }
}


static void deepin_shadow_workspace_init (DeepinShadowWorkspace *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DEEPIN_TYPE_SHADOW_WORKSPACE, DeepinShadowWorkspacePrivate);

    self->priv->scale = 1.0;
}

static void deepin_shadow_workspace_finalize (GObject *object)
{
    DeepinShadowWorkspacePrivate* priv = DEEPIN_SHADOW_WORKSPACE(object)->priv;
    if (priv->clones) {
        g_ptr_array_free(priv->clones, FALSE);
    }

	G_OBJECT_CLASS (deepin_shadow_workspace_parent_class)->finalize (object);
}

static void deepin_shadow_workspace_get_preferred_width (GtkWidget *widget,
        gint *minimum, gint *natural)
{
  DeepinShadowWorkspace *self = DEEPIN_SHADOW_WORKSPACE (widget);

  *minimum = *natural = self->priv->fixed_width;
}

static void deepin_shadow_workspace_get_preferred_height (GtkWidget *widget,
        gint *minimum, gint *natural)
{
  DeepinShadowWorkspace *self = DEEPIN_SHADOW_WORKSPACE (widget);

  *minimum = *natural = self->priv->fixed_height;
}

/* FIXME: no need to draw when do moving animation, just a snapshot */
static gboolean deepin_shadow_workspace_draw (GtkWidget *widget,
        cairo_t *cr)
{
  DeepinShadowWorkspace *fixed = DEEPIN_SHADOW_WORKSPACE (widget);
  DeepinShadowWorkspacePrivate *priv = fixed->priv;

  cairo_set_source_surface(cr,
          deepin_background_cache_get_surface(priv->scale), 0, 0);
  cairo_paint(cr);

  return GTK_WIDGET_CLASS(deepin_shadow_workspace_parent_class)->draw(widget, cr);
}

static void deepin_shadow_workspace_class_init (DeepinShadowWorkspaceClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass* widget_class = (GtkWidgetClass*) klass;

	g_type_class_add_private (klass, sizeof (DeepinShadowWorkspacePrivate));
    widget_class->get_preferred_width = deepin_shadow_workspace_get_preferred_width;
    widget_class->get_preferred_height = deepin_shadow_workspace_get_preferred_height;
    widget_class->draw = deepin_shadow_workspace_draw;

	object_class->finalize = deepin_shadow_workspace_finalize;
}

void deepin_shadow_workspace_populate(DeepinShadowWorkspace* self,
        MetaWorkspace* ws)
{
    DeepinShadowWorkspacePrivate* priv = self->priv;
    priv->workspace = ws;

    if (!priv->clones) priv->clones = g_ptr_array_new();

    GList* ls = meta_stack_list_windows(ws->screen->stack, ws);
    GList* l = ls;
    while (l) {
        MetaWindow* win = (MetaWindow*)l->data;
        if (win->type == META_WINDOW_NORMAL) {
            GtkWidget* widget = meta_deepin_cloned_widget_new(win);
            g_ptr_array_add(priv->clones, widget);

            MetaRectangle r;
            meta_window_get_outer_rect(win, &r);
            gint w = r.width * priv->scale, h = r.height * priv->scale;
            meta_deepin_cloned_widget_set_size(META_DEEPIN_CLONED_WIDGET(widget),
                    w, h);

            deepin_fixed_put(DEEPIN_FIXED(self), widget,
                    r.x * priv->scale + w/2,
                    r.y * priv->scale + h/2);
        }


        l = l->next;
    }
    g_list_free(ls);

    gtk_widget_queue_resize(GTK_WIDGET(self));
}

static gboolean on_idle(DeepinShadowWorkspace* self)
{
    calculate_places(self);
    return G_SOURCE_REMOVE;
}

static void on_deepin_shadow_workspace_show(DeepinShadowWorkspace* self, gpointer data)
{
    g_idle_add(on_idle, self);
}

GtkWidget* deepin_shadow_workspace_new(void)
{
    DeepinShadowWorkspace* self = (DeepinShadowWorkspace*)g_object_new(
            DEEPIN_TYPE_SHADOW_WORKSPACE, NULL);

    GdkScreen* screen = gdk_screen_get_default();
    self->priv->fixed_width = gdk_screen_get_width(screen);
    self->priv->fixed_height = gdk_screen_get_height(screen);
    self->priv->scale = 1.0;

    g_signal_connect(G_OBJECT(self), "show",
            (GCallback)on_deepin_shadow_workspace_show, NULL);

    return (GtkWidget*)self;
}

void deepin_shadow_workspace_set_scale(DeepinShadowWorkspace* self, gdouble s)
{
    DeepinShadowWorkspacePrivate* priv = self->priv;

    MetaDisplay* display = meta_get_display();
    MetaRectangle r = display->active_screen->rect;

    priv->scale = s;
    priv->fixed_width = r.width * s;
    priv->fixed_height = r.height * s;

    gtk_widget_queue_resize(GTK_WIDGET(self));
}

