#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <samplerate.h>

#include <iostream>
#include <fstream>

#include <algorithm>


#include <sys/types.h>
#include <dirent.h>

#include "kits.h"

using namespace std;



std::vector <std::string> files_get_list (const std::string &path) //ext with dot: ".txt"
{
  DIR *directory;
  struct dirent *dir_entry;

  std::vector <std::string> result;

  directory = opendir(path.c_str());
  if (! directory)
     {
      closedir (directory);
      return result;
     }

  while ((dir_entry = readdir (directory)))
        {
          // std::cout << dir_entry->d_name << std::endl;
         std::string t = dir_entry->d_name;

         if (t != "." && t != "..")
             result.push_back (path + "/" + t);
        }

  closedir (directory);
  return result;
}





std::string get_home_dir()
{
  std::string result;

#if !defined(_WIN32) || !defined(_WIN64)

  const char *homedir = getenv ("HOME");

  if (homedir != NULL)
     result = homedir;

#else

  char homeDirStr[MAX_PATH];

 if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, homeDirStr)))
   result = homeDirStr;

#endif

  return result;
}



std::string get_file_path (const std::string &path)
{
  size_t i = path.rfind ("/", path.length());

  if (i != std::string::npos)
     return path.substr (0, i);

  return std::string();
}


std::string string_file_load (const string &fname)
{
 if (fname.empty())
    return string();

 std::ifstream t (fname.c_str());
 std::string s ((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

 return s;
}


float* CDrumLayer::load_whole_sample (const char *fname)
{
  SNDFILE *file = sf_open (fname, SFM_READ, &info);

  if (! file)
     return NULL;

  if (info.channels == 0 || info.frames == 0)
     return NULL;

  float *buffer = new float [info.channels * info.frames];
  sf_count_t zzz = sf_readf_float (file, buffer, info.frames);
  sf_close (file);

  return buffer;
}


float* CDrumLayer::load_whole_sample_resampled (const char *fname, int samplerate)
{
   float *buffer = load_whole_sample (fname);
   if (! buffer)
      {
       cout << "load error: " << fname << endl;
       return 0;
      }

   if (info.samplerate == samplerate)
      {
       samples_count = info.channels * info.frames;
       std::cout << fname << " loaded\n";
       return buffer;
      }

  float ratio = (float) 1.0f * samplerate / info.samplerate;

  size_t output_frames_count = (size_t) floor (info.frames * ratio);
  size_t output_samples_count = output_frames_count * info.channels;

  float *new_buffer = new float [output_samples_count];

  SRC_DATA data;
  data.src_ratio = ratio;

  data.input_frames = info.frames;
  data.output_frames = output_frames_count;

  data.data_in = buffer;
  data.data_out = new_buffer;

  int error = src_simple (&data, SRC_SINC_BEST_QUALITY, info.channels);
  if (error)
     {
      delete buffer;
      return 0;
     }


  info.samplerate = samplerate;
  info.frames = output_frames_count;
  samples_count = info.channels * info.frames;

  std::cout << fname << " loaded and resampled to " << samplerate << endl;

  delete [] buffer;
  return new_buffer;
}



void CDrumLayer::load (const char *fname)
{
  data = load_whole_sample_resampled (fname, samplerate);
  file_name = fname;
}


CDrumLayer::CDrumLayer (int sample_rate)
{
  samplerate = sample_rate;
  offset = 0;
  dataoffset = 0;
  data = 0;
}


void CDrumLayer::print()
{
  cout << "sample layer -- start" << endl;
  cout << "file_name: " << file_name << endl;
  cout << "min: " << min << endl;
  cout << "max: " << max << endl;


  cout << "sample layer -- end"  << endl;

}


CDrumLayer::~CDrumLayer()
{
  if (data)
      delete [] data;
}


CDrumSample::CDrumSample (int sample_rate)
{
  samplerate = sample_rate;
  current_layer = 0;
  velocity = 0.0;
  //dataoffset = 0;

  //pan = 0;
  //gain = 0;


}


CDrumSample::~CDrumSample()
{
  for (size_t i = 0; i < v_layers.size(); i++)
      {
       delete v_layers[i];
      }
}

/*


static inline void layer_to_sample (drmr_sample *sample, float gain)
{
  float mapped_gain = (1 - (gain / GAIN_MIN));

  if (mapped_gain > 1.0f)
      mapped_gain = 1.0f;

  for (int i = 0; i < sample->layer_count; i++)
      {
       if (sample->layers[i].min <= mapped_gain &&
          (sample->layers[i].max > mapped_gain ||
          (sample->layers[i].max == 1 && mapped_gain == 1)))
          {
           sample->limit = sample->layers[i].limit;
           sample->info = sample->layers[i].info;
           sample->data = sample->layers[i].data;
           return;
          }
       }

  fprintf (stderr, "Couldn't find layer for gain %f in sample\n\n", gain);

  // to avoid not playing something, and to deal with kits like the
     //k-27_trash_kit, let's just use the first layer
  sample->limit = sample->layers[0].limit;
  sample->info = sample->layers[0].info;
  sample->data = sample->layers[0].data;
}


 */

#define GAIN_MIN -60.0f


size_t CDrumSample::map_gain_to_layer_number (float gain)
{
  if (v_layers.size() == 1)
     return 0;

  size_t result = 0;
  float mapped_gain = (1 - (gain / GAIN_MIN));

  if (mapped_gain > 1.0f)
      mapped_gain = 1.0f;

  for (size_t i = 0; i < v_layers.size(); i++)
      {
       if (v_layers[i]->min <= mapped_gain &&
          (v_layers[i]->max > mapped_gain ||
          (v_layers[i]->max == 1 && mapped_gain == 1)))
          {
           result = i;
           break;
          }
       }

  return result;
}



void CDrumSample::add_layer()
{
  CDrumLayer *l = new CDrumLayer (samplerate);
  v_layers.push_back (l);
}


void CDrumSample::print()
{
  cout << "CDrumSample -- start"  << endl;

  cout << "id: " << id << endl;
  cout << "name: " << name << endl;
  cout << "midiOutNote: " << midiOutNote << endl;


  for (auto l: v_layers)
      {
       l->print();
      }

  cout << "CDrumSample -- end"  << endl;


}




void CDrumSample::print_stats()
{

//  cout << "id: " << id << endl;
  cout << "name: " << name << endl;
 // cout << "midiOutNote: " << midiOutNote << endl;


}




CHydrogenXMLWalker::CHydrogenXMLWalker (CHydrogenKit *hkit)
{
  kit = hkit;

  drumkit_info_passed = false;
  drumkitComponent_passed = false;
}




bool CHydrogenXMLWalker::for_each (pugi::xml_node &node)
{
  string node_name = node.name();
  pugi::xml_text txt = node.text();

   //if (node_name == "drumkit_info")
     // drumkit_info_passed = true;

  if (node_name == "drumkitComponent")
     drumkit_info_passed = true;

  if (node_name == "instrument")
     drumkitComponent_passed = true;

  if (node_name == "name" && ! drumkit_info_passed)
     kit->name = txt.as_string();

  if (node_name == "name" && drumkit_info_passed && drumkitComponent_passed)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->name = txt.as_string();

  if (node_name == "id" && drumkit_info_passed && drumkitComponent_passed && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->id = txt.as_int();

  if (node_name == "min" && drumkit_info_passed && drumkitComponent_passed && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->v_layers.back()->min = txt.as_float();

  if (node_name == "max" && drumkit_info_passed && drumkitComponent_passed  && ! kit->scan_mode)
     if (kit->v_samples.size() != 0)
         kit->v_samples.back()->v_layers.back()->max = txt.as_float();


  if (node_name == "instrument")
     {
      drumkit_info_passed = true;

      kit->add_sample();

      if (! kit->layers_supported) //non-layered
         kit->v_samples.back()->add_layer(); //add default layer
     }


  if (node_name == "layer" && ! kit->scan_mode)
     {
      if (kit->v_samples.size() != 0)
          kit->v_samples.back()->add_layer();

     }

  if (node_name == "filename" && ! kit->scan_mode)
     {
      std::string fname = txt.as_string();
      std::string path = kit->kit_dir + "/" + fname;

      if (kit->v_samples.size() != 0)
          if (kit->v_samples.back()->v_layers.size() != 0)
                kit->v_samples.back()->v_layers.back()->load (path.c_str());
     }


  return true;
}


void CHydrogenKit::load (const char *fname, int sample_rate)
{
   samplerate = sample_rate;

  //path = fname;

   kit_xml_filename = fname;
   kit_dir = get_file_path (kit_xml_filename);


  pugi::xml_document doc;
  //pugi::xml_parse_result result = doc.load_buffer (temp.utf16(),
    //                                               temp.size() * 2,
      //                                             pugi::parse_default,
        //                                           pugi::encoding_utf16);

//  load_buffer(const void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);


 // pugi::xml_parse_result result = doc.load_file (fname);

  std::string source = string_file_load (fname);

  cout << "loading kit: " << fname << endl;
  //cout << "source: " << source << endl;


  size_t r = source.find ("<layer>");
  if (r != std::string::npos)
     layers_supported = true;
   else
       layers_supported = false;


  pugi::xml_parse_result result = doc.load_buffer (source.c_str(), source.size());


  if (! result)
     return;

   CHydrogenXMLWalker walker (this);

   doc.traverse (walker);

}


CHydrogenKit::CHydrogenKit()
{
  scan_mode = false;
}


CHydrogenKit::~CHydrogenKit()
{
  for (size_t i = 0; i < v_samples.size(); i++)
      {
       delete v_samples[i];
      }

}


void CHydrogenKit::add_sample()
{

  CDrumSample *s  = new CDrumSample (samplerate);
  v_samples.push_back (s);

}


void CHydrogenKit::print()
{
  cout << "void CHydrogenKit::print() -- start" << endl;

  for (size_t i = 0; i < v_samples.size(); i++)
      {
       v_samples[i]->print();
      }


  cout << "void CHydrogenKit::print() -- end" << endl;


}

void CHydrogenKit::print_stats()
{
  cout << "void CHydrogenKit::print-stats() -- start" << endl;

  cout << "kitname: " << name << endl;

  for (size_t i = 0; i < v_samples.size(); i++)
      {
       v_samples[i]->print_stats();
      }


  cout << "void CHydrogenKit::print-stats() -- end" << endl;


}



CHydrogenKits::CHydrogenKits()
{
//  scan();
}


CHydrogenKits::~CHydrogenKits()
{

}


std::string CHydrogenKits::get_kit_name (const std::string full_path)
{
  std::string s = string_file_load (full_path);

  size_t start_pos = s.find ("<name>");

  if (start_pos == std::string::npos)
     return std::string();

   start_pos += 6;

  size_t end_pos = s.find ("</name>", start_pos);
  if (end_pos == std::string::npos)
     return std::string();

  return s.substr (start_pos, end_pos - start_pos);
}



void CHydrogenKits::scan()
{
  std::vector <std::string> v_kits_locations;

  v_kits_locations.push_back ("/usr/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/local/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/share/drmr/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drmr/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drumrox/drumkits");

  std::vector <std::string> v_kits_dirs;

  for (std::string i :v_kits_locations)
      {
       std::vector <std::string> v_kits_dirs_t = files_get_list (i);
       v_kits_dirs.insert(v_kits_dirs.end(), v_kits_dirs_t.begin(), v_kits_dirs_t.end());
      }


  std::sort (v_kits_dirs.begin(), v_kits_dirs.end());
  v_kits_dirs.erase (std::unique( v_kits_dirs.begin(), v_kits_dirs.end() ), v_kits_dirs.end() );



  for (std::string kd :v_kits_dirs)
      {
       //cout << kd << endl;
       //cout << get_kit_name (kd + "/drumkit.xml") << endl;

       std::string kit_name = get_kit_name (kd + "/drumkit.xml");

       if (! kit_name.empty())
          {
           m_kits.insert (pair<string,string> (kit_name, kd));
           v_kits_names.push_back (kit_name);
          }
      }

}


void CHydrogenKits::print()
{

 for (auto it = m_kits.begin(); it != m_kits.end(); ++it )
     {
  cout << it->first; // key
  string& value = it->second;
  cout << ":" << value << endl;
    }
}



CHydrogenKitsScanner::CHydrogenKitsScanner()
{
//  scan();

}


CHydrogenKitsScanner::~CHydrogenKitsScanner()
{
  for (size_t i = 0; i < v_scanned_kits.size(); i++)
      {
       delete v_scanned_kits[i];
      }
}




void CHydrogenKitsScanner::scan()
{


  std::vector <std::string> v_kits_locations;

  v_kits_locations.push_back ("/usr/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/local/share/hydrogen/data/drumkits");
  v_kits_locations.push_back ("/usr/share/drmr/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drmr/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drumrox/drumkits");

  std::vector <std::string> v_kits_dirs;

  for (std::string i : v_kits_locations)
      {
       std::vector <std::string> v_kits_dirs_t = files_get_list (i);
       v_kits_dirs.insert(v_kits_dirs.end(), v_kits_dirs_t.begin(), v_kits_dirs_t.end());
      }


  std::sort (v_kits_dirs.begin(), v_kits_dirs.end());
  v_kits_dirs.erase (std::unique( v_kits_dirs.begin(), v_kits_dirs.end() ), v_kits_dirs.end() );


  for (std::string kd : v_kits_dirs)
      {
       //cout << kd << endl;
       //cout << get_kit_name (kd + "/drumkit.xml") << endl;

       std::string fname = kd + "/drumkit.xml";

       //check for file exists

       CHydrogenKit *kit = new CHydrogenKit;
       kit->scan_mode = true;
       kit->load (fname.c_str(), 44100);
       v_scanned_kits.push_back (kit);
       v_kits_names.push_back (kit->name);

       m_kits.insert (pair<string,string> (kit->name, fname));


      }

}


void CHydrogenKitsScanner::print()
{
  for (size_t i = 0; i < v_scanned_kits.size(); i++)
     {
      std::cout << i << ": ";
      v_scanned_kits[i]->print_stats();
     }


}