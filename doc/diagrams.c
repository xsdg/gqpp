/*
 * Copyright (C) 2006 John Ellis
 * Copyright (C) 2008 - 2022 The Geeqie Team
 *
 * Author: Colin Clark
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * @brief Diagrams to be included in the Doxygen output
 * 
 * The contents are diagrams to be included in the Doxygen output.
 * The .c file extension is so that Doxygen will process it.
 */

/**
 * @page diagrams Diagrams
 * @section options_overview Options Overview
 * 
 * #_ConfOptions  #_LayoutOptions
 * 
 * @startuml
 * 
 * object options.h
 * object typedefs.h
 * 
 * options.h : ConfOptions
 * options.h : \n
 * options.h : Options applicable to **all** Layout Windows
 * options.h : These are in the <global> section of geeqierc.xml
 * options.h : Available to all modules via the global variable **options**
 * typedefs.h : LayoutOptions
 * typedefs.h : \n
 * typedefs.h : Options applicable to **each** Layout Window
 * typedefs.h : These are in the <layout> section of geeqierc.xml
 * typedefs.h : There is one <layout> section for each Layout Window displayed
 * typedefs.h : Available via **<layout_window>->options**
 * 
 * @enduml
 */

/**
 * @page diagrams Diagrams
 * @section options_diagrams_main Options - New Window From Main
 * #main  
 * #init_options  
 * #layout_new_from_default  
 * #load_config_from_file  
 * #load_options  
 * #setup_default_options
 * 
 * @startuml
 * group main.c
 * start
 * group options.c
 * : **init_options()**
 * 
 * Set **options** = ConfOptions from hard-coded init values;
 * end group
 * 
 * group options.c
 * : **setup_default_options()**
 * 
 * set hard-coded ConfOptions:
 * 
 * bookmarks:
 * * dot dir
 * * Home
 * * Desktop
 * * Collections
 * safe delete path
 * OSD template string
 * sidecar extensions
 * shell path and options
 * marks tooltips
 * help search engine;
 * end group
 * 
 * if (first entry
 * or
 * --new-instance) then (yes)
 * group options.c
 * : **load_options()**
 * ;
 * 
 * split
 * : GQ_SYSTEM_WIDE_DIR
 * /geeqierc.xml;
 * split again
 * : XDG_CONFIG_HOME
 * /geeqierc.xml;
 * split again
 * : HOME
 * /.geeqie/geeqierc.xml;
 * end split
 * 
 * group rcfile.c
 * : **load_config_from_file()**
 * 
 * set  **options** from file
 * and all <layout window>->options  in file;
 * end group
 * 
 * end group
 * 
 * if (broken config. file
 * or no config file
 * or no layout section loaded
 * (i.e. session not saved)) then (yes)
 * group layout.c
 * : **layout_new_from_default()**;
 * if (default.xml exists) then (yes)
 * : Load user-saved
 *  layout_window default options
 *  from default.xml file;
 * else (no)
 * : Load hard-coded
 *  layout_window default options;
 * endif
 * end group
 * endif
 * 
 * else (no)
 * : Send --new-window request to remote
 *  No return to this point
 *  This instance terminates;
 * stop
 * endif
 * 
 * : Enter gtk main loop;
 * 
 * end group
 * @enduml
 */

/**
 * @page diagrams Diagrams
 * @section options_diagrams_remote Options - New Window From Remote
 * #layout_new_from_default  
 * @startuml
 * 
 * group remote.c
 * start
 * group layout.c
 * : **layout_new_from_default()**;
 * if (default.xml exists) then (yes)
 * : Load user-saved
 *  layout_window default options
 *  from default.xml file;
 * else (no)
 * : Load hard-coded
 *  layout_window default options;
 * endif
 * end group
 * : set path from PWD;
 * @enduml
 */

/**
 * @page diagrams Diagrams
 * @section options_diagrams_menu Options - New Window From Menu
 * #layout_menu_new_window_cb  
 * #layout_menu_window_from_current_cb  
 * #layout_new_from_default  
 * @startuml
 * 
 * group layout_util.c
 * start
 * 
 * split
 * : default;
 * group layout.c
 * : **layout_new_from_default()**;
 * if (default.xml exists) then (yes)
 * : Load user-saved
 *  layout_window default options
 *  from default.xml file;
 * else (no)
 * : Load hard-coded
 *  layout_window default options;
 * endif
 * end group
 * 
 * split again
 * : from current
 * 
 * **layout_menu_window_from_current_cb()**
 * copy layout_window options
 * from current window;
 * 
 * split again
 * : named
 * 
 * **layout_menu_new_window_cb()**
 * load layout_window options
 * from saved xml file list;
 * end split
 * 
 * end group
 * @enduml
 */
  /**
  * @file
  * @ref options_overview Options Overview
  */
