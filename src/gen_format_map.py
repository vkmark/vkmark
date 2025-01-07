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
import xml.etree.ElementTree as ET

infile = sys.argv[1]
outfile = sys.argv[2]

def vk_fmt_to_hpp(fmt):
    fmt = fmt.replace("VK_FORMAT_", "")
    ret = ""
    last_was_alpha = False
    for c in fmt:
        ret += c.lower() if last_was_alpha else c
        last_was_alpha = c.isalpha()
    return ret.replace("_", "")

with open(infile, 'r') as inf, open(outfile, 'w') as outf:
    print("#include <unordered_map>", file=outf);
    print("static std::unordered_map<std::string, vk::Format> format_map = {", file=outf)
    vk_xml = ET.fromstring(inf.read())
    for enum in vk_xml.findall(".//enums[@name='VkFormat']/enum"):
        fmt = vk_fmt_to_hpp(enum.get("name"))
        print('    {"%s", vk::Format::e%s},' % (fmt.upper(), fmt), file=outf)
    print("};", file=outf);
