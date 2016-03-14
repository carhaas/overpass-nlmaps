For the NLmaps backend installation run: cmake . && make

Required dependencies are: zlib and Boost (regex, filesystem, program_options & system)

For instructions to populate the database see: http://wiki.openstreetmap.org/wiki/Overpass_API/Installation#Populating_the_DB

Usage:
to execute a NLmaps query (e.g. "query(nwr(keyval('amenity','exhibition_center')),qtype(findkey('name')))") run the following:

./query_db -d $DB_DIR -a answer_file -f query_file

where $DB_DIR contains the location of the database, answer_file is the location of the file which will contain the answer and query_file contains the queries to be run with each query in a separate line.

Additional one can specify "-l latlong_file" which is the location of the file which will contain the GPS location of all relevant entities (node, way or relation) found for the query.

To connect the backend with your own C++ project look at this file: src/overpass-api/dispatch/osm3s_query_nlmaps.cc
Some relevant code:

    NLmaps_query query_info;
    query_info.mrl = "query(nwr(keyval('amenity','exhibition_center')),qtype(findkey('name')))";
    Evaluator mrl_eval(database_dir);
    int i = mrl_eval.initalise(&query_info, true);
    string answer = mrl_eval.interpret(&query_info);
