#ifndef KITS_H
#define KITS_H

#include <vector>
#include <map>

#include <stdint.h>
#include <sndfile.h>
#include <string.h>


#include "pugixml.hpp"

// drumkit scanned from a hydrogen xml file
/*
typedef struct
{
  char* name;
  char* desc;
  char* path;
  char** sample_names;
  int samples;
} scanned_kit;


typedef struct
{
  int num_kits;
  scanned_kit* kits;
} s_kits;
*/
// libsndfile stuff

class CDrumLayer
{
public:

  float min;
  float max;

  int samplerate;

  uint32_t offset;
  int dataoffset;


  std::string file_name;

  //SF_INFO *info;
  SF_INFO info;
  //was: uint32_t limit;  //data size in samples (frames * channels)
  uint32_t samples_count;  //data size in samples (frames * channels)

  float* data; //interleaved sample data

//  CDrumLayer (const char *fname, int sample_rate);
  CDrumLayer (int sample_rate);
  void load (const char *fname);
  void print();

  ~CDrumLayer();

  float* load_whole_sample (const char *fname);
  float* load_whole_sample_resampled (const char *fname, int samplerate);

};


class CDrumSample//was drmr_sample
{
public:

  std::string name;
  int id;

  int current_layer;

  //pointers to LV's pan and gain
  //float *pan;
  //float *gain;

  float volume;
  int midiOutNote;

  int samplerate; //session srate, taken from the upper object


  char active;
  //uint32_t offset; //WHAT IS IT?


  //  uint32_t limit;
 // uint32_t samples_count; //SET TO WHAT? first layer?

  //uint32_t layer_count;
  float velocity;
  //int dataoffset;

  //drmr_layer *layers;
  std::vector <CDrumLayer*> v_layers;

//  float* data;

  CDrumSample (int sample_rate);
  ~CDrumSample();

  size_t map_gain_to_layer_number (float gain);

//  void add_layer (const char *fname);
  void add_layer();

  void print();
  void print_stats();

};



class CHydrogenKit
{
public:

  bool scan_mode; //if false, we do not load kit' samples

  std::string name;
 // std::string path;

  int samplerate; //session srate

  bool layers_supported;

  std::string kit_name; //parsed from XML
  std::string kit_xml_filename; //full path to the kit xml file
  std::string kit_dir; //full path to the kit

  std::vector <CDrumSample*> v_samples;

  void add_sample();


//  CHydrogenKit (const char *fname, int sample_rate);

  void load (const char *fname, int sample_rate);

  CHydrogenKit();

  ~CHydrogenKit();

  void print();
  void print_stats();

};


class CHydrogenKits
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;

  std::map <std::string, std::string> m_kits; //name = full path

  CHydrogenKits();
  ~CHydrogenKits();

  std::string get_kit_name (const std::string full_path); //get kit name for full path

  void scan();
  void print();

};


class CHydrogenKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;

  std::vector <CHydrogenKit*> v_scanned_kits;

  std::map <std::string, std::string> m_kits; //name = full path

  CHydrogenKitsScanner();
  ~CHydrogenKitsScanner();

//  std::string get_kit_name (const std::string full_path);

  void scan();
  void print();
};




class CHydrogenXMLWalker: public pugi::xml_tree_walker
{
public:

  CHydrogenKit *kit;

  CHydrogenXMLWalker (CHydrogenKit *hkit);

  bool is_drumkit_info;
  bool is_instrument;
  bool is_layer;

  bool drumkit_info_passed;
  bool drumkitComponent_passed;

  bool for_each (pugi::xml_node& node);

};



#endif

