#include "nlmaps_query.h"
#include "interpret.cc"
#include "linearise.cc"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "boost/program_options.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace std;

int main(int argc, char** argv) {
  try {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "Print help messages")
      ("file,f", po::value<string>()->required(), "Query file's (total) input path")
      ("db_dir,d", po::value<string>()->required(), "(Full) database directory path")
      ("answer,a", po::value<string>()->required(), "Answer file's (total) output path")
      ("latlong,l", po::value<string>()->default_value(""), "Latlong file's (total) output path")
      ("geojson,g", po::bool_switch()->default_value(false), "The latlong file will contain geojson information instead, needed for the graphical user interface")
      ("reference,e", po::value<string>()->default_value(""), "Supply a reference point separately");

    po::variables_map vm;
    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);
      if(vm.count("help")) {
        cout << desc << endl;
        return 0;
      }
      po::notify(vm);
    }  catch(po::error& e) {
      cerr << "ERROR: " << e.what() << endl << endl;
      cerr << desc << endl;
      return 1;
    }

    string db_dir = vm["db_dir"].as<string>();
    ifstream query_file(vm["file"].as<string>());

    if (!query_file.is_open()){
      cerr << "The following file does not exist" << vm["file"].as<string>() << endl;
      exit (EXIT_FAILURE);
    }

    ofstream outfile_answer;
    outfile_answer.open(vm["answer"].as<string>());
    if(!outfile_answer.is_open()){
      cerr << "The following file cannot be opened for writing" << vm["answer"].as<string>() << endl;
      exit (EXIT_FAILURE);
    }

    ofstream outfile_latlong;
    if(vm["latlong"].as<string>()!=""){
      outfile_latlong.open(vm["latlong"].as<string>());
      if(!outfile_latlong.is_open()){
        cerr << "The following file cannot be opened for writing" << vm["answer"].as<string>() << endl;
        exit (EXIT_FAILURE);
      }
    }

    vector<string> references;
    if(vm["reference"].as<string>()!=""){
      ifstream input_reference(vm["reference"].as<string>());
      if(!input_reference.is_open()){
        cerr << "The following file cannot be opened for reading" << vm["reference"].as<string>() << endl;
        exit (EXIT_FAILURE);
      }
      string ref;
      while(getline(input_reference, ref)){
        references.push_back(ref);
      }
    }

    int size = references.size();
    string mrl;
    int counter = -1;
    while(getline(query_file, mrl)){
      counter++;
      if(size>counter){//then we have a reference point for this query
        if(references[counter]==""){
          ;
        } else {
          boost::replace_all(mrl, "area(ref)", "area(keyval('name','<nom>"+references[counter]+"</nom>'))");
        }
      }
      NLmaps_query query_info;
      query_info.mrl = mrl;
      preprocess_mrl(&query_info);
      bool geojson = vm["geojson"].as<bool>();
	  Evaluator mrl_eval(db_dir);
      int init_return = mrl_eval.initalise(&query_info, geojson);
      if(init_return != 0){
        cerr << "Warning: Failed to initialise the following query with error code " << init_return << ": " << mrl << endl;
        outfile_answer << "" << endl; //if failed, then empty answer
        if(outfile_latlong.is_open()){
          outfile_latlong << "" << endl;
        }
        continue;
      }
      string answer = mrl_eval.interpret(&query_info);
      //collect geojson
      stringstream ss_latlong;
      stringstream ss_pre_latlong;
      double center_lat = 0.0;
      double center_lon = 0.0;
      bool first = true;
      if(geojson){
        if(query_info.findkey && answer == ""){ answer = "We couldn't seem to find an object that fullfills your requirements.<br/>We are wrong? Let us know via feedback!"; }
        if(query_info.latlon && answer == " and "){ answer = ""; }
        if(query_info.latlon && !query_info.dist){
          if(query_info.elements.size() == 0){
            answer = "We couldn't seem to find an object that fullfills your requirements.<br/>We are wrong? Let us know via feedback!";
          } else  {
            answer = "Your objects of interest are marked below";  //it might not be empty yet, e.g. if we had an and query.
          }
        }
        ss_latlong << "\"features\": [";
        int id = 0;
        for(auto it = query_info.elements.begin(); it != query_info.elements.end(); it++, id++){
          center_lat += it->lat_lon.first;
          center_lon += it->lat_lon.second;
          if(first){
            first = false;
          } else {
            ss_latlong << ",";
          }
          //lat and lon need to be revered for geojson
          //pretty up the it->value in the same way as the answer, so that appropriate popups can be found
          string& element_values = it->value;
          boost::replace_all(element_values, ";", ", ");
          boost::replace_all(element_values, ", ", ","); //there is some cases where there is no space after the comma, these two lines fix that
          boost::replace_all(element_values, ",", ", ");
          boost::replace_all(element_values, "_", " ");
          boost::replace_all(element_values, "\"", "\\\"");
          string& element_values_nopretty = it->value;
          boost::replace_all(element_values_nopretty, "\"", "\\\"");
          string element_name = it->name;
          boost::replace_all(element_name, "\"", "\\\"");//else there are errors in the json

          if(it->name==""){ it->name = "<i>Unnamed</i>"; }
          ss_latlong << "{";
          ss_latlong<<"\"osm_id\": \"" << it->osm_type << it->osm_id_number << "\",";
          ss_latlong<<"\"search_word\": \"" << element_values << "\",";
          ss_latlong << "\"type\": \"Feature\",\"properties\": {\"popupContent\": \"<b>"<<element_name<<"</b>";
          if(element_values_nopretty != it->name && element_values_nopretty!=""){ ss_latlong<<"<br/><i>"<<element_values_nopretty<<"</i>"; }
          ss_latlong<<"<br/><b>lat</b> "<<it->lat_lon.first<<" <b>lon</b> " <<it->lat_lon.second;
          if(it->detailed_info.size() > 0){
            ss_latlong<<"<br/><i>";
            bool first_it2 = true;
            for(auto it2 = it->detailed_info.begin(); it2 != it->detailed_info.end(); it2++){
              boost::replace_all((*it2), "_", " ");
              if(first_it2){
                first_it2 = false;
              } else {
                ss_latlong<<", ";
              }
              ss_latlong<<(*it2);
            }
            ss_latlong<<"</i>";
          }

          ss_latlong << "\"},\"geometry\": {\"type\": \"Point\",\"coordinates\": ["
            <<fixed<<setprecision(7)<<it->lat_lon.second<<", "<<fixed<<setprecision(7)<<it->lat_lon.first<<"]}}";
        }
        center_lat = center_lat / query_info.elements.size();
        center_lon = center_lon / query_info.elements.size();
        ss_latlong << "]";
        //we note latlong here because we don't want the gps coordinates to be printed in the answer box
        bool latlon = 0;
        if(query_info.latlon){
          latlon = 1;
        }
        ss_pre_latlong << "{";
        //information for feedback form
        stringstream ss_location;
        stringstream ss_osm_tags;
        stringstream ss_qtype;
        bool first = true;
        for(auto it = query_info.osm_tags.begin(); it != query_info.osm_tags.end(); it++){
          if(!first){
            ss_osm_tags << "; ";
          }
          ss_osm_tags << (*it);
          first = false;
        }
        first = true;
        for(auto it = query_info.area_name.begin(); it != query_info.area_name.end(); it++){
          if(!first){
            ss_location << "; ";
          }
          ss_location << (*it);
          first = false;
        }
        if(query_info.latlon){ ss_qtype << "latlong, "; }
        if(query_info.findkey){ ss_qtype << "findkey, "; }
        if(query_info.count){ ss_qtype << "count, "; }
        if(query_info.least){ ss_qtype << "least, "; }
        if(query_info.dist){ ss_qtype << "dist, "; }
        string qtype = ss_qtype.str();
        qtype = qtype.substr(0, qtype.size()-2);
        string location = ss_location.str();
        string osm_tags = ss_osm_tags.str();
        boost::replace_all(location, "\"", "");
        boost::replace_all(osm_tags, "\"", "");
        boost::replace_all(query_info.query, "\"", "");
        boost::replace_all(query_info.mrl, "'", "");
        ss_pre_latlong << "\"location\": \""<<location<<"\",\"qtype\": \""<<qtype<<"\",\"osmtags\": \""<<osm_tags<<"\",";
        ss_pre_latlong << "\"overpass\": \""<<query_info.query<<"\",\"mrl\": \""<<query_info.mrl<<"\",";
        if(query_info.findkey){ //because we only want hyperlinks for finkey requests
          ss_pre_latlong << "\"findkey\": \"yes\",";
        } else {
          ss_pre_latlong << "\"findkey\": \"no\",";
        }
        ss_pre_latlong << "\"correctly_empty\": \""<<latlon<<"\",\"lat\": \""<<center_lat<<"\",\"lon\": \""<<center_lon<<"\"," << ss_latlong.str() << "}";
      } else {
        // get latlong
        bool first = true;
        for(auto it = query_info.elements.begin(); it != query_info.elements.end(); it++){
          if(first){
            first = false;
            ss_latlong << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
          } else {
            ss_latlong << ", " << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
          }
        }
      }
      
	  if(outfile_latlong.is_open()){
        outfile_latlong << ss_latlong.str() << endl;
        outfile_latlong.close();
      }

      outfile_answer << answer << endl;
    }

    outfile_answer.close();

  }
  catch(exception& e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}
