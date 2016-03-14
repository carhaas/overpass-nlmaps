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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__DIFFERENCE_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__DIFFERENCE_H

#include <map>
#include <string>
#include <vector>

#include "statement.h"
#include "../nlmaps/nlmaps_query.h"

int register_difference();

class Difference_Statement : public Output_Statement
{
  public:
    Difference_Statement(int line_number_, const map< string, string >& input_attributes,
                         Query_Constraint* bbox_limitation = 0);
    virtual void add_statement(Statement* statement, string text);
    virtual string get_name() const { return "difference"; }
    virtual void execute(Resource_Manager& rman, NLmaps_query* ptr_query_info = NULL);
    virtual ~Difference_Statement() {}
    
    static Generic_Statement_Maker< Difference_Statement > statement_maker;

  private:
    vector< Statement* > substatements;
};


#endif
