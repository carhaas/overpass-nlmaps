#ifndef LINEARISE_CC
#define	LINEARISE_CC

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "nlmaps_query.h"

using namespace std;

static void preprocess_mrl(NLmaps_query* query_info){
	string mrl = query_info->mrl;
  Tree& mrl_tree = query_info->mrl_tree;
  map<int, vertex>& map_string2vertex = query_info->map_string2vertex;

  boost::trim(mrl);
  //sequence of characters that does not contain ( or ) : [^\\(\\)]
  mrl = boost::regex_replace(mrl, boost::regex(",' *([^\\(\\)]*?)\\((.*?) *'\\)"),",'$1BRACKETOPEN$2')"); //protect open brackets ( that occur in values
  mrl = boost::regex_replace(mrl, boost::regex(",' *([^\\(\\)]*?)\\)([^\\(\\)]*?) *'\\)"),",'$1BRACKETCLOSE$2')"); //protect close brackets ) that occur in values
  boost::replace_all(mrl, " ", "SAVESPACE"); //protect space
  mrl = boost::regex_replace(mrl, boost::regex("(?<=([^,\\(\\)]))'(?=([^,\\(\\)]))"), "SAVEAPO"); //protect apostrophe '
  mrl = boost::regex_replace(mrl, boost::regex("\\s+"), " ");
  mrl = boost::regex_replace(mrl, boost::regex("'"), "");
  string mrl_just_words = mrl;
  boost::replace_all(mrl_just_words, "(", " ");
  boost::replace_all(mrl_just_words, ")", " ");
  boost::replace_all(mrl_just_words, ",", " ");
  mrl_just_words = boost::regex_replace(mrl_just_words, boost::regex("\\s+"), " ");
  boost::trim(mrl_just_words);

  map<string, int> seen;
  vector<string> elements;
  int count = 0;
  boost::split(elements, mrl_just_words, boost::is_any_of(" "));
  for(vector<string>::iterator it = elements.begin(); it != elements.end(); ++it, ++count) {
    vertex v = boost::add_vertex(mrl_tree);
    mrl_tree[v].value = *it;
    mrl_tree[v].index = count;
		map_string2vertex[count] = v;
  }
	query_info->number_vertices = count-1;
  string save_mrl = mrl;
  count = 0;
  pair<vertex_iter, vertex_iter> v_iter;
  for (v_iter = vertices(mrl_tree); v_iter.first != v_iter.second; ++v_iter.first, ++count){
    int inner_count = count;
    string current_value = mrl_tree[*v_iter.first].value;
		int current_index = mrl_tree[*v_iter.first].index;
    size_t found = mrl.find(current_value);
    if(found==string::npos){
      cerr << "Warning: Something went wrong in this mrl: " << save_mrl << endl;
      break;
    }
    mrl = mrl.substr(found);
  	bool args = false;
  	int parens = 0;
  	int commas = 0 ;
  	for(size_t i = 0; i < mrl.size(); ++i){
  		const char& c = mrl.at(i);
  		if(c == '('){
        inner_count++;
        if(args == false){ //first child
					add_edge(map_string2vertex[current_index], map_string2vertex[inner_count], mrl_tree);
        }
  			args = true;
  			parens += 1;
  		} else if(c == ')'){
  			parens -= 1;
  		} else if(parens == 1 && c == ','){//further children
        inner_count++;
				add_edge(map_string2vertex[current_index], map_string2vertex[inner_count], mrl_tree);
  			commas += 1;
  		} else if(parens < 1 && c == ','){
  			break;
  		} else if(c == ','){
        inner_count++;
			}
  	}
    mrl = mrl.substr(current_value.size());
  }
}

#endif
