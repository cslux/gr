/* gr-image-editor.h:
 *
 * Copyright (C) 2016 Matthias Clasen <mclasen@redhat.com>
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct {
        char *path;
        int angle;
        gboolean dark_text;
} GrRotatedImage;

GArray *gr_rotated_image_array_new (void);

#define GR_TYPE_IMAGE_EDITOR (gr_image_editor_get_type())

G_DECLARE_FINAL_TYPE (GrImageEditor, gr_image_editor, GR, IMAGE_EDITOR, GtkBox)

GrImageEditor  *gr_image_editor_new          (void);

void            gr_image_editor_add_image    (GrImageEditor *editor);
void            gr_image_editor_remove_image (GrImageEditor *editor);
void            gr_image_editor_rotate_image (GrImageEditor *editor,
                                              gint           angle);

G_END_DECLS