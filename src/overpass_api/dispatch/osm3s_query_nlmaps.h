#ifndef DE__OSM3S___OVERPASS_API__DISPATCH__OSM3SQUERYNLMAPS_H
#define	DE__OSM3S___OVERPASS_API__DISPATCH__OSM3SQUERYNLMAPS_H

#include "../../expat/expat_justparse_interface.h"
#include "../../template_db/dispatcher.h"
#include "../frontend/console_output.h"
#include "../frontend/user_interface.h"
#include "../frontend/web_output.h"
#include "../osm-backend/clone_database.h"
#include "../statements/osm_script.h"
#include "../statements/statement.h"
#include "../nlmaps/nlmaps_query.h"
#include "resource_manager.h"
#include "scripting_core.h"

#include "../statements/pivot.h"
#include "../statements/area_query.h"
#include "../statements/around.h"
#include "../statements/bbox_query.h"
#include "../statements/coord_query.h"
#include "../statements/difference.h"
#include "../statements/foreach.h"
#include "../statements/id_query.h"
#include "../statements/item.h"
#include "../statements/make_area.h"
#include "../statements/map_to_area.h"
#include "../statements/newer.h"
#include "../statements/polygon_query.h"
#include "../statements/print.h"
#include "../statements/osm_script.h"
#include "../statements/query.h"
#include "../statements/recurse.h"
#include "../statements/union.h"
#include "../statements/user.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int execute_command(string db_dir, string clone_db_dir, uint log_level, Debug_Level debug_level, int area_level, bool respect_timeout, NLmaps_query* ptr_query_info);
string execute_for_NLmaps(string db_dir, NLmaps_query* ptr_query_info, string clone_db_dir = "",
      uint log_level = Error_Output::VERBOSE,
      Debug_Level debug_level = parser_execute,
      int area_level = 0,
      bool respect_timeout = true);

#endif
