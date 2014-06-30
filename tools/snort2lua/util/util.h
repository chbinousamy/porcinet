/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2002-2013 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
// converter.h author Josh Rosenbaum <jorosenba@cisco.com>

#ifndef SNORT2LUA_UTIL_H
#define SNORT2LUA_UTIL_H


#include <string>
#include <vector>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <sstream>

struct ConvertMap;

namespace util
{

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
const ConvertMap* find_map(const std::vector<const ConvertMap*>, std::string keyword);

// trim from begining
std::string &ltrim(std::string &s);
// trim from end
std::string &rtrim(std::string &s);
// trim from both ends
std::string &trim(std::string &s);


// trim from start
inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

std::string get_rule_option_args(std::istringstream& data_stream);
// remove any ']]'  from this string.
std::string &sanitize_multi_line_string(std::string &s);
// find the location of the first space before max_str_lenght.
// if no space exists before max_str_length, return the first space
// after max_length. Otherwise, return std::string::npos
int get_substr_length(std::string s, int max_length);

} // namespace util

#endif
