#ifndef NLMAPS_QUERY_CC
#define	NLMAPS_QUERY_CC

#include <boost/graph/adjacency_list.hpp>

using namespace std;

struct VertexValue { string value; int index; };
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, VertexValue> Tree;
typedef boost::graph_traits<Tree>::vertex_descriptor vertex;
typedef boost::graph_traits<Tree>::vertex_iterator vertex_iter;
typedef boost::graph_traits<Tree>::out_edge_iterator out_edge_iterator;

enum class cardinal_direction_t {NORTH, EAST, SOUTH, WEST};

struct OSM_element{
  string value = "";
  string name = "";
  string street = "";
  string street_number = "";
  string postcode = "";
  string town = "";
  string osm_id_number = "";
  string osm_type = "";
  pair<double, double> lat_lon;
  vector<string> detailed_info;
};

struct NLmaps_query{
  string mrl;
  string query;
  Tree mrl_tree;
  map<int, vertex> map_string2vertex;
  int number_vertices = -1;
  bool area = false;
  bool search = false;
  bool center = false;
  bool center_pivot = false;
  bool pivot = false;
  bool around = false;
  bool get_center_latlong = false;
  bool geojson = false;
  bool latlon = false;
  bool count = false;
  bool findkey = false;
  bool least = false;
  bool dist = false;
  int topx = 0; //if needed for around
  string maxdist;
  string perform_nodup;
  OSM_element ref_element;
  vector<string> keys;
  vector<string> keys_detailed_info;
  vector<OSM_element> elements;
  int count_elements = 0;
  vector<cardinal_direction_t> cardinal_directions;
  vector<string> osm_tags;
  vector<string> area_name;
};

#endif
