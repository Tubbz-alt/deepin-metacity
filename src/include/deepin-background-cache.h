/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */

/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef _DEEPIN_BACKGROUND_CACHE_H_
#define _DEEPIN_BACKGROUND_CACHE_H_

#include <glib-object.h>
#include <cairo.h>

G_BEGIN_DECLS

#define DEEPIN_TYPE_BACKGROUND_CACHE             (deepin_background_cache_get_type ())
#define DEEPIN_BACKGROUND_CACHE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEEPIN_TYPE_BACKGROUND_CACHE, DeepinBackgroundCache))
#define DEEPIN_BACKGROUND_CACHE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DEEPIN_TYPE_BACKGROUND_CACHE, DeepinBackgroundCacheClass))
#define DEEPIN_IS_BACKGROUND_CACHE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEEPIN_TYPE_BACKGROUND_CACHE))
#define DEEPIN_IS_BACKGROUND_CACHE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DEEPIN_TYPE_BACKGROUND_CACHE))
#define DEEPIN_BACKGROUND_CACHE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DEEPIN_TYPE_BACKGROUND_CACHE, DeepinBackgroundCacheClass))

typedef struct _DeepinBackgroundCacheClass DeepinBackgroundCacheClass;
typedef struct _DeepinBackgroundCache DeepinBackgroundCache;
typedef struct _DeepinBackgroundCachePrivate DeepinBackgroundCachePrivate;


struct _DeepinBackgroundCacheClass
{
	GObjectClass parent_class;
};

struct _DeepinBackgroundCache
{
	GObject parent_instance;

	DeepinBackgroundCachePrivate *priv;
};

GType deepin_background_cache_get_type (void) G_GNUC_CONST;

// skeleton
DeepinBackgroundCache* deepin_get_background();
cairo_surface_t* deepin_background_cache_get_surface(gint monitor, gint workspace, double scale);
cairo_surface_t* deepin_background_cache_get_default(double scale);
void deepin_change_background (int index, const char* uri);
char* deepin_get_background_uri (int index);
// set current background transiently, do not write back into settings 
void deepin_change_background_transient (int index, const char* uri);
void deepin_background_cache_request_new_default_uri();

G_END_DECLS

#endif /* _DEEPIN_BACKGROUND_CACHE_H_ */

