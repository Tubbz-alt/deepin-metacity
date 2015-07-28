/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2015 Sian Cao <yinshuiboy@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "deepin-design.h"
#include <math.h>

static struct {
    int entry_count, max_width;
    float box_width, box_height;
    float item_width, item_height;
    int items_each_row;
} cached = {0, 0, 0, 0, 0, 0, 0};

void calculate_preferred_size(gint entry_count, gint max_width,
        float* box_width, float* box_height, float* item_width, float* item_height,
        int* max_items_each_row)
{
    if (cached.entry_count == entry_count && cached.max_width == max_width) {
        if (box_width) *box_width = cached.box_width;
        if (box_height) *box_height = cached.box_height;
        if (item_width) *item_width = cached.item_width;
        if (item_height) *item_height = cached.item_height;
        if (max_items_each_row) *max_items_each_row = cached.items_each_row;
        return;
    }

    float bw, bh, iw, ih;
    gint cols;
    gboolean item_need_scale = FALSE;

    iw = SWITCHER_ITEM_PREFER_WIDTH;
    ih = SWITCHER_ITEM_PREFER_HEIGHT;

    cols = (int) ((max_width + SWITCHER_COLUMN_SPACING) / (SWITCHER_ITEM_PREFER_WIDTH + SWITCHER_COLUMN_SPACING));
    if (cols < SWITCHER_MIN_ITEMS_EACH_ROW && entry_count > cols ) {
        item_need_scale = TRUE;
        cols = MIN(SWITCHER_MIN_ITEMS_EACH_ROW, entry_count);
    }

    if (cols * SWITCHER_MAX_ROWS < entry_count) {
        cols = (int) ceilf((float) entry_count / SWITCHER_MAX_ROWS);
        item_need_scale = TRUE;
    }

    if (item_need_scale) {
        iw = (max_width + SWITCHER_COLUMN_SPACING) / cols - SWITCHER_COLUMN_SPACING;
        float item_scale = iw / SWITCHER_ITEM_PREFER_WIDTH;
        ih = SWITCHER_ITEM_PREFER_HEIGHT * item_scale;
    }

    if (entry_count < cols) {
        if (entry_count > 0) {
            bw = (iw + SWITCHER_COLUMN_SPACING) * entry_count - SWITCHER_COLUMN_SPACING;
        } else {
            g_assert(0);
            bw = 0;
        }
    } else {
        bw = (iw + SWITCHER_COLUMN_SPACING) * cols - SWITCHER_COLUMN_SPACING;
    }

    int rows = (entry_count + cols - 1) / cols;
    if (rows > 0) {
        bh = (ih + SWITCHER_ROW_SPACING) * rows - SWITCHER_ROW_SPACING;
    } else {
        g_assert(0);
        bh = 0;
    }

    bw += bw * 0.033, bh += bh * 0.033 + SWITCHER_ROW_SPACING;

    cached.box_width = bw;
    cached.box_height = bh;
    cached.item_width = iw;
    cached.item_height = ih;
    cached.items_each_row = cols;
    cached.entry_count = entry_count;
    cached.max_width = max_width;

    if (box_width) *box_width = cached.box_width;
    if (box_height) *box_height = cached.box_height;
    if (item_width) *item_width = cached.item_width;
    if (item_height) *item_height = cached.item_height;
    if (max_items_each_row) *max_items_each_row = cached.items_each_row;
}
