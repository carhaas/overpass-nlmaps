For the NLmaps backend installation run: cmake . && make

Required dependencies are: cmake, g++, expat, zlib and Boost (regex, filesystem, program_options & system).

For example on Ubuntu:

    sudo apt-get install g++ cmake expat libexpat1-dev zlib1g-dev libboost-all-dev

For instructions to populate the database see: http://wiki.openstreetmap.org/wiki/Overpass_API/Installation#Populating_the_DB
Note: For the area creation a different set of scripts need to be build from the original (Overpass project)[https://github.com/drolbr/Overpass-API]
By renaming the file `Makefile_db_scripts` to `Makefile`, this can be done in the Overpass-nlmaps folder by running the following:

    cd osm-3s-dev-version
    pushd src/
    autoscan
    aclocal-1.11
    autoheader
    libtoolize
    automake-1.11 --add-missing
    autoconf
    popd
		../src/configure CXXFLAGS="-O3 -std=c++11" --prefix=/path/to/overpass-nlmaps
    make install

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
