/*
written at 2023 by Peter Semiletov
this code is the public domain
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <string>
#include <chrono>

#include <stdio.h>
#include <stdlib.h>
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

  int error = src_simple (&data, SRC_SINC_BEST_QUALITY/*SRC_SINC_MEDIUM_QUALITY*/, channels);
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

size_t CDrumSample::map_velo_to_layer_number (float velo)
{
  if (v_layers.size() == 1)
     return 0; //return zero pos layer if we have just one layer

  size_t result = 0;

  //search for layer within its min..max gain
  for (size_t i = 0; i < v_layers.size(); i++)
      {
       if (v_layers[i]->min <= velo &&
          (v_layers[i]->max > velo ||
          (v_layers[i]->max == 1 && velo == 1)))
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

  if (node_name == "image")
     kit->image_fname = kit->kit_dir + "/" + txt.as_string();


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


  if (node_name == "filename")
     {
      std::string fname = txt.as_string();
      std::string path = kit->kit_dir + "/" + fname;
      std::string sample_name = kit->v_samples.back()->name;

      for (auto signature: kit->v_hat_open_signatures)
          {
           if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
              {
               kit->v_samples.back()->hihat_open = true;
               break;
              }
          }

      for (auto signature: kit->v_hat_close_signatures)
          {
           if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
              {
               kit->v_samples.back()->hihat_close = true;
               break;
              }
          }


      if (! kit->scan_mode && kit->v_samples.size() != 0)
          if (kit->v_samples.back()->v_layers.size() != 0)
                kit->v_samples.back()->v_layers.back()->load (path.c_str());
     }


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
         string fname = line.substr (pos + 1, line.size() - pos);

         if (fname.empty())
            continue;

         size_t check_for_list = fname.find (",");

         if (check_for_list != string::npos)
            {
             vector <string> v_fnames = split_string_to_vector (fname, ",", false);

             add_sample();
             v_samples.back()->name = sample_name;

             for (auto f: v_fnames)
                 {
                  string filename = kit_dir + "/" + f;
                  v_samples.back()->add_layer();

                  if (file_exists (filename) && ! scan_mode)
                      v_samples.back()->v_layers.back()->load (filename.c_str());
                 }

             float part_size = (float) 1 / v_samples.back()->v_layers.size();
             CDrumLayer *l;
              //evaluate min and max velocities by the file position in the vector
             for (size_t i = 0; i < v_samples.back()->v_layers.size(); i++)
                 {
                  l = v_samples.back()->v_layers[i];

                  float segment_start = part_size * i;
                  float segment_end = part_size * (i + 1) - 0.001;

                  //std::cout << "segment_start: " << segment_start << std::endl;
                  //std::cout << "segment_end: " << segment_end << std::endl;

                  l->min = segment_start;
                  l->max = segment_end;
                 }

              l->max = 1.0f;

//              std::cout << "l->max: " << l->max << std::endl;

            }
         else
             {
              string filename = kit_dir + "/" + fname;

              add_sample();

              v_samples.back()->name = sample_name;

              v_samples.back()->add_layer(); //add default layer

              if (file_exists (filename) && ! scan_mode)
                  v_samples.back()->v_layers.back()->load (filename.c_str());
             }


         for (auto signature: v_hat_open_signatures)
             {
              if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
                 {
                  v_samples.back()->hihat_open = true;
                  break;
                 }
             }


         for (auto signature: v_hat_close_signatures)
             {
              if (findStringIC (sample_name, signature) || findStringIC (fname, signature))
                 {
                  v_samples.back()->hihat_close = true;
                  break;
                 }
             }


        }

     std::string kitimg = kit_dir + "/image.jpg";

     if (! file_exists (kitimg))
          kitimg = kit_dir + "/image.png";

     if (file_exists (kitimg))
        image_fname = kitimg;
}


std::string guess_sample_name (const std::string &raw)
{
  std::string result;

  std::string t = raw;

  //remove .ext

  t.pop_back();
  t.pop_back();
  t.pop_back();
  t.pop_back();

  for (size_t i = 0; i < t.size(); i++)
      if (isalpha(t[i]))
         result += t[i];

  return result;
}


// trim from right
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}


void CHydrogenKit::load_sfz (const std::string data)
{
//  cout << "void CHydrogenKit::load_sfz (const std::string data)\n";

  if (data.empty())
      return;

  //change crlf in data to lf

  std::string temp_data = string_replace_all (data, "\r\n", "\n");
  temp_data = string_replace_all (data, "\\", "/");


  bool multi_layered = false;

  size_t pos = temp_data.find ("<group>");
  if (pos != string::npos)
     multi_layered = true;


  size_t i = kit_dir.rfind ("/");
  kit_name = kit_dir.substr (i + 1);

  stringstream st (temp_data);
  string line;

  while (getline (st, line))
        {
         if (line.empty())
            continue;

         if (line.find("//") != string::npos)
            continue;

         string fname;

//          cout << "parse line: " << line << endl;

         if (line.find ("<group>") != string::npos)
             add_sample();


         if (line.find("<region>") != string::npos  && ! multi_layered)
             add_sample();

         //parse filename for a layer

          pos = line.find ("sample=");

         if (pos != string::npos)
            {
             string just_name = line.substr (pos + 7);
             just_name = rtrim (just_name); //remove trailing spaces if any

             fname = kit_dir + "/" + just_name;

               //cout << "fname:" << fname << endl;
             v_samples.back()->add_layer();

             if (file_exists (fname))
                {
                 if (! scan_mode)
                     v_samples.back()->v_layers.back()->load (fname.c_str());

                  v_samples.back()->name = guess_sample_name (just_name);
                 }
            }


         if (! scan_mode && multi_layered && v_samples.back()->v_layers.size() != 0)
            {
             float part_size = (float) 1 / v_samples.back()->v_layers.size();
             CDrumLayer *l;
              //evaluate min and max velocities by the file position in the vector
             for (size_t i = 0; i < v_samples.back()->v_layers.size(); i++)
                 {
                  l = v_samples.back()->v_layers[i];

                  float segment_start = part_size * i;
                  float segment_end = part_size * (i + 1) - 0.001;

                  //std::cout << "segment_start: " << segment_start << std::endl;
                  //std::cout << "segment_end: " << segment_end << std::endl;

                  l->min = segment_start;
                  l->max = segment_end;
                  }

             l->max = 1.0f;
//            std::cout << "l->max: " << l->max << std::endl;
            }

  //     cout << "5555\n";

         if (! scan_mode && v_samples.size() > 0)
            {
             for (auto signature: v_hat_open_signatures)
                 {
                  //cout << v_samples.back()->name << endl;

                  if (findStringIC (v_samples.back()->name, signature))
                     {
                      v_samples.back()->hihat_open = true;
                      break;
                     }
                  }


            for (auto signature: v_hat_close_signatures)
                {
                 if (findStringIC (v_samples.back()->name, signature))
                    {
                     v_samples.back()->hihat_close = true;
                     break;
                    }
                }
           }
        }
}


void CHydrogenKit::load (const char *fname, int sample_rate)
{
//  cout << "void CHydrogenKit::load: " << fname << endl;

  auto start = chrono::high_resolution_clock::now();


  samplerate = sample_rate;

  string filename = resolve_symlink (fname);

//  cout << "resolved filename :" << filename << endl;

  kit_filename = filename;
  kit_dir = get_file_path (kit_filename);

  std::string source = string_file_load (filename);
  if (source.empty())
     return;

  if (ends_with (kit_filename, ".txt"))
     {
      load_txt (source);
      return;
     }

  if (ends_with (kit_filename, ".sfz"))
     {
      load_sfz (source);
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



  auto stop = chrono::high_resolution_clock::now();

  auto duration_msecs = chrono::duration_cast<chrono::milliseconds>(stop - start);

  std::cout << "loaded at: " << duration_msecs.count() << " msecs" << std::endl;

  //seconds_counter_ev = duration_s.count();

}


CHydrogenKit::CHydrogenKit()
{
  scan_mode = false;
  layers_supported = false;

  v_hat_open_signatures.push_back ("hat_o");
  v_hat_open_signatures.push_back ("open");
  v_hat_open_signatures.push_back ("swish");

  v_hat_close_signatures.push_back ("close");
  v_hat_close_signatures.push_back ("choke");
  v_hat_close_signatures.push_back ("hat_c");

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
  v_kits_locations.push_back (get_home_dir() + "/drumrox-kits");
  v_kits_locations.push_back (get_home_dir() + "/sfz-kits");

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

       bool kit_exists = false;

       std::string fname = kd + "/drumkit.xml";

       if (file_exists (fname))
          kit_exists = true;
       else
           {
            fname = kd + "/drumkit.txt";
            if (file_exists (fname))
               kit_exists = true;

          //  cout << fname << endl;
           }


       if (kd.find ("/sfz-kits") != string::npos)
          {
           //search sfz file
           std::cout << "search sfz file at: " << kd << std::endl;


           std::vector <std::string> v = files_get_list (kd, ".sfz");
           if (v.size() != 0)
              fname = v[0];

           std::cout << "fname: " << fname << std::endl;


            if (file_exists (fname))
               kit_exists = true;
          }


       if (kit_exists)
          {
           CHydrogenKit *kit = new CHydrogenKit;
           kit->scan_mode = true;
           kit->load (fname.c_str(), 44100);
           v_scanned_kits.push_back (kit);
//           v_kits_names.push_back (kit->kit_name);
          // m_kits.insert (pair<string,string> (kit->kit_name, fname));
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
      //v_scanned_kits[i]->print_stats();
       std::cout << v_scanned_kits[i]->kit_name << std::endl;

     }
}
