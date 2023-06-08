/*
written at 2023 by Peter Semiletov
this code is the public domain
 */

#ifndef KITS_H
#define KITS_H

#include <vector>
#include <map>

#include <stdint.h>
#include <string.h>


#include "pugixml.hpp"


class CDrumLayer
{
public:

  int session_samplerate; //uplink (session) samplerate

  //for mapped gain
  float min;
  float max;

  std::string file_name; //name of the loaded file

  int channels;
  int frames;
  int samplerate;

  uint32_t samples_count;  //data size in samples (info.frames * info.channels)

  float* data; //interleaved sample data from the file

  uint32_t offset;
  int dataoffset;

  CDrumLayer (int sample_rate); //sample_rate is uplink (session) samplerate
  ~CDrumLayer();

  void load (const char *fname); //loads the sample, sets internally info, data, file_name
  void print();

  float* load_whole_sample (const char *fname); //called from load_whole_sample_resampled
  float* load_whole_sample_resampled (const char *fname, int sess_samplerate); //called from load
};


class CDrumSample
{
public:

  std::string name;
  int id;
  int current_layer;
  float volume;
  int midiOutNote;
  int session_samplerate; //session srate, taken from the upper object
  char active;
  float velocity;

  std::vector <CDrumLayer*> v_layers;

  CDrumSample (int sample_rate);
  ~CDrumSample();

  size_t map_gain_to_layer_number (float gain);

  void add_layer();

  void print();
  void print_stats();
};



class CHydrogenKit
{
public:

  bool scan_mode; //if false, we do not load kit' samples

  std::string kit_name; //parsed from XML
  std::string kit_xml_filename; //full path to the kit xml file
  std::string kit_dir; //full path to the kit

  int samplerate; //session srate

  bool layers_supported;

  std::vector <CDrumSample*> v_samples;

  void add_sample();
  void load (const char *fname, int sample_rate);
  void load_txt (std::string fname);

  CHydrogenKit();
  ~CHydrogenKit();

  void print();
  void print_stats();
};

/*
class CHydrogenKits
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;

  std::map <std::string, std::string> m_kits; //var=value i.e. name = full path

  CHydrogenKits();
  ~CHydrogenKits();

  std::string get_kit_name (const std::string full_path); //get kit name for full path

  void scan();
  void print();

};
*/

class CHydrogenKitsScanner
{
public:

  std::vector <std::string> v_kits_dirs;
  std::vector <std::string> v_kits_names;
  std::vector <CHydrogenKit*> v_scanned_kits;
  std::map <std::string, std::string> m_kits; //name = full path

  CHydrogenKitsScanner();
  ~CHydrogenKitsScanner();

  void scan();
  void print();
};



class CHydrogenXMLWalker: public pugi::xml_tree_walker
{
public:

  CHydrogenKit *kit;

  bool is_drumkit_info;
  bool is_instrument;
  bool is_layer;
  bool drumkit_info_passed;
  bool drumkitComponent_passed;

  CHydrogenXMLWalker (CHydrogenKit *hkit);
  bool for_each (pugi::xml_node& node);
};


#endif

