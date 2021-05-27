[Log Window](#log-window)  
[GPL header](#gpl-header)  
[Git change log](#git-change-log)  
[Sources](#sources)  
[Documentation](#documentation)  

-----------


# <a name='log-window'>
#Log Window


`log_printf()`  
If the first word of the message is "error" or "warning" (case insensitive) the message will be color-coded appropriately.


`DEBUG_NAME(widget)`  
For use with the [GTKInspector](https://wiki.gnome.org/action/show/Projects/GTK/Inspector?action=show&redirect=Projects%2FGTK%2B%2FInspector) to provide a visual indication of where objects are declared.

Sample command line call:  
`GTK_DEBUG=interactive src/geeqie`

--------------------------------------------------------------------------------

# <a name='gpl-header'>
# GPL header

Include a header in every file, like this:  

    /** @file  
    * @brief Short description of this file.  
    * @author Author1  
    * @author Author2  
    *  
    * Optionally detailed description of this file    
    * on more lines.    
    */    

    /*  
    *  This file is a part of Geeqie project (http://www.geeqie.org/).  
    *  Copyright (C) 2008 - 2021 The Geeqie Team  
    *  
    *  This program is free software; you can redistribute it and/or modify it  
    *  under the terms of the GNU General Public License as published by the Free  
    *  Software Foundation; either version 2 of the License, or (at your option)  
    *  any later version.  
    *  
    *  This program is distributed in the hope that it will be useful, but WITHOUT  
    *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or  
    *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  
    *  more details.  
    */  

-------------

# <a name='git-change-log'>
#git change-log

If referencing a Geeqie GitHub issue, include the issue number in the summary line. Start with a short summary in the first line (without a dot at the end) followed by a empty line.

If referencing a Geeqie GitHub issue, include a hyperlink to the GitHub issue webpage in the message body. Use whole sentences beginning with Capital letter. For each modification use a new line. Or you can write the theme, colon and then every change on new line, begin with "- ".

See also [A Note About Git Commit Messages](http://www.tpope.net/node/106)

Example:

   `I did some bugfixes`

   `There was the bug that something was wrong. I fixed it.`

  ` Library:`  
   `- the interface was modified`  
  `- new functions were added`

Also please use your full name and a working e-mail address as author for any contribution.

--------------------------------------------------------------------------------

# <a name='sources'>
#Sources

Indentation: tabs at 4 spaces
	
	
Names:

- of variables & functions:	 small\_letters  
- of defines:		 CAPITAL\_LETTERS

Try to use explicit variable and function names.  
Try not to use macros.  
Use EITHER "struct foo" OR "foo"; never both


Conditions, cycles:  

    if (<cond>)
    	{
    	<command>;
    	...
    	<command>;
    	}
    else
    	{
    	<command>;
    	...
    	<command>;
    	}

    if (<cond_very_very_very_very_very_very_very_very_very_long> &&
    <cond2very_very_very_very_very_very_very_very_very_long>)
    <the_only_command>;

    switch (<var>)
    	{
    	case 0:
    		<command>;
		    <command>;
		    break;
    	case 1:
    		<command>; break;
    	}

    for (i = 0; i <= 10; i++)
    	{
    	<command>;
    	...
    	<command>;
    	}

Functions:

    gint bar(<var_def>, <var_def>, <var_def>)
    {
    	<command>;
    	...
    	<command>;

    	return 0; // i.e. SUCCESS; if error, you must return minus <err_no>
    }

    void bar2(void)
    {
    	<command>;
    	...
    	<command>;
    }

Pragma: (Indentation 2 spaces)

    #ifdef ENABLE_NLS
    #  undef _
    #  define _(String) (String)
    #endif /* ENABLE_NLS */

Headers:

    #ifndef _FILENAME_H


Use spaces around every operator (except ".", "->", "++" and "--").   
Unary operator '*' and '&' are missing the space from right, (and also unary '-').

As you can see above, parentheses are closed to inside, i.e. " (blah blah) "  
In "`function(<var>)`" there is no space before the '('.  
You MAY use more tabs/spaces than you OUGHT TO (according to this CodingStyle), if it makes your code nicer in being vertically indented.  
Variables declarations should be followed by a blank line and should always be at the start of the block.  


Use glib types when possible (ie. gint and gchar instead of int and char).
Use glib functions when possible (ie. `g_ascii_isspace()` instead of `isspace()`).
Check if used functions are not deprecated.

--------------------------------------------------------------------------------

# <a name='documentation'>
#Documentation

Use American, rather than British English, spelling. This will facilitate consistent
text searches. User text may be translated via the en_GB.po file.

To document the code use the following rules to allow extraction with doxygen.
Do not save with comments. Not all comments have to be doxygen comments.

- Use C comments in plain C files and use C++ comments in C++ files for one line
  comments.
- Use '/**' (note the two asterisks) to start comments to be extracted by
  doxygen and start every following line with " *".
- Use '@' to indicate doxygen keywords/commands (see below).
- Use the '@deprecated' command to tell if the function is subject to be deleted
  or to a  complete rewrite.

Example:

To document functions or big structures:

    /**
     * @brief This is a short description of the function.
     *
     * This function does ...
     *
     * @param x1 This is the first parameter named x1
     * @param y1 This is the second parameter named y1
     * @return What the function returns
     *    You can extend that return description (or anything else) by indenting the
     *    following lines until the next empty line or the next keyword/command.
     * @see Cross reference
     */

To document members of a structure that have to be documented (use it at least
for big structures) use the `/**<` format:  
`int counter; /**< This counter counts images */`

Document TODO or FIXME comments as:  

    /**  
    * @todo
   
or 
 
    /**  
    * @FIXME   

For further documentation about doxygen see the [Doxygen Manual](https://www.doxygen.nl/index.html).  
For the possible commands you may use, see [Doxygen Special Commands](https://www.doxygen.nl/manual/commands.html).

The file `./scripts/doxygen-help.sh` may be used to integrate access to the Doxygen files into a code editor.

The following environment variables may be set to personalize the Doxygen output:  
`DOCDIR=<output folder>`  
`SRCDIR=<the folder above ./src>`  
`PROJECT=`  
`VERSION=`  
`PLANTUML_JAR_PATH=`  
`INLINE_SOURCES=<YES|NO>`  
`STRIP_CODE_COMMENTS=<YES|NO>`  

Ref: [INLINE\_SOURCES](https://www.doxygen.nl/manual/config.html#cfg_inline_sources)  
Ref: [STRIP\_CODE\_COMMENTS](https://www.doxygen.nl/manual/config.html#cfg_strip_code_comments)

To include diagrams (if any) in the Doxygen output, the following are required to be installed. The installation process will vary between distributions:  
[The PlantUML jar](https://plantuml.com/download)  
`sudo apt install default-jre`  
`sudo apt install texlive-font-utils`

-------------

But in case just think about that the documentation is for other developers not
for the end user. So keep the focus.
