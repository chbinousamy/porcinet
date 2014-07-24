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
// dt_comment.h author Josh Rosenbaum <jrosenba@cisco.com>

#ifndef DT_COMMENT_H
#define DT_COMMENT_H

#include <string>
#include <vector>
#include <iostream>


static const std::string start_comments =
    "\nCOMMENTS:\n"
    "    these line were commented "
    "in the configuration file.";

static const std::string start_errors =
    "\nERRORS:\n"
    "    all of these occured during the attempted conversion:\n\n";

static const std::string start_bad_rules =
    "\nFAILED RULES CONVERSIONS:\n"
    "    These rules has invalid rule options\n\n";

class Comments
{
public:

    enum class CommentType
    {
        SINGLE_LINE,
        MULTI_LINE
    };


    Comments(CommentType);
    Comments(int depth, CommentType);
    Comments(std::string name, int depth, CommentType);
    virtual ~Comments();

    void add_text(std::string new_text);
    // insert this string before the first lexigraphically larger string.
    // will not add duplicates.
    void add_sorted_text(std::string new_text);
    bool empty();

    // overloading operators
    friend std::ostream &operator<<( std::ostream&, const Comments &);

private:
    std::vector<std::string> comment;
    int depth;
    bool prev_empty;
    bool header;  // true if a string was passed into constructor
    enum CommentType type;
    const std::size_t max_line_length = 80;
    const std::string comment_line = "--";
    const std::string start_multi_com = "--[[";
    const std::string end_multi_com = "--]]";
};


#endif
