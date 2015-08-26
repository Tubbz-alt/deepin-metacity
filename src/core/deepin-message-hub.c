/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * deepin-message-hub.c
 * Copyright (C) 2015 Sian Cao <yinshuiboy@gmail.com>
 *
 * deepin metacity is free software: you can redistribute it and/or modify it
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

#include "deepin-message-hub.h"
#include "window-private.h"

static DeepinMessageHub* _the_hub = NULL;

struct _DeepinMessageHubPrivate
{
    guint diposed: 1;
};


enum
{
    SIGNAL_WINDOW_REMOVED,
    SIGNAL_WINDOW_ADDED,
    SIGNAL_DESKTOP_CHANGED,
    SIGNAL_SCREEN_RESIZED,

    LAST_SIGNAL
};


static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (DeepinMessageHub, deepin_message_hub, G_TYPE_OBJECT);

static void deepin_message_hub_init (DeepinMessageHub *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DEEPIN_TYPE_MESSAGE_HUB, DeepinMessageHubPrivate);

    /* TODO: Add initialization code here */
}

static void deepin_message_hub_finalize (GObject *object)
{
    /* TODO: Add deinitalization code here */

    G_OBJECT_CLASS (deepin_message_hub_parent_class)->finalize (object);
}

void deepin_message_hub_window_added(MetaWindow* window)
{
    g_message("%s: %s", __func__, window->desc);
    g_signal_emit(deepin_message_hub_get(), signals[SIGNAL_WINDOW_ADDED], 0, window);
}

void deepin_message_hub_window_removed(MetaWindow* window)
{
    g_message("%s: %s", __func__, window->desc);
    g_signal_emit(deepin_message_hub_get(), signals[SIGNAL_WINDOW_REMOVED], 0, window);
}

void deepin_message_hub_desktop_changed()
{
    g_message("%s", __func__);
    g_signal_emit(deepin_message_hub_get(), signals[SIGNAL_DESKTOP_CHANGED], 0);
}

void deepin_message_hub_screen_resized(MetaScreen* screen)
{
    g_message("%s", __func__);
    g_signal_emit(deepin_message_hub_get(), signals[SIGNAL_SCREEN_RESIZED], 0, screen);
}

static void deepin_message_hub_class_init (DeepinMessageHubClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (DeepinMessageHubPrivate));

    object_class->finalize = deepin_message_hub_finalize;

    signals[SIGNAL_WINDOW_REMOVED] = g_signal_new ("window-removed",
            G_OBJECT_CLASS_TYPE (klass),
            G_SIGNAL_RUN_LAST, 0,
            NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[SIGNAL_WINDOW_ADDED] = g_signal_new ("window-added",
            G_OBJECT_CLASS_TYPE (klass),
            G_SIGNAL_RUN_LAST, 0,
            NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[SIGNAL_DESKTOP_CHANGED] = g_signal_new ("desktop-changed",
            G_OBJECT_CLASS_TYPE (klass),
            G_SIGNAL_RUN_LAST, 0,
            NULL, NULL, NULL,
            G_TYPE_NONE, 0);

    signals[SIGNAL_SCREEN_RESIZED] = g_signal_new ("screen-resized",
            G_OBJECT_CLASS_TYPE (klass),
            G_SIGNAL_RUN_LAST, 0,
            NULL, NULL, NULL,
            G_TYPE_NONE, 1, G_TYPE_POINTER);
}

DeepinMessageHub* deepin_message_hub_get()
{
    if (!_the_hub) {
        _the_hub = (DeepinMessageHub*)g_object_new(DEEPIN_TYPE_MESSAGE_HUB, NULL);
    }
    return _the_hub;
}

