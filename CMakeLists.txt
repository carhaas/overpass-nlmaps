cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(osm)

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -O3 -g")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lrt -ldl")

find_package(ZLIB REQUIRED)
find_package(Boost COMPONENTS regex filesystem program_options system REQUIRED)
find_package(EXPAT)

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(src/overpass_api/dispatch)
INCLUDE_DIRECTORIES(src/overpass_api/frontend)
INCLUDE_DIRECTORIES(src/overpass_api/osm-backend)
INCLUDE_DIRECTORIES(src/template_db)
INCLUDE_DIRECTORIES(src/expat)
INCLUDE_DIRECTORIES(src/pt_diagrams)
INCLUDE_DIRECTORIES(utils)
INCLUDE_DIRECTORIES(src/overpass_api/data)
INCLUDE_DIRECTORIES(src/overpass_api/statements)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIRS})


set(osm_nlmaps_STAT_SRCS
    src/overpass_api/frontend/console_output.cc
    src/overpass_api/dispatch/osm3s_query_nlmaps.cc
    src/overpass_api/osm-backend/clone_database.cc
    src/overpass_api/dispatch/scripting_core.cc
    src/overpass_api/dispatch/dispatcher_stub.cc
    src/overpass_api/dispatch/resource_manager.cc
    src/template_db/types.cc
    src/overpass_api/statements/statement
    src/overpass_api/statements/area_query.cc
    src/overpass_api/statements/around.cc
    src/overpass_api/statements/bbox_query.cc
    src/overpass_api/statements/changed.cc
    src/overpass_api/statements/coord_query.cc
    src/overpass_api/statements/difference.cc
    src/overpass_api/statements/foreach.cc
    src/overpass_api/statements/id_query.cc
    src/overpass_api/statements/item.cc
    src/overpass_api/statements/make_area.cc
    src/overpass_api/statements/map_to_area.cc
    src/overpass_api/statements/newer.cc
    src/overpass_api/statements/osm_script.cc
    src/overpass_api/statements/pivot.cc
    src/overpass_api/statements/polygon_query.cc
    src/overpass_api/statements/print.cc
    src/overpass_api/statements/query.cc
    src/overpass_api/statements/recurse.cc
    src/overpass_api/statements/union.cc
    src/overpass_api/statements/user.cc
    src/overpass_api/frontend/print_target.cc
    src/expat/escape_xml.cc
    src/overpass_api/data/collect_members.cc
    src/overpass_api/frontend/map_ql_parser.cc
    src/overpass_api/statements/statement_dump.cc
    src/expat/map_ql_input.cc
    src/overpass_api/data/filenames.h
    src/template_db/dispatcher.cc
    src/overpass_api/dispatch/resource_manager.cc
    src/overpass_api/osm-backend/area_updater.cc
    src/expat/expat_justparse_interface.cc
    src/overpass_api/frontend/cgi-helper.cc
    src/overpass_api/frontend/user_interface.cc
    src/overpass_api/frontend/output.cc
    src/overpass_api/core/settings.cc
    src/overpass_api/frontend/web_output.cc
    src/overpass_api/nlmaps/linearise.cc
    src/overpass_api/nlmaps/interpret.cc
    src/overpass_api/nlmaps/nlmaps_query.h)

add_library(osm_nlmaps STATIC ${osm_nlmaps_STAT_SRCS})
target_link_libraries(osm_nlmaps)

set(query_db_SRCS
    src/overpass_api/nlmaps/query_db.cc)
add_executable(query_db ${query_db_SRCS})
target_link_libraries(query_db expat osm_nlmaps ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${EXPAT_LIBRARIES})
