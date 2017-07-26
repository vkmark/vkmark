/*
 * Copyright © 2017 Collabora Ltd.
 *
 * This file is part of vkmark.
 *
 * vkmark is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * vkmark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with vkmark. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Alexandros Frantzis <alexandros.frantzis@collabora.com>
 */

/* GIMP RGBA C-Source image dump (noise_rgba.c) */

static const struct {
  unsigned int  width;
  unsigned int  height;
  unsigned int  bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char pixel_data[8 * 8 * 4 + 1];
} noise_rgba_image = {
  8, 8, 4,
  "}\344\017@{\234\331\335\206\374\377\377A\372\021\377\377\262\377\000(\366\244"
  "sd\357\213\066\361\230\232\377\377t\000\377>!\310d\206\374\377\377\263\066M\305"
  "\363\231~\215+/\377\377\036\236Y\214\037\071$\033]M\226\227\370\213\234J\000\322"
  "\377\377\017\022\320vX\213\221\256\313\034!\324\000\377?\377\330,\205\004}\377\000"
  "\377X\062l\266h\017\315X\377\067[\377\377\000\000\377\000\232\000\377\000\377\000\377e\325"
  "\063\377\226\013p\014\340>\366\271Ol\350*\377\377\375\377\353a\062-\331\311\245"
  "\200\220\377\377\377\003\035\020Tp\016\063O\377\000\377\377\237\000\000\377\065\231n\260"
  "\000\032R\377\377\214\000\377Q\351\320)\014\362\036\321\374\220\200\237\377\377"
  "\377\377\000\377\040\377#\354\377\377|\246\376\026BC\276_\233R>\366I\377\377"
  "\377k\060u\335\313\361s\326/\000\377\377\225\241_\253\313\377\377\377\345I\250"
  "e\353ZG\210H\234\377m",
};
