
/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "osm3s_query_nlmaps.h"

using namespace std;

int execute_command(string db_dir, string clone_db_dir, uint log_level, Debug_Level debug_level, int area_level, bool respect_timeout, NLmaps_query* ptr_query_info){
  Error_Output* error_output(new Console_Output(log_level));//this is passed to function via which they write to stderr
  Statement::set_error_output(error_output);

  if ((db_dir.size() > 0) && (db_dir[db_dir.size()-1] != '/'))
  db_dir += '/';

  register_pivot();
  register_area_query();
  register_around();
  register_bbox_query();
  register_coord_query();
  register_difference();
  register_foreach();
  register_id_query();
  register_item();
  register_make_area();
  register_map_to_area();
  register_newer();
  register_osm_script();
  register_polygon_query();
  register_print();
  register_query();
  register_recurse();
  register_union();
  register_user();

  // connect to dispatcher and get database dir
  try
  {
    if (clone_db_dir != "")
    {

      // open read transaction and log this.
      area_level = determine_area_level(error_output, area_level);
      Dispatcher_Stub dispatcher(db_dir, error_output, "-- clone database --",
				 get_uses_meta_data(), area_level, 24*60*60, 1024*1024*1024);

      clone_database(*dispatcher.resource_manager().get_transaction(), clone_db_dir);
      return 0;
    }

    string xml_raw = get_xml_from_string(error_output, ptr_query_info->query);

    if ((error_output) && (error_output->display_encoding_errors()))
      return 0;

    Statement::Factory stmt_factory;//defined in Statement.h
    if (!parse_and_validate(stmt_factory, xml_raw, error_output, debug_level))
      return 0;
    if (debug_level != parser_execute)
      return 0;

    Osm_Script_Statement* osm_script = 0;
    if (!get_statement_stack()->empty())
      osm_script = dynamic_cast< Osm_Script_Statement* >(get_statement_stack()->front());

    uint32 max_allowed_time = 0;
    uint64 max_allowed_space = 0;
    if (osm_script)
    {
      max_allowed_time = osm_script->get_max_allowed_time();
      max_allowed_space = osm_script->get_max_allowed_space();
    }
    else
    {
      Osm_Script_Statement temp(0, map< string, string >(), 0);
      max_allowed_time = temp.get_max_allowed_time();
      max_allowed_space = temp.get_max_allowed_space();
    }
    // Allow rules to run for unlimited time
    if (!respect_timeout)
      max_allowed_time = 0;

    // open read transaction and log this.
    area_level = determine_area_level(error_output, area_level);
    Dispatcher_Stub dispatcher(db_dir, error_output, xml_raw,
			       get_uses_meta_data(), area_level, max_allowed_time, max_allowed_space);

    if (osm_script && osm_script->get_desired_timestamp())
      dispatcher.resource_manager().set_desired_timestamp(osm_script->get_desired_timestamp());

    for (vector< Statement* >::const_iterator it(get_statement_stack()->begin());
	 it != get_statement_stack()->end(); ++it){
      (*it)->execute(dispatcher.resource_manager(), ptr_query_info);
    }

    return 0;
  }
  catch(File_Error e)
  {
    ostringstream temp;
    if (e.origin != "Dispatcher_Stub::Dispatcher_Stub::1")
    {
      temp<<"open64: "<<e.error_number<<' '<<strerror(e.error_number)<<' '<<e.filename<<' '<<e.origin;
      if (error_output)
        error_output->runtime_error(temp.str());
    }

    return 1;
  }
  catch(Resource_Error e)
  {
    ostringstream temp;
    if (e.timed_out)
      temp<<"Query timed out in \""<<e.stmt_name<<"\" at line "<<e.line_number
          <<" after "<<e.runtime<<" seconds.";
    else
      temp<<"Query run out of memory in \""<<e.stmt_name<<"\" at line "
          <<e.line_number<<" using about "<<e.size/(1024*1024)<<" MB of RAM.";
    if (error_output)
      error_output->runtime_error(temp.str());

    return 2;
  }
  catch(Exit_Error e)
  {
    return 3;
  }
}

string execute_for_NLmaps(string db_dir, NLmaps_query* ptr_query_info, string clone_db_dir,
      uint log_level,
      Debug_Level debug_level,
      int area_level,
      bool respect_timeout)
{
  string answer = "";
  execute_command(db_dir, clone_db_dir, log_level, debug_level, area_level, respect_timeout, ptr_query_info);
  clean_up_core();
  return answer;
}
