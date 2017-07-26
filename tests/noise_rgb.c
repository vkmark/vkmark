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

/* GIMP RGBA C-Source image dump (noise_rgb.c) */

static const struct {
  unsigned int  width;
  unsigned int  height;
  unsigned int  bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char pixel_data[8 * 8 * 4 + 1];
} noise_rgb_image = {
  8, 8, 4,
  "v\377\000\377\226\377h\377\321\377\377\377\000\377\377\377\377>\377\377\377V"
  "/\377\000\347\377\377\366\250\377\377\377:\000\377\377\000\377\377C\377\377\377"
  "\377\377\213\377!\351\015\377\377\377\377\377\365\000\377\377\377E`\377\204"
  "\377\000\377\377\343\300\377\000q\302\377\377\000\377\377\355\000\377\377#\000\000\377"
  "]\377\336\377\377\260N\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\375\266o\377\377\377\241\377\000\350\000\377\000\377\245\377FeK\377\040\377\377"
  "\377_\000\335\377\000\377\000\377\330\340\377\377\377\000\322\377\337\000\377\377l"
  "\377\377\377\377\035r\377\257\377\000\377\322\000\377\377\377\000\377\377\377\377"
  "\377\377\000;.\377\377\215\000\377\377[\000\377\377\377\377\377\377\377\377\377"
  "\377\377\233\377\035\377\377\377\021\377\377\377\000\377\377\377[\000\377\377\377"
  "\377\374\377/\351\377\377\377\377\244\377\377\377\000\377o\002\377\377\377\377"
  "\227\377\377\377\377\377\000\000\377\377\371\377\377\377@\231y\377",
};
