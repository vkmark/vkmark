# Copyright © 2017 Collabora Ltd.
# 
# This file is part of vkmark.
# 
# vkmark is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation, either
# version 2.1 of the License, or (at your option) any later version.
# 
# vkmark is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with vkmark. If not, see <http://www.gnu.org/licenses/>.
# 
# Authors:
#   Alexandros Frantzis <alexandros.frantzis@collabora.com>

import sys

infile = sys.argv[1]
outfile = sys.argv[2]

in_formats = False

with open(infile, 'r') as inf, open(outfile, 'w') as outf:
    print("#include <unordered_map>", file=outf);
    print("static std::unordered_map<std::string, vk::Format> format_map = {", file=outf)
    for line in inf:
        line = line.strip()
        if in_formats:
            if line.startswith('e'):
                enum = line.split('=')[0].strip()
                print('    {"%s", vk::Format::%s},' % (enum[1:].upper(), enum), file=outf)
            if line.endswith('};'): 
                break
        elif line.startswith('enum class Format'):
            in_formats = True
    print("};", file=outf)
