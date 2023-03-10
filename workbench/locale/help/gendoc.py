#!/usr/bin/env python3
# -*- coding: iso-8859-1 -*-
# Copyright (C) 2013-2022, The AROS Development Team. All rights reserved.
# $Id$

"""Documentation to guide.

Extract documentation from C source files and create guide files

Usage: python gendoc.py <sourcedir> <targetdir>

<sourcedir> will be scanned recursively for C source files.
<targetdir> where to place the guide. Directory will be created
            if it doesn't exist.
"""

import re
import os
import sys
import datetime

# Regex for whole autodoc block (without surrounding comments)
AD_REGX = re.compile(r"""
^/\*{6,}
(
.*?
^\s{4,4}NAME
.*?
)
^\*{7,}/
""", re.VERBOSE | re.MULTILINE | re.DOTALL)

# Regex for a title
TITLES_REGX = re.compile(r"""
^\s{4,4}
(NAME|FORMAT|SYNOPSIS|LOCATION|FUNCTION|INPUTS|TAGS|RESULT|EXAMPLE|NOTES|BUGS|SEE\ ALSO|INTERNALS|HISTORY|TEMPLATE)$
""", re.VERBOSE)


def parsedoc(filename, targetdir):
    # print "reading " + filename
    blocks = {}
    filehandle = open(filename, encoding="ISO-8859-15")
    content = filehandle.read()
    doc = AD_REGX.search(content)
    current_title = None
    if doc:
        for line in doc.group(1).splitlines():
            match = TITLES_REGX.match(line)
            if match:
                current_title = match.group(1)
                blocks[current_title] = ""
            elif current_title:
                blocks[current_title] += line.expandtabs()[4:] + "\n"

        # check for empty chapters, because we don't want to print them
        for title, content in blocks.items():
            if content.strip() == "":
                blocks[title] = ""

    filehandle.close()

    if "NAME" in blocks:
        # get docname
        docname = blocks["NAME"].split()[0]
        if docname == "":
            raise ValueError("docname is empty")

        docfilename = docname + ".guide"
        today = datetime.date.today()
        filehandle = open(os.path.join(targetdir, docfilename), "w", encoding="ISO-8859-15")

        # The titles we want to printed
        shell_titles = ("Name", "Format", "Template", "Synopsis", "Location", "Function",
                        "Inputs", "Tags", "Result", "Example", "Notes", "Bugs", "See also")

        filehandle.write("@DATABASE %s\n\n" % (docfilename))
        filehandle.write("@$VER: %s 1.0 (%d.%d.%d)\n" % (docfilename, today.day, today.month, today.year))
        filehandle.write("@(C) Copyright (C) %d, The AROS Development Team. All rights reserved.\n" % (today.year))
        filehandle.write("@MASTER %s\n\n" %(filename))
        filehandle.write("@NODE MAIN \"%s\"\n\n" % (docname))

        for title in shell_titles:
            title_key = title.upper()
            if title_key in blocks and blocks[title_key] != "":
                filehandle.write("@{B}" + title + "@{UB}\n")
                filehandle.write(blocks[title_key])
                filehandle.write("\n")

        filehandle.write('@TOC "HELP:English/Index.guide/MAIN"\n')
        filehandle.write("@ENDNODE\n")

        filehandle.close()

###############################################################################

def main():
    sourcedir = sys.argv[1]
    targetdir = sys.argv[2]

    if not os.path.exists(targetdir):
        os.mkdir(targetdir)

    for root, dirs, files in os.walk(sourcedir):
        for filename in files:
            if len(filename) > 2 and filename[-2:] == ".c":
                parsedoc(os.path.join(root, filename), targetdir)
        if '.svn' in dirs:
            dirs.remove('.svn')


if __name__ == "__main__":
    main()
