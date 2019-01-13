#ifndef INTERPRET_CC
#define	INTERPRET_CC

#include "nlmaps_query.h"
#include "../dispatch/osm3s_query_nlmaps.h"

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <numeric>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

#define WALKDING_DIST 1000
#define DIST_INTOWN 5000
#define DIST_OUTTOWN 20000
#define DIST_DAYTRIP 80000

typedef string String;
struct Expression;

enum class for_t {CAR, WALK, UNSET};
enum class and_or_t {AND, OR, UNSET};

static int prep_query(string& query){
  boost::replace_all(query, "SAVESPACE", " ");
  boost::replace_all(query, "SAVEAPO", "'");
  boost::replace_all(query, "BRACKETOPEN", "(");
  boost::replace_all(query, "BRACKETCLOSE", ")");
  return 0;
}

static int save_stoi(string convert_this){
  int number = 0;
  try{
    number = stoi(convert_this);
  } catch(invalid_argument e){
    cerr << "Warning: conversion to int could not be performed, argument invalid" << endl;
  } catch(out_of_range e){
    cerr << "Warning: conversion to int could not be performed, argument out of range" << endl;
  }
  return number;
}

static double distance_from_gps(double lat1, double long1, double lat2, double long2, bool km = true){
  double dist = -1;
  double avg_radius_earth_km = 6371;
  double avg_radius_earth_mi = 3959;

  double lat_dist = ((lat1 - lat2) * M_PI) / 180;
  double long_dist =((long1 - long2) * M_PI) / 180;

  double lat1_rad = (lat1 * M_PI) / 180;
  double lat2_rad = (lat2 * M_PI) / 180;

  dist = sin(lat_dist / 2) * sin(lat_dist / 2)
        + cos(lat1_rad) * cos(lat2_rad) * sin(long_dist / 2) * sin(long_dist / 2);

  dist = 2 * atan2(sqrt(dist), sqrt(1-dist));

  if(km){
    dist = avg_radius_earth_km * dist; //get km
  } else {
    dist = avg_radius_earth_mi * dist; //get miles
  }
  return dist;
}

static int reduce_answers_to_topx(NLmaps_query* ptr_query_info){
  vector<OSM_element> elements = ptr_query_info->elements; ptr_query_info->elements.clear();
  OSM_element ref_element = ptr_query_info->ref_element;
  vector<double> distances;
  for(auto it = elements.begin(); it != elements.end(); it++){
    double this_distance = distance_from_gps(ref_element.lat_lon.first, ref_element.lat_lon.second, it->lat_lon.first, it->lat_lon.second);
    distances.push_back(this_distance);
  }
  int count = 0;
  vector<size_t> permutation(distances.size());
  iota(permutation.begin(), permutation.end(), 0); //fills vector with increase numbers
  sort(permutation.begin(), permutation.end(), [&](size_t i, size_t j){ return distances[i] < distances[j]; });
  for(auto it = permutation.begin(); it != permutation.end(); it++, count++){
    if(count==ptr_query_info->topx){ break; }
    ptr_query_info->elements.push_back(elements.at((*it)));
  }
  return 0;
}

struct Expression {
  virtual string interpret(NLmaps_query* ptr_query_info) = 0;
	virtual ~Expression() {}
  virtual string class_name() = 0;
};

class Terminal : public Expression {
  String name;
  and_or_t and_or;
public:
	Terminal(String name, and_or_t const& and_or = and_or_t::UNSET) {
    this->name = name;
    this->and_or = and_or;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    return "\""+name+"\"";
  }
  string class_name(){ return "Terminal"; }
  string terminal_name(){ return name; }
};

class Keyval : public Expression {
  Terminal* ptr_key;
  Terminal* ptr_val;
  and_or_t and_or;
public:
	Keyval(Terminal* ptr_key, Terminal* ptr_val,
         and_or_t const& and_or = and_or_t::UNSET) {
    this->ptr_key = ptr_key;
    this->ptr_val = ptr_val;
    this->and_or = and_or;
  }
  ~Keyval(){
    delete ptr_key;
    delete ptr_val;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    if(ptr_key->terminal_name() != "name"){
      ptr_query_info->keys_detailed_info.push_back(ptr_key->terminal_name());
    }

    if(ptr_val->terminal_name() == "*"){
      return "["+ptr_key->interpret(ptr_query_info)+"]";
    }
    return "["+ptr_key->interpret(ptr_query_info)+"="+ptr_val->interpret(ptr_query_info)+"]";

  }
  string class_name(){ return "Keyval"; }
  string key(){ return ptr_key->terminal_name(); }
  string val(){ return ptr_val->terminal_name(); }
  and_or_t get_and_or(){ return and_or; }
};

class Nwr : public Expression {
  list<Keyval*> keyvals;
  and_or_t and_or;
  bool first;
  bool last;
public:
	Nwr(list<Keyval*> keyvals,
      bool first = true, bool last = true,
      and_or_t const& and_or = and_or_t::UNSET) {
    this->keyvals = keyvals;
    this->and_or = and_or;
    this->first = first;
    this->last = last;
  }
  ~Nwr(){
    for(auto it = keyvals.begin(); it != keyvals.end(); it++){ delete (*it); }
  }
  string interpret(NLmaps_query* ptr_query_info) {
    stringstream ss_assemble;
    for(auto it = keyvals.begin(); it != keyvals.end(); it++){
      ss_assemble << (*it)->interpret(ptr_query_info);
    }
    if(find(ptr_query_info->osm_tags.begin(), ptr_query_info->osm_tags.end(), ss_assemble.str()) != ptr_query_info->osm_tags.end()) {
      ;
    } else {
      string assembled = ss_assemble.str();
      prep_query(assembled);
      ptr_query_info->osm_tags.push_back(assembled);
    }
    stringstream assemble_total;
    if(first){assemble_total << "(";}
    if(ptr_query_info->area){
      assemble_total << "node(area.a)"+ss_assemble.str()+";way(area.a)"+ss_assemble.str()+";relation(area.a)"+ss_assemble.str()+";";
    } else if(ptr_query_info->search){
      assemble_total << "node(around.b:"+ptr_query_info->maxdist+")"+ss_assemble.str()+";way(around.b:"+ptr_query_info->maxdist+")"+ss_assemble.str()+";relation(around.b:"+ptr_query_info->maxdist+")"+ss_assemble.str()+";";
    } else {
      assemble_total << "node"+ss_assemble.str()+";way"+ss_assemble.str()+";relation"+ss_assemble.str()+";";
    }
    if(ptr_query_info->center && !ptr_query_info->get_center_latlong){
      assemble_total << ")->.b;";
    } else if(last){
      assemble_total << ");";
    }
    return assemble_total.str();
  }
  string class_name(){ return "Nwr"; }
  and_or_t get_and_or(){ return and_or; }
};

class AreaMRL : public Expression {
  list<Keyval*> keyvals;
  and_or_t and_or;
  bool first;
  bool last;
public:
	AreaMRL(list<Keyval*> keyvals,
      bool first = false, bool last = false,
      and_or_t const& and_or = and_or_t::UNSET) {
    this->keyvals = keyvals;
    this->and_or = and_or;
    this->first = first;
    this->last = last;
  }
  ~AreaMRL(){
    for(auto it = keyvals.begin(); it != keyvals.end(); it++){ delete (*it); }
  }
  string interpret(NLmaps_query* ptr_query_info) {
    stringstream ss_assemble;
    for(auto it = keyvals.begin(); it != keyvals.end(); it++){
      ss_assemble << (*it)->interpret(ptr_query_info);
    }
    if(find(ptr_query_info->area_name.begin(), ptr_query_info->area_name.end(), ss_assemble.str()) != ptr_query_info->area_name.end()) {
      ;
    } else {
      string assembled = ss_assemble.str();
      prep_query(assembled);
      ptr_query_info->area_name.push_back(assembled);
    }
    string assemble = ss_assemble.str();
    stringstream ss_assemble_total;
    size_t found_beginning = assemble.find("<nom>");
    if(found_beginning != string::npos){
      //extract relevant
      assemble = assemble.substr(found_beginning+5, assemble.size());
      size_t found_end = assemble.find("</nom>");
      assemble = assemble.substr(0, found_end);
      ss_assemble_total << "area(" << assemble << ")->.a;";//TODO is it ok to do? (cos below we handle or() etc.)
      if(ptr_query_info->pivot){
        return "area("+assemble+");relation(pivot);";
      }
      if(ptr_query_info->center_pivot){
        return "area("+assemble+");relation(pivot)->.b;";
      }
      return ss_assemble_total.str();
    }
    if(ptr_query_info->center_pivot){
      return "area"+ss_assemble.str()+";relation(pivot)->.b;";
    }
    if(ptr_query_info->pivot){
      return "area"+ss_assemble.str()+";relation(pivot);";
    }
    if(first){ ss_assemble_total << "("; }
    if(and_or == and_or_t::OR && !first){ ss_assemble_total << ";"; }
    ss_assemble_total << "area"+assemble;
    if(last){ ss_assemble_total << ")"; }
    if(and_or != and_or_t::OR || (and_or == and_or_t::OR && last)){ ss_assemble_total << "->.a;"; }
    return ss_assemble_total.str();
  }
  string class_name(){ return "AreaMRL"; }
  and_or_t get_and_or(){ return and_or; }
};

class Topx : public Expression {
  string s_topx;
public:
	Topx(string s_topx) {
    this->s_topx = s_topx;
    size_t found_beginning = s_topx.find("<topx>");
    if(found_beginning != string::npos){
      s_topx = s_topx.substr(found_beginning+6, s_topx.size());
      size_t found_end = s_topx.find("</topx>");
      s_topx = s_topx.substr(0, found_end);
    }
  }
  string interpret(NLmaps_query* ptr_query_info) {
    return s_topx;
  }
  string class_name(){ return "Topx"; }
};

class Maxdist : public Expression {
  string s_maxdist;
public:
	Maxdist(string s_maxdist) { this->s_maxdist = s_maxdist; }
  string interpret(NLmaps_query* ptr_query_info) {
    if(s_maxdist == "WALKDING_DIST"){
      return to_string(WALKDING_DIST);
    } else if(s_maxdist == "DIST_INTOWN"){
      return to_string(DIST_INTOWN);
    } else if(s_maxdist == "DIST_OUTTOWN"){
      return to_string(DIST_OUTTOWN);
    } else if(s_maxdist == "DIST_DAYTRIP"){
      return to_string(DIST_DAYTRIP);
    }
    return s_maxdist;
  }
  string class_name(){ return "Maxdist"; }
};

class Nodup : public Expression {
//NOTE: Nodup only works for Findkey and only for an entire MRL (if e.g. findkey also holds and() or or())
public:
	Nodup() {}
  string interpret(NLmaps_query* ptr_query_info) {
    stringstream ss_answer;
    vector<string> assemble_answer;
    vector<string> assemble_answer_unique;
    boost::split(assemble_answer, ptr_query_info->perform_nodup, boost::is_any_of(","));
    for(vector<string>::iterator it(assemble_answer.begin()); it != assemble_answer.end(); ++it){
      boost::trim((*it));
      if(find(assemble_answer_unique.begin(), assemble_answer_unique.end(), (*it)) == assemble_answer_unique.end()) {
        assemble_answer_unique.push_back((*it));
      }
    }
    bool first = true;
    for(vector<string>::iterator it(assemble_answer_unique.begin()); it != assemble_answer_unique.end(); ++it){
      if(first){
        ss_answer << (*it);
        first= false;
      } else {
        ss_answer << ", " << (*it);
      }
    }
    ptr_query_info->perform_nodup = ss_answer.str();
    return "";
  }
  string class_name(){ return "Nodup"; }
};

class AnswerType : public Expression {
public:
  virtual string type() = 0;
};

class Latlong : public AnswerType {
  Topx* ptr_topx;
public:
	Latlong(Topx* ptr_topx = NULL) {
    this->ptr_topx = ptr_topx;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    ptr_query_info->latlon = true;
    if(ptr_query_info->geojson){ return ""; }
    int topx = -1;
    if(ptr_topx != NULL){ topx = save_stoi(ptr_topx->interpret(ptr_query_info)); }
    bool first = true;
    stringstream assemble;
    for(auto it = ptr_query_info->elements.begin(); it != ptr_query_info->elements.end(); it++){
      if(topx==0){ break; }
      if(first){
        first = false;
        assemble << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
      } else {
        assemble << ", " << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
      }
      topx--;
    }
    return assemble.str();
  }
  string type(){ return "latlong"; }
  string class_name(){ return "Latlong"; }
};

class Count : public AnswerType {
public:
	Count() {}
  string interpret(NLmaps_query* ptr_query_info) {
    ptr_query_info->count = true;
    return to_string(ptr_query_info->count_elements);
  }
  string type(){ return "count"; }
  string class_name(){ return "Count"; }
};

class Least : public AnswerType {
  Topx* ptr_topx;
public:
	Least(Topx* ptr_topx){
    this->ptr_topx = ptr_topx;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    ptr_query_info->least = true;
    if(ptr_query_info->count_elements >= save_stoi(ptr_topx->interpret(ptr_query_info))){
      return "yes";
    }
    return "no";
  }
  string type(){ return "least"; }
  string class_name(){ return "Least"; }
};

class Findkey : public AnswerType {
  Topx* ptr_topx;
public:
	Findkey(Topx* ptr_topx = NULL){
    this->ptr_topx = ptr_topx;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    ptr_query_info->findkey = true;
    int topx = -1;
    if(ptr_topx != NULL){ topx = save_stoi(ptr_topx->interpret(ptr_query_info)); }
    bool first = true;
    stringstream assemble;
    for(auto it = ptr_query_info->elements.begin(); it != ptr_query_info->elements.end(); it++){
      if(topx==0){ break; }
      if(it->value == ""){ //element for findkey can be empty. this is needed upto this point to ensure that answer_latlong and answer_findkey have the same # of elements
        continue;
      }
      if(first){
        first = false;
        assemble << it->value;
      } else {
        assemble << ", " << it->value;
      }
      topx--;
    }
    return assemble.str();
  }
  string type(){ return "findkey"; }
  string class_name(){ return "Findkey"; }
};

class AnswerTypeFactory {
public:
  static AnswerType* createAnswerType(string& description, Topx* ptr_topx = NULL){
    if(description == "latlong"){
      return new Latlong(ptr_topx);
    } else if(description == "count"){
      return new Count();
    } else if(description == "least"){
      if(ptr_topx == NULL){ return NULL; }
      return new Least(ptr_topx);
    } else if(description == "findkey"){
      return new Findkey(ptr_topx);
    }
    return NULL;
  }
};

class Qtype : public Expression {
  list<AnswerType*> answer_types;
  Nodup* ptr_nodup;
public:
	Qtype(list<AnswerType*> answer_types, Nodup* ptr_nodup = NULL) {
    this->answer_types = answer_types;
    this->ptr_nodup = ptr_nodup;
  }
  ~Qtype(){
    for(auto it = answer_types.begin(); it != answer_types.end(); it++){ delete (*it); }
  }

  string interpret(NLmaps_query* ptr_query_info) {
    stringstream assemble_answer;
    bool filled = false;
    //contains hack to preserve answer order from the previous system: least, count, findkey, latlong
    string answer_least, answer_count, answer_findkey, answer_latlong = "";
    if(ptr_query_info->topx>0 && ptr_query_info->around){ //if we have an around query containing topx, then this value is >0
      reduce_answers_to_topx(ptr_query_info);
    }
    for(auto it = answer_types.begin(); it != answer_types.end(); it++){
      string current_answer = (*it)->interpret(ptr_query_info);
      //pretty up answer
      boost::replace_all(current_answer, ";", ", ");
      boost::replace_all(current_answer, ", ", ","); //there is some cases where there is no space after the comma, these two lines fix that
      boost::replace_all(current_answer, ",", ", ");
      boost::replace_all(current_answer, "_", " ");
      if(ptr_nodup!=NULL && (*it)->type()=="findkey"){
        ptr_query_info->perform_nodup = current_answer;
        ptr_nodup->interpret(ptr_query_info);
        current_answer = ptr_query_info->perform_nodup;
      }
      if((*it)->type()=="least"){ answer_least = current_answer; }
      else if((*it)->type()=="count"){ answer_count = current_answer; }
      else if((*it)->type()=="findkey"){ answer_findkey = current_answer; }
      else if((*it)->type()=="latlong"){ answer_latlong = current_answer; }
    }
    if(answer_least != ""){ assemble_answer << answer_least; filled = true; }
    if(answer_count != ""){
      if(filled){ assemble_answer << " and "; }
      assemble_answer << answer_count;
      filled = true;
    }
    if(answer_findkey != ""){
      if(filled){ assemble_answer << " and "; }
      assemble_answer << answer_findkey;
      filled = true;
    }
    if(answer_latlong != ""){
      if(filled){ assemble_answer << " and "; }
      assemble_answer << answer_latlong;
      filled = true;
    }
    return assemble_answer.str();
  }
  string class_name(){ return "Qtype"; }
};

class Element : public Expression {
  list<Nwr*> nwrs;
  list<AreaMRL*> areas;
  bool search;
  bool center;
public:
	Element(list<Nwr*> nwrs, list<AreaMRL*> areas,
          bool search = false,  bool center = false) {
    this->nwrs = nwrs;
    this->areas = areas;
    this->search = search;
    this->center = center;
  }

  string interpret(NLmaps_query* ptr_query_info) {
    if(search){ ptr_query_info->search = true; }
    if(center){ ptr_query_info->center = true; }
    if(center && nwrs.size() == 0){ ptr_query_info->center_pivot = true; }
    stringstream assemble;
    if(areas.size() > 0){
      ptr_query_info->area = true;
      for(auto it = areas.begin(); it != areas.end(); it++){
        assemble << (*it)->interpret(ptr_query_info);
      }
    }
    if(!ptr_query_info->pivot){
      for(auto it = nwrs.begin(); it != nwrs.end(); it++){
        assemble << (*it)->interpret(ptr_query_info);
      }
    }
    ptr_query_info->area = false; //switch back to false in case there is any other later on
    ptr_query_info->search = false;
    ptr_query_info->center = false;
    ptr_query_info->center_pivot = false;
    return assemble.str();
  }
  string class_name(){ return "Element"; }
};

class Query : public Expression {
  string db_dir;
  Qtype* qtype;
  Element* ptr_ele;
  Element* ptr_ele_ref;
  Maxdist* maxdist;
public:
	Query(string db_dir, Qtype* qtype,
        Element* ptr_ele, Element* ptr_ele_ref = NULL,
        Maxdist* maxdist = NULL) {
    this->qtype = qtype;
    this->ptr_ele = ptr_ele;
    this->ptr_ele_ref = ptr_ele_ref;
    this->db_dir = db_dir;
    this->maxdist = maxdist;
  }

  int set_ref_element(NLmaps_query* ptr_query_info, string& query){
    //assemble query to get center latlong
    prep_query(query);
    ptr_query_info->query = query;
    execute_for_NLmaps(db_dir, ptr_query_info);
    if(ptr_query_info->elements.size()>0){//take the first GPS as ref point
      ptr_query_info->ref_element = ptr_query_info->elements[0];
    }
    //reset values for the actual query
    ptr_query_info->elements.clear();
    ptr_query_info->count_elements = 0;
    ptr_query_info->get_center_latlong = false;
    return 0;
  }

  string interpret(NLmaps_query* ptr_query_info) {
    //assemble query
    string query = "";
    if(ptr_ele_ref!=NULL && maxdist!=NULL){ //around query
      ptr_query_info->around = true;

      if(ptr_query_info->topx>0){ ptr_query_info->get_center_latlong = true; }
      if(ptr_query_info->get_center_latlong){
        query = ptr_ele_ref->interpret(ptr_query_info)+"out;";
        set_ref_element(ptr_query_info, query);
      }
      ptr_query_info->maxdist = maxdist->interpret(ptr_query_info);
      query = ptr_ele_ref->interpret(ptr_query_info)+ptr_ele->interpret(ptr_query_info)+"out;";
    } else if(ptr_query_info->cardinal_directions.size()>0 && ptr_query_info->get_center_latlong) { //cardinal direction query, the ref lat_lon needs to be set
      //this is only for non-around queries, around queries are handled above
      ptr_query_info->pivot = true;
      query = ptr_ele->interpret(ptr_query_info)+"out;";
      set_ref_element(ptr_query_info, query);
      ptr_query_info->pivot = false;
      query = ptr_ele->interpret(ptr_query_info)+"out;";
    } else { //not around query
      query = ptr_ele->interpret(ptr_query_info)+"out;";
    }

    //execute query
    prep_query(query);
    ptr_query_info->query = query;
    execute_for_NLmaps(db_dir, ptr_query_info);

    //call interpret on answer_types
    string answer = qtype->interpret(ptr_query_info);
    return answer;
  }
  string class_name(){ return "Query"; }
};

class Dist : public Expression {
  Query* first;
  Query* second;
  bool km;
  for_t for_what;
public:
	Dist(Query* first, Query* second = NULL,
       bool km = true, for_t const& for_what = for_t::UNSET) {
    this->first = first;
    this->second = second;
    this->km = km;
    this->for_what = for_what;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    ptr_query_info->dist = true;
    double distance = numeric_limits<double>::infinity();
    OSM_element element_first; element_first.lat_lon = make_pair(-1, -1);
    OSM_element element_second; element_second.lat_lon = make_pair(-1, -1);

    if(second==NULL){ //then around query
      ptr_query_info->get_center_latlong=true;
      first->interpret(ptr_query_info);
      element_first = ptr_query_info->ref_element;
      if(ptr_query_info->elements.size()>0){ element_second = ptr_query_info->elements[0]; }
    } else {
      first->interpret(ptr_query_info);
      if(ptr_query_info->elements.size()>0){ element_first = ptr_query_info->elements[0]; }
      ptr_query_info->elements.clear();
      second->interpret(ptr_query_info);
      if(ptr_query_info->elements.size()>0){ element_second = ptr_query_info->elements[0]; }
    }
    ptr_query_info->elements.clear();
    ptr_query_info->elements.push_back(element_first);
    ptr_query_info->elements.push_back(element_second);
    if(element_first.lat_lon.first == -1 || element_first.lat_lon.second == -1 || element_second.lat_lon.first == -1 || element_second.lat_lon.second == -1){ //then one of the elements wasn't found and distance should remain numeric infinity
      if(for_what==for_t::CAR){
        return "yes";
      } else if(for_what==for_t::WALK){
        return "no";
      } else {
        return "";
      }
    } else {
      distance = distance_from_gps(element_first.lat_lon.first, element_first.lat_lon.second,
                                   element_second.lat_lon.first, element_second.lat_lon.second, km);
    }
    if(for_what==for_t::CAR){
      double compare_to = 0.0;
      if(km){
        compare_to = WALKDING_DIST/1000; //convert to km first
      } else {
        compare_to = (WALKDING_DIST/1000)*0.62137119; //converts km to mi
      }
      if(distance > compare_to){
        return "yes";
      } else {
        return "no";
      }
    }
    if(for_what==for_t::WALK){
      double compare_to = 0.0;
      if(km){
        compare_to = WALKDING_DIST/1000; //convert to km first
      } else {
        compare_to = (WALKDING_DIST/1000)*0.62137119; //converts km to mi
      }
      if(distance > compare_to){
        return "no";
      } else {
        return "yes";
      }
    }
    return boost::lexical_cast<std::string>(distance);  //needed for legacy
  }
  string class_name(){ return "Dist"; }
};

class And : public Expression {
  list<Query*> queries;
public:
	And(list<Query*> queries) {
    this->queries = queries;
  }
  string interpret(NLmaps_query* ptr_query_info) {
    stringstream assemble;
    bool first = true;
    string answer = "";
    //save elements from previous queries
    vector<OSM_element> save_elements;
    for(auto it = queries.begin(); it != queries.end(); it++){
      for(auto it2 = ptr_query_info->elements.begin(); it2 != ptr_query_info->elements.end(); it2++){
        save_elements.push_back((*it2));
      }
      ptr_query_info->elements.clear();
      ptr_query_info->count_elements = 0;
      if(first){
        answer = (*it)->interpret(ptr_query_info);
        assemble << answer;
        first = false;
      } else {
        answer = (*it)->interpret(ptr_query_info);
        assemble << " and " << answer;
      }
    }
    //add saved element back in
    for(auto it = save_elements.begin(); it != save_elements.end(); it++){
      ptr_query_info->elements.push_back((*it));
    }
    return assemble.str();
  }
  string class_name(){ return "And"; }
};

class Evaluator : public Expression {
  Expression* syntaxTree = NULL;
  string db_dir;

public:
	Evaluator(string db_dir){
    this->db_dir = db_dir;
  }

  ~Evaluator() {
    delete syntaxTree;
  }

  int initalise(NLmaps_query* ptr_query_info, bool geojson = false){
    if(geojson){ ptr_query_info->geojson = true; } //then we don't return latlong as part of the answer
    Tree& mrl_tree = ptr_query_info->mrl_tree;
    map<int, vertex>& map_string2vertex = ptr_query_info->map_string2vertex;
    list<Expression*> expressions;
    int i = -1;
    Element* search = NULL;
    Element* search_and = NULL;
    Element* center = NULL;
    for_t for_what = for_t::UNSET;
    bool km = true;
    string prev_value = "";
    for(i = ptr_query_info->number_vertices; i>=0; --i){
      vertex v = map_string2vertex[i];
      string current_value = mrl_tree[v].value;
      pair<out_edge_iterator, out_edge_iterator> e_out_iter;
      if(current_value == "unit"){  // KEYWORD
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        int count_children = 0;
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          if(mrl_tree[u].value == "mi"){
            km = false;
          } else if(mrl_tree[u].value == "km"){
            km = true;
          }
          count_children++;
        }
        if(count_children!=1){cerr << "Warning: unit() should only have 1 child" << endl;}
      } else if(current_value == "for"){  // KEYWORD
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        int count_children = 0;
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          if(mrl_tree[u].value == "car"){
            for_what = for_t::CAR;
          } else if(mrl_tree[u].value == "walk"){
            for_what = for_t::WALK;
          }
          count_children++;
        }
        if(count_children!=1){cerr << "Warning: for() should only have 1 child" << endl;}
      } else if(current_value == "qtype"){  // KEYWORD
        list<AnswerType*> answers;
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        int nodup_index = -1;
        Nodup* ptr_nodup = NULL;
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){//get different AnswerType: count, findkey, latlong and least
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          if(mrl_tree[u].value == "nodup"){//if Nodup, we need to switch to the child (there is only ever 1 and that is a findkey)
            nodup_index = mrl_tree[u].index;
            ptr_nodup = new Nodup();
            pair<out_edge_iterator, out_edge_iterator>  e_out_iter_nodup_child = boost::out_edges(map_string2vertex[nodup_index], mrl_tree);
            for(; e_out_iter_nodup_child.first != e_out_iter_nodup_child.second; ++e_out_iter_nodup_child.first){
              u = boost::target(*e_out_iter_nodup_child.first, mrl_tree);
              break; //just in case the MRL is invalid, we break to avoid chaos
            }
          }
          pair<out_edge_iterator, out_edge_iterator> e_out_iter_inner = boost::out_edges(map_string2vertex[mrl_tree[u].index], mrl_tree);
          Topx* ptr_topx = NULL;
          for(; e_out_iter_inner.first != e_out_iter_inner.second; ++e_out_iter_inner.first){ // in case AnswerType has further specifications: either topx or some value for key in findkey()
            vertex w = boost::target(*e_out_iter_inner.first, mrl_tree);
            if(mrl_tree[w].value == "topx"){
              pair<out_edge_iterator, out_edge_iterator> e_out_iter_inner_topx = boost::out_edges(map_string2vertex[mrl_tree[w].index], mrl_tree);
              for(; e_out_iter_inner_topx.first != e_out_iter_inner_topx.second; ++e_out_iter_inner_topx.first){ //get topx child, there should be only 1
                vertex x = boost::target(*e_out_iter_inner_topx.first, mrl_tree);
                ptr_query_info->topx = save_stoi(mrl_tree[x].value);
                ptr_topx = new Topx(mrl_tree[x].value);
              }
            } else if(mrl_tree[w].value == "and") { //else it a findkey(and(*,*))
              pair<out_edge_iterator, out_edge_iterator> e_out_iter_inner_findkey_and = boost::out_edges(map_string2vertex[mrl_tree[w].index], mrl_tree);
              for(; e_out_iter_inner_findkey_and.first != e_out_iter_inner_findkey_and.second; ++e_out_iter_inner_findkey_and.first){
                vertex x = boost::target(*e_out_iter_inner_findkey_and.first, mrl_tree);
                ptr_query_info->keys.push_back(mrl_tree[x].value);
              }
            } else{ //else it is some key for findkey()
              ptr_query_info->keys.push_back(mrl_tree[w].value);
            }
          }
          AnswerType* answer_type = AnswerTypeFactory::createAnswerType(mrl_tree[u].value, ptr_topx);
          if(answer_type!=NULL){
            answers.push_back(answer_type);
          } else {
            cerr << "Warning: Tried and failed to create the answer type \"" << mrl_tree[u].value << "\" for query: "<< ptr_query_info->mrl << endl;
          }
        }
        if(answers.size() == 0){ return 1; }
        expressions.push_back( new Qtype(answers, ptr_nodup) );
      } else if(current_value == "around"){  // KEYWORD
        //this around is only needed to specifially check for topx() as a child to around, topx() as some child of qtype() is covered under qtype
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          if(mrl_tree[u].value == "topx"){
            pair<out_edge_iterator, out_edge_iterator> e_out_iter_inner_topx = boost::out_edges(map_string2vertex[mrl_tree[u].index], mrl_tree);
            for(; e_out_iter_inner_topx.first != e_out_iter_inner_topx.second; ++e_out_iter_inner_topx.first){ //get topx child, there should be only 1
              vertex v = boost::target(*e_out_iter_inner_topx.first, mrl_tree);
              ptr_query_info->topx = save_stoi(mrl_tree[v].value);
            }
          }
        }
      } else if(current_value == "maxdist"){  // KEYWORD
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        int count_children = 0;
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          expressions.push_back( new Maxdist(mrl_tree[u].value) );
          count_children++;
        }
        if(count_children!=1){cerr << "Warning: maxdist() should only have 1 child" << endl;}
      } else if(current_value == "keyval"){  // KEYWORD
        bool and_or_case = false;
        and_or_t and_or = and_or_t::UNSET;
        bool future_and_or_case = false; // it is a future and/or iff either one ahead or four ahead are and/or
        //check for future and/or
        vertex look_ahead = map_string2vertex[i-1];
        string next_value = mrl_tree[look_ahead].value;
        if(next_value == "or" || next_value == "and"){
          future_and_or_case = true;
          if(next_value == "or"){ and_or = and_or_t::OR; }
          if(next_value == "and"){ and_or = and_or_t::AND; }
        }
        if((i-4)>=0){ //ensure that we will not go out of range
          look_ahead = map_string2vertex[i-4];
          next_value = mrl_tree[look_ahead].value;
          look_ahead = map_string2vertex[i-3];
          string verify_keval = mrl_tree[look_ahead].value;// else we might jump from 1 nwr to the next or an area
          if((next_value == "or" || next_value == "and") && verify_keval == "keyval"){
            future_and_or_case = true;
            if(next_value == "or"){ and_or = and_or_t::OR; }
            if(next_value == "and"){ and_or = and_or_t::AND; }
          }
        } //end of check for future and/or
        e_out_iter = boost::out_edges(map_string2vertex[i], mrl_tree);
        for(; e_out_iter.first != e_out_iter.second; ++e_out_iter.first){ // exactly 2
          vertex u = boost::target(*e_out_iter.first, mrl_tree);
          string& keyval_child_value = mrl_tree[u].value;
          if(keyval_child_value == "or" || keyval_child_value == "and"){//CASE: keyval('organic',or/and('only','yes')
            and_or_case = true;
            if(keyval_child_value == "or"){ and_or = and_or_t::OR; }
            if(keyval_child_value == "and"){ and_or = and_or_t::AND; }
            pair<out_edge_iterator, out_edge_iterator> e_out_iter_inner = boost::out_edges(map_string2vertex[mrl_tree[u].index], mrl_tree);
            for(; e_out_iter_inner.first != e_out_iter_inner.second; ++e_out_iter_inner.first){ //children of or/and. exactly 2
              vertex v = boost::target(*e_out_iter_inner.first, mrl_tree);
              expressions.push_back( new Terminal(mrl_tree[v].value, and_or) );
            }
            continue;
          }
          expressions.push_back( new Terminal(mrl_tree[u].value) );
        }
        if(expressions.size() < 2){ return 2; }
        Terminal* ptr_val2 = NULL;
        Terminal* ptr_val = dynamic_cast<Terminal*>(expressions.back()); expressions.pop_back();
        if(and_or_case){
          if(expressions.size() < 1){ return 3; }
          ptr_val2 = dynamic_cast<Terminal*>(expressions.back()); expressions.pop_back();
        }
        Terminal* ptr_key = dynamic_cast<Terminal*>(expressions.back()); expressions.pop_back();
        if(ptr_val!=0 && ptr_key!=0){
          if(and_or_case && ptr_val2!=0){
            Expression* subExpression = new Keyval(ptr_key, ptr_val, and_or);
            expressions.push_back( subExpression );
            Expression* subExpression2 = new Keyval(ptr_key, ptr_val2, and_or);
            expressions.push_back( subExpression2 );
          } else if(future_and_or_case){ //then and_or is set
            Expression* subExpression = new Keyval(ptr_key, ptr_val, and_or);
            expressions.push_back( subExpression );
          } else {
            Expression* subExpression = new Keyval(ptr_key, ptr_val);
            expressions.push_back( subExpression );
          }
        } else {
          return 4;
        }
      } else if(current_value == "nwr" || current_value == "area"){  // KEYWORD
        //assumes that in 1 nwr there is at most one or()
        list<Keyval*> keyvals;
        list<Keyval*> keyvals_or;
        and_or_t and_or = and_or_t::UNSET;
        bool or_found = false;
        bool and_found = false;
        while(expressions.size()>0){
          Keyval* keyval = dynamic_cast<Keyval*>(expressions.back());
          if(keyval==NULL){ break; }
          and_or = keyval->get_and_or();
          if(and_or!=and_or_t::UNSET){ //then automatically the next in the stack is its partner
            if(and_or==and_or_t::OR){ or_found = true; }
            if(and_or==and_or_t::AND){ and_found = true; }
            keyvals_or.push_back(keyval); //order might seem strange but is needed to preserve ordering
            expressions.pop_back();
            keyval = dynamic_cast<Keyval*>(expressions.back());
            if(keyval==NULL){ break; cerr << "i am null" << endl; }
            keyvals.push_back(keyval);
            expressions.pop_back();
          } else {
            keyvals.push_back(keyval);
            keyvals_or.push_back(keyval);
            expressions.pop_back();
          }
        }

        if(keyvals.size() == 0){ return 5; }
        if(current_value == "nwr"){
          if(or_found){
            //false and true is seemingly reversed but it's correct due to the stack
            Expression* subExpression = new Nwr(keyvals, false, true, and_or);
            expressions.push_back( subExpression );
            Expression* subExpression2 = new Nwr(keyvals_or, true, false, and_or);
            expressions.push_back( subExpression2 );
          } else if(and_found) {
            Expression* subExpression = new Nwr(keyvals, true, true, and_or);
            expressions.push_back( subExpression );
            Expression* subExpression2 = new Nwr(keyvals_or, true, true, and_or);
            expressions.push_back( subExpression2 );
          } else {
            Expression* subExpression = new Nwr(keyvals);
            expressions.push_back( subExpression );
          }
        } else if(current_value == "area"){
          if(or_found){
            Expression* subExpression = new AreaMRL(keyvals, false, true, and_or);
            expressions.push_back( subExpression );
            Expression* subExpression2 = new AreaMRL(keyvals_or, true, false, and_or);
            expressions.push_back( subExpression2 );
          } else if(and_found) {
            Expression* subExpression = new AreaMRL(keyvals, true, true, and_or);
            expressions.push_back( subExpression );
            Expression* subExpression2 = new AreaMRL(keyvals_or, true, true, and_or);
            expressions.push_back( subExpression2 );
          } else {
            Expression* subExpression = new AreaMRL(keyvals);
            expressions.push_back( subExpression );
          }
        }
      } else if(current_value == "north"){  // KEYWORD
        cardinal_direction_t c = cardinal_direction_t::NORTH;
        ptr_query_info->cardinal_directions.push_back(c);
        ptr_query_info->get_center_latlong = true;
      } else if(current_value == "south"){  // KEYWORD
        cardinal_direction_t c = cardinal_direction_t::SOUTH;
        ptr_query_info->cardinal_directions.push_back(c);
        ptr_query_info->get_center_latlong = true;
      } else if(current_value == "east"){  // KEYWORD
        cardinal_direction_t c = cardinal_direction_t::EAST;
        ptr_query_info->cardinal_directions.push_back(c);
        ptr_query_info->get_center_latlong = true;
      } else if(current_value == "west"){  // KEYWORD
        cardinal_direction_t c = cardinal_direction_t::WEST;
        ptr_query_info->cardinal_directions.push_back(c);
        ptr_query_info->get_center_latlong = true;
      } else if(current_value == "query" || current_value == "search" || current_value == "center"){  // KEYWORD
        list<AreaMRL*> areas;
        list<AreaMRL*> areas_and;
        and_or_t and_or_area;
        bool area_and_found = false;
        while(expressions.size()>0){
          AreaMRL* area = dynamic_cast<AreaMRL*>(expressions.back());
          if(area==NULL){ break; }
          and_or_area = area->get_and_or();
          if(and_or_area!=and_or_t::UNSET){
            if(and_or_area==and_or_t::OR){
              areas.push_back(area);
              expressions.pop_back();
            }
            if(and_or_area==and_or_t::AND){ //then automatically the next in the stack is its partner
              area_and_found = true;
              areas_and.push_back(area);
              expressions.pop_back();
              area = dynamic_cast<AreaMRL*>(expressions.back());
              if(area==NULL){ break; }
              areas.push_back(area);
              expressions.pop_back();
            }
          } else {
            areas.push_back(area);
            areas_and.push_back(area);
            expressions.pop_back();
          }
        }
        list<Nwr*> nwrs;
        list<Nwr*> nwrs_and;
        and_or_t and_or;
        bool and_found = false;
        while(expressions.size()>0){
          Nwr* nwr = dynamic_cast<Nwr*>(expressions.back());
          if(nwr==NULL){ break; }
          and_or = nwr->get_and_or();
          if(and_or!=and_or_t::UNSET){
            if(and_or==and_or_t::OR){
              nwrs.push_back(nwr);
              expressions.pop_back();
            }
            if(and_or==and_or_t::AND){ //then automatically the next in the stack is its partner
              and_found = true;
              nwrs_and.push_back(nwr);
              expressions.pop_back();
              nwr = dynamic_cast<Nwr*>(expressions.back());
              if(nwr==NULL){ break; }
              nwrs.push_back(nwr);
              expressions.pop_back();
            }
          } else {
            nwrs.push_back(nwr);
            nwrs_and.push_back(nwr);
            expressions.pop_back();
          }
        }
        if(current_value == "search"){
          search = new Element(nwrs, areas, true);
          if(and_found){ search_and = new Element(nwrs_and, areas, true); }
        } else if(current_value == "center"){
          center = new Element(nwrs, areas, false, true);
        } else if(current_value == "query"){
          Maxdist* maxdist = NULL;
          if(search!=NULL && center!=NULL){ //around query, need to pop_back maxdist first
            maxdist = dynamic_cast<Maxdist*>(expressions.back()); //if around then there should be a maxdist here
            if(maxdist == NULL){ return 6; }
            expressions.pop_back();
          }
          if(expressions.size() == 0){ return 7; } //stack empty when it shouldn't be
          Qtype* qtype = NULL;
          qtype = dynamic_cast<Qtype*>(expressions.back()); //there can only be 1 Qtype per query
          if(qtype == NULL){ return 8; }
          expressions.pop_back();
          if(search!=NULL && center!=NULL){ //around query
            Expression* subExpression = new Query(db_dir, qtype, search, center, maxdist);
            expressions.push_back( subExpression );
            if(search_and != NULL){
              Expression* subExpression2 = new Query(db_dir, qtype, search_and, center, maxdist);
              expressions.push_back( subExpression2 );
              and_found = true;
            }
          } else { //not around query
            if(nwrs.size() == 0){ return 9; }//need at least 1 of those for a valid query
            Element* ptr_ele = new Element(nwrs, areas); //this is required to be filled with at least 1 nwr
            Expression* subExpression = new Query(db_dir, qtype, ptr_ele);
            expressions.push_back( subExpression );
            if(and_found){
              Element* ptr_ele2 = new Element(nwrs_and, areas); //this is required to be filled with at least 1 nwr
              Expression* subExpression2 = new Query(db_dir, qtype, ptr_ele2);
              expressions.push_back( subExpression2 );
            } else if(area_and_found){
              Element* ptr_ele2 = new Element(nwrs, areas_and); //this is required to be filled with at least 1 nwr
              Expression* subExpression2 = new Query(db_dir, qtype, ptr_ele2);
              expressions.push_back( subExpression2 );
            }
          }
          if(and_found || area_and_found){ //then the MRL contains an and thus we need 2 queries
            list<Query*> queries;
            Query* ptr_and_1 = dynamic_cast<Query*>(expressions.back()); expressions.pop_back();
            Query* ptr_and_2 = dynamic_cast<Query*>(expressions.back()); expressions.pop_back();
            if(ptr_and_1!=NULL && ptr_and_2!=NULL){
              queries.push_back(ptr_and_1);
              queries.push_back(ptr_and_2);
              Expression* AndExpression = new And(queries);
              expressions.push_back( AndExpression );
            }
          }
        }
      } else if(current_value == "dist"){  // KEYWORD
        if(expressions.size() == 2){
          Query* ptr_query_1 = dynamic_cast<Query*>(expressions.back()); expressions.pop_back();
          Query* ptr_query_2 = dynamic_cast<Query*>(expressions.back()); expressions.pop_back();
          if(ptr_query_1!=0 && ptr_query_2!=0){
            Expression* subExpression = new Dist(ptr_query_1, ptr_query_2, km, for_what);
            expressions.push_back( subExpression );
          } else {
            return 10;
          }
        } else if(expressions.size() == 1){
          Query* ptr_query_1 = dynamic_cast<Query*>(expressions.back()); expressions.pop_back();
          if(ptr_query_1!=0){
            Expression* subExpression = new Dist(ptr_query_1, NULL, km, for_what);
            expressions.push_back( subExpression );
          } else {
            return 11;
          }
        } else { cerr << "Error: When dist is reached expression stack should be of size 1 or 2" << endl; return 12; }
      }
      prev_value = current_value; //needed to find or/and(keyval)
    }
    if(expressions.size() != 1){ return 13; }
    syntaxTree = expressions.back(); expressions.pop_back();
    return 0;
  }

  string interpret(NLmaps_query* ptr_query_info) {
    if(syntaxTree==NULL){
      cerr << "Warning: Something went wrong, interpret() cannot be called for: " << ptr_query_info->mrl << endl;
      return "";
    }
    return syntaxTree->interpret(ptr_query_info);
  }
  string class_name(){ return "Evaluator"; }
};


#endif
