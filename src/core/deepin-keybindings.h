/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* deepin custom keybindings */


/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef DEEPIN_META_KEYBINDINGS_H
#define DEEPIN_META_KEYBINDINGS_H

#include <display.h>
#include <screen.h>
#include "keybindings.h"
#include "window-private.h"

void deepin_init_custom_handlers(MetaDisplay* display);

/* when expose_mode == 3, xids is used */
void do_expose_windows(MetaDisplay *display, MetaScreen *screen,
        MetaWindow *window, guint32 timestamp, MetaKeyBinding *binding,
        int expose_mode, GVariant* xids);
#endif

