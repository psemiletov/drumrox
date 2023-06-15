/*
written at 2023 by Peter Semiletov
this code is the public domain
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>


#include <samplerate.h>
#include <sndfile.h>


#include "kits.h"
#include "utl.h"


using namespace std;



float* CDrumLayer::load_whole_sample (const char *fname)
{
  SF_INFO info;

  SNDFILE *file = sf_open (fname, SFM_READ, &info);

  if (! file)
     return NULL;

  if (info.channels == 0 || info.frames == 0)
     return NULL;

  float *buffer = new float [info.channels * info.frames];
  sf_count_t zzz = sf_readf_float (file, buffer, info.frames);
  sf_close (file);

  samplerate = info.samplerate;
  frames = info.frames;
  samples_count = info.channels * info.frames;
  channels = info.channels;

  return buffer;
}


float* CDrumLayer::load_whole_sample_resampled (const char *fname, int sess_samplerate)
{
   float *buffer = load_whole_sample (fname);
   if (! buffer)
      {
       cout << "load error: " << fname << endl;
       return 0;
      }

   if (samplerate == sess_samplerate)
       return buffer;

  float ratio = (float) 1.0f * sess_samplerate / samplerate;

  size_t output_frames_count = (size_t) floor (frames * ratio);
  size_t output_samples_count = output_frames_count * channels;

  float *new_buffer = new float [output_samples_count];

  SRC_DATA data;

  data.src_ratio = ratio;
  data.input_frames = frames;
  data.output_frames = output_frames_count;
  data.data_in = buffer;
  data.data_out = new_buffer;

  int error = src_simple (&data, SRC_SINC_BEST_QUALITY, channels);
  if (error)
     {
      delete buffer;
      return 0;
     }

  samplerate = sess_samplerate;
  frames = output_frames_count;
  samples_count = channels * frames;

  std::cout << fname << " loaded and resampled to " << samplerate << endl;

  delete [] buffer;
  return new_buffer;
}


void CDrumLayer::load (const char *fname)
{
  data = load_whole_sample_resampled (fname, session_samplerate);
  file_name = fname;
}


CDrumLayer::CDrumLayer (int sample_rate)
{
  session_samplerate = sample_rate;
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
  session_samplerate = sample_rate;
  current_layer = 0;
  velocity = 0.0;

  hihat = false;
  hihat_open = false;
  hihat_close = false;

}


CDrumSample::~CDrumSample()
{
  for (size_t i = 0; i < v_layers.size(); i++)
      {
       delete v_layers[i];
      }
}



#define GAIN_MIN -60.0f

size_t CDrumSample::map_gain_to_layer_number (float gain)
{
  if (v_layers.size() == 1)
     return 0; //return zero pos layer if we have just one layer

  size_t result = 0;

  float mapped_gain = (1 - (gain / GAIN_MIN));

  if (mapped_gain > 1.0f)
      mapped_gain = 1.0f;

  //search for layer within its min..max gain
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
  CDrumLayer *l = new CDrumLayer (session_samplerate);
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
     kit->kit_name = txt.as_string();

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

      if (findStringIC (fname, "hihat"))
         kit->v_samples.back()->hihat = true;

      if (findStringIC (fname, "open"))
         kit->v_samples.back()->hihat_open = true;

      if (findStringIC (fname, "close"))
         kit->v_samples.back()->hihat_close = true;


      if (kit->v_samples.size() != 0)
          if (kit->v_samples.back()->v_layers.size() != 0)
                kit->v_samples.back()->v_layers.back()->load (path.c_str());
     }

/*
  if (node_name == "layer" && ! kit->scan_mode)
     {
      if (kit->v_samples.size() != 0)
          kit->v_samples.back()->add_layer();
     }

  if (node_name == "filename")
     {
      std::string fname = txt.as_string();
      std::string path = kit->kit_dir + "/" + fname;

      if (findStringIC (fname, "hihat"))
         kit->v_samples.back()->hihat = true;

      if (findStringIC (fname, "open"))
         kit->v_samples.back()->hihat_open = true;

      if (findStringIC (fname, "close"))
         kit->v_samples.back()->hihat_close = true;



      if (! kit->scan_mode && kit->v_samples.size() != 0)
          if (kit->v_samples.back()->v_layers.size() != 0)
                kit->v_samples.back()->v_layers.back()->load (path.c_str());
     }
*/


  return true;
}


void CHydrogenKit::load_txt (const std::string data)
{
//  cout << "void CHydrogenKit::load_txt (const std::string data)\n";

  if (data.empty())
      return;

  size_t i = kit_dir.rfind ("/");
  kit_name = kit_dir.substr (i + 1);

  stringstream st (data);
  string line;

  while (getline (st, line))
        {
         if (line.empty())
            continue;

         size_t pos = line.find ("=");

         if (pos == string::npos)
             continue;

         if (pos > line.size())
             continue;

         string sample_name = line.substr (0, pos);
         string filename = line.substr (pos + 1, line.size() - pos);
         filename = kit_dir + "/" + filename;

         add_sample();
         v_samples.back()->name = sample_name;

//         cout << "added sample: " << sample_name << endl;

         v_samples.back()->add_layer(); //add default layer


         if (file_exists (filename) && ! scan_mode)
            v_samples.back()->v_layers.back()->load (filename.c_str());
         }

}


void CHydrogenKit::load (const char *fname, int sample_rate)
{
//  cout << "void CHydrogenKit::load: " << fname << endl;

  samplerate = sample_rate;

  string filename = resolve_symlink (fname);

//  cout << "resolved filename :" << filename << endl;

  kit_xml_filename = filename;
  kit_dir = get_file_path (kit_xml_filename);

  std::string source = string_file_load (filename);
  if (source.empty())
     return;

  if (ends_with (kit_xml_filename, ".txt"))
     {
      load_txt (source);
      return;
     }


  //else Hydrogen format

  pugi::xml_document doc;


 // cout << "loading kit: " << fname << endl;
  //cout << "source: " << source << endl;

  size_t r = source.find ("<layer>");
  if (r != std::string::npos)
     layers_supported = true;
   else
       layers_supported = false;


  //cout << "layers_supported: " << layers_supported  << endl;

  //delete empty instruments
  //because we don't want parse them

  size_t idx_filename = source.rfind ("</filename>");
  size_t idx_instrument = source.find ("<instrument>", idx_filename);

//  cout << "idx_filename: " << idx_filename  << endl;
//  cout << "idx_instrument: " << idx_instrument  << endl;

  if (idx_instrument != std::string::npos)
  if (idx_instrument > idx_filename)
     //oops, we wave empty instruments!
     {
      //первый пустой инструмент у нас уже есть, он находится по
      //idx_instrument

      //теперь найдем конец последнего
      size_t idx_instrument_end = source.rfind ("</instrument>");
      size_t sz_to_remove = idx_instrument_end - idx_instrument + 13;

      source = source.erase (idx_instrument, sz_to_remove);
     }


  pugi::xml_parse_result result = doc.load_buffer (source.c_str(), source.size());

  if (! result)
     return;

   CHydrogenXMLWalker walker (this);

   doc.traverse (walker);
}


CHydrogenKit::CHydrogenKit()
{
  scan_mode = false;
  layers_supported = false;
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

  cout << "samples count:" << v_samples.size() << endl;

  cout << "void CHydrogenKit::print() -- end" << endl;
}


void CHydrogenKit::print_stats()
{
  cout << "void CHydrogenKit::print-stats() -- start" << endl;

  cout << "kitname: " << kit_name << endl;

  for (size_t i = 0; i < v_samples.size(); i++)
      {
       v_samples[i]->print_stats();
      }

  cout << "void CHydrogenKit::print-stats() -- end" << endl;
}

/*
CHydrogenKits::CHydrogenKits()
{
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
  v_kits_locations.push_back ("/usr/share/drumrox/drumkits");

  v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drmr/drumkits");
//  v_kits_locations.push_back (get_home_dir() + "/.drumrox/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/drumrox");

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

*/

CHydrogenKitsScanner::CHydrogenKitsScanner()
{
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
  v_kits_locations.push_back ("/usr/share/drumrox-kits");

  v_kits_locations.push_back (get_home_dir() + "/.hydrogen/data/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/.drmr/drumkits");
//  v_kits_locations.push_back (get_home_dir() + "/.drumrox/drumkits");
  v_kits_locations.push_back (get_home_dir() + "/drumrox-kits");

  std::vector <std::string> v_kits_dirs;

  for (std::string i: v_kits_locations)
      {
       std::vector <std::string> v_kits_dirs_t = files_get_list (i);
       v_kits_dirs.insert (v_kits_dirs.end(), v_kits_dirs_t.begin(), v_kits_dirs_t.end());
      }

  std::sort (v_kits_dirs.begin(), v_kits_dirs.end());
  v_kits_dirs.erase (std::unique (v_kits_dirs.begin(), v_kits_dirs.end() ), v_kits_dirs.end() );

  for (std::string kd: v_kits_dirs)
      {
       //cout << kd << endl;
       //cout << get_kit_name (kd + "/drumkit.xml") << endl;

       std::string fname = kd + "/drumkit.xml";

       if (file_exists (fname))
          {
           //Hydrogen kit
           CHydrogenKit *kit = new CHydrogenKit;
           kit->scan_mode = true;
           kit->load (fname.c_str(), 44100);
           v_scanned_kits.push_back (kit);
//           v_kits_names.push_back (kit->kit_name);

          // m_kits.insert (pair<string,string> (kit->kit_name, fname));
          }

       fname = kd + "/drumkit.txt";
       if (file_exists (fname))
          {
           //Hydrogen kit
           CHydrogenKit *kit = new CHydrogenKit;
           kit->scan_mode = true;
           kit->load (fname.c_str(), 44100);
           v_scanned_kits.push_back (kit);
  //         v_kits_names.push_back (kit->kit_name);

        //   m_kits.insert (pair<string,string> (kit->kit_name, fname));
          }

      }

    std::sort (v_scanned_kits.begin(), v_scanned_kits.end(),  [](CHydrogenKit* a, CHydrogenKit* b){return a->kit_name < b->kit_name;});

    for (auto i : v_scanned_kits)
       {
        v_kits_names.push_back (i->kit_name);

       }


//    std::sort ( v_kits_names.begin(),  v_kits_names.end(),  [](CHydrogenKit* a, CHydrogenKit* b){return a->kit_name < b->kit_name;});

   //std::sort (v_kits_names.begin(), v_kits_names.end());
   //v_kits_names.erase (std::unique( v_kits_names.begin(), v_kits_names.end() ), v_kits_names.end() );

}


void CHydrogenKitsScanner::print()
{
  for (size_t i = 0; i < v_scanned_kits.size(); i++)
     {
      std::cout << i << ": ";
      v_scanned_kits[i]->print_stats();
     }
}
