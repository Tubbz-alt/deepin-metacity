/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */

/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef _DEEPIN_NAME_ENTRY_H_
#define _DEEPIN_NAME_ENTRY_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DEEPIN_TYPE_NAME_ENTRY             (deepin_name_entry_get_type ())
#define DEEPIN_NAME_ENTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEEPIN_TYPE_NAME_ENTRY, DeepinNameEntry))
#define DEEPIN_NAME_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DEEPIN_TYPE_NAME_ENTRY, DeepinNameEntryClass))
#define DEEPIN_IS_NAME_ENTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEEPIN_TYPE_NAME_ENTRY))
#define DEEPIN_IS_NAME_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DEEPIN_TYPE_NAME_ENTRY))
#define DEEPIN_NAME_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DEEPIN_TYPE_NAME_ENTRY, DeepinNameEntryClass))

typedef struct _DeepinNameEntryClass DeepinNameEntryClass;
typedef struct _DeepinNameEntry DeepinNameEntry;
typedef struct _DeepinNameEntryPrivate DeepinNameEntryPrivate;


struct _DeepinNameEntryClass
{
	GtkEntryClass parent_class;
};

struct _DeepinNameEntry
{
	GtkEntry parent_instance;

	DeepinNameEntryPrivate *priv;
};

GType deepin_name_entry_get_type (void) G_GNUC_CONST;
GtkWidget* deepin_name_entry_new(int recommend_width);

G_END_DECLS

#endif /* _DEEPIN_NAME_ENTRY_H_ */

