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
      ("latlong,l", po::value<string>()->default_value(""), "Latlong file's (total) output path");

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

    string mrl;
    while(getline(query_file, mrl)){
      if(mrl == "no mrl found"){
        outfile_answer << "empty" << endl;
        outfile_latlong << "empty" << endl;
        continue;
      }
      NLmaps_query query_info;
      query_info.mrl = mrl;
      preprocess_mrl(&query_info);

			Evaluator mrl_eval(db_dir);
      int init_return = mrl_eval.initalise(&query_info);
      if(init_return != 0){
        cerr << "Warning: Failed to initialise the following query with error code " << init_return << ": " << mrl << endl;
        outfile_answer << "" << endl; //if failed, then empty answer
        if(outfile_latlong.is_open()){
          outfile_latlong << "" << endl;
        }
        continue;
      }
      string answer = mrl_eval.interpret(&query_info);
      outfile_answer << answer << endl;
      //always get latlong
      stringstream ss_latlong;
      bool first = true;
      for(auto it = query_info.elements.begin(); it != query_info.elements.end(); it++){
        if(first){
          first = false;
          ss_latlong << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
        } else {
          ss_latlong << ", " << fixed<<setprecision(7)<<it->lat_lon.first << " " << fixed<<setprecision(7)<<it->lat_lon.second;
        }
      }
      if(outfile_latlong.is_open()){
        outfile_latlong << ss_latlong.str() << endl;
      }
    }

    outfile_answer.close();

  }
  catch(exception& e) {
    cerr << e.what() << endl;
    return 1;
  }

  return 0;
}
