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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__PIVOT_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__PIVOT_H

#include <map>
#include <string>
#include <vector>
#include "statement.h"
#include "../nlmaps/nlmaps_query.h"

using namespace std;

int register_pivot();

class Pivot_Statement : public Output_Statement
{
  public:
    Pivot_Statement(int line_number_, const map< string, string >& attributes,
                    Query_Constraint* bbox_limitation = 0);
    virtual string get_name() const { return "pivot"; }
    virtual void execute(Resource_Manager& rman, NLmaps_query* ptr_query_info = NULL);
    virtual ~Pivot_Statement();

    static Generic_Statement_Maker< Pivot_Statement > statement_maker;

    virtual Query_Constraint* get_query_constraint();
    string get_input() const { return input; }

  private:
    string input;
    vector< Query_Constraint* > constraints;
};

template< class TIndex, class TObject >
void filter_elems(const vector< typename TObject::Id_Type >& ids,
                  map< TIndex, vector< TObject > >& elems);

#endif
