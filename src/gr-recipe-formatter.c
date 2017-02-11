/* gr-recipe-formatter.c:
 *
 * Copyright (C) 2017 Matthias Clasen <mclasen@redhat.com>
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

#include "config.h"

#include <stdlib.h>
#include <glib/gi18n.h>

#include "gr-recipe-formatter.h"
#include "gr-ingredients-list.h"
#include "gr-images.h"
#include "gr-chef.h"
#include "gr-recipe-store.h"
#include "gr-app.h"
#include "gr-utils.h"

char *
gr_recipe_format (GrRecipe *recipe)
{
        GString *s;
        g_autoptr(GrChef) chef = NULL;
        GrRecipeStore *store;
        g_autoptr(GrIngredientsList) ingredients = NULL;
        g_autofree char **segs = NULL;
        g_auto(GStrv) ings = NULL;
        int i, j;
        int length;
        g_autoptr(GPtrArray) steps = NULL;

        store = gr_app_get_recipe_store (GR_APP (g_application_get_default ()));
        chef = gr_recipe_store_get_chef (store, gr_recipe_get_author (recipe));

        s = g_string_new ("");

        g_string_append_printf (s, "*** %s ***\n", gr_recipe_get_translated_name (recipe));
        g_string_append (s, "\n");
        g_string_append_printf (s, "%s %s\n", _("Author:"), gr_chef_get_fullname (chef));
        g_string_append_printf (s, "%s %s\n", _("Preparation:"), gr_recipe_get_prep_time (recipe));
        g_string_append_printf (s, "%s %s\n", _("Cooking:"), gr_recipe_get_cook_time (recipe));
        g_string_append_printf (s, "%s %d\n", _("Serves:"), gr_recipe_get_serves (recipe));
        g_string_append (s, "\n");
        g_string_append_printf (s, "%s\n", gr_recipe_get_translated_description (recipe));

        ingredients = gr_ingredients_list_new (gr_recipe_get_ingredients (recipe));
        segs = gr_ingredients_list_get_segments (ingredients);
        for (j = 0; segs[j]; j++) {
                g_string_append (s, "\n");
                if (segs[j][0] != 0)
                        g_string_append_printf (s, "* %s *\n", segs[j]);
                else
                        g_string_append_printf (s, "* %s *\n", _("Ingredients"));

                ings = gr_ingredients_list_get_ingredients (ingredients, segs[j]);
                length = g_strv_length (ings);
                for (i = 0; i < length; i++) {
                        char *unit;

                        g_string_append (s, "\n");
                        unit = gr_ingredients_list_scale_unit (ingredients, segs[j], ings[i], 1, 1);
                        g_string_append (s, unit);
                        g_free (unit);
                        g_string_append (s, " ");
                        g_string_append (s, ings[i]);
                }

                g_string_append (s, "\n");
        }

        g_string_append (s, "\n");
        g_string_append_printf (s, "* %s *\n", _("Directions"));
        g_string_append (s, "\n");

        steps = gr_recipe_parse_instructions (gr_recipe_get_translated_instructions (recipe));
        for (i = 0; i < steps->len; i++) {
                GrRecipeStep *step = g_ptr_array_index (steps, i);
                g_string_append (s, step->text);
                g_string_append (s, "\n\n");
        }

        return g_string_free (s, FALSE);
}

static GrRecipeStep *
recipe_step_new (const char *text,
                 int         image,
                 guint64     timer)
{
        GrRecipeStep *step;

        step = g_new (GrRecipeStep, 1);
        step->text = g_strdup (text);
        step->image = image;
        step->timer = timer;

        return step;
}

static void
recipe_step_free (gpointer data)
{
        GrRecipeStep *d = data;
        g_free (d->text);
        g_free (d);
}

GPtrArray *
gr_recipe_parse_instructions (const char *instructions)
{
        GPtrArray *step_array;
        g_auto(GStrv) steps = NULL;
        int i;

        step_array = g_ptr_array_new_with_free_func (recipe_step_free);

        steps = g_strsplit (instructions, "\n\n", -1);

        for (i = 0; steps[i]; i++) {
                const char *p, *q;
                int image = -1;
                guint64 timer = 0;
                g_autofree char *step = NULL;

                p = strstr (steps[i], "[image:");
                if (p) {
                        g_autofree char *prefix = NULL;

                        image = atoi (p + strlen ("[image:"));

                        prefix = g_strndup (steps[i], p - steps[i]);
                        q = strstr (p, "]");
                        step = g_strconcat (prefix, q + 1, NULL);
                }

                p = strstr (steps[i], "[timer:");
                if (p) {
                        g_autofree char *s = NULL;
                        g_auto(GStrv) strv = NULL;
                        g_autofree char *prefix = NULL;

                        q = strstr (p, "]");
                        s = strndup (p + strlen ("[timer:"), q - (p + strlen ("[timer:")) - 1);
                        strv = g_strsplit (s, ":", -1);
                        if (g_strv_length (strv) == 2) {
                                timer = G_TIME_SPAN_MINUTE * atoi (strv[0]) +
                                        G_TIME_SPAN_SECOND * atoi (strv[1]);
                        }
                        else if (g_strv_length (strv) == 3) {
                                timer = G_TIME_SPAN_HOUR * atoi (strv[0]) +
                                        G_TIME_SPAN_MINUTE * atoi (strv[1]) +
                                        G_TIME_SPAN_SECOND * atoi (strv[2]);
                        }
                        else {
                                g_message ("Could not parse timer field %s; ignoring", s);
                        }

                        prefix = g_strndup (steps[i], p - steps[i]);
                        q = strstr (p, "]");
                        step = g_strconcat (prefix, q + 1, NULL);
                }

                g_ptr_array_add (step_array, recipe_step_new (step ? step : steps[i], image, timer));
        }

        return step_array;
}