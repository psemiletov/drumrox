/* drumrox.cpp
 * LV2 Drumrox plugin
 * 2023 Peter Semiletov
 * based on DrMr
 * Copyright 2012 Nick Lanham <nick@afternight.org>
 * and Filipe Coelho's DrMr fork (https://github.com/falkTX/drmr).
 *
 * GPL Public License v3

 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#include "dsp.h"
#include "drumrox.h"


//#define REQ_BUF_SIZE 10
#define VELOCITY_MAX 127


//#define DB3SCALE -0.8317830986718104f
//#define DB3SCALEPO 1.8317830986718104f
// taken from lv2 example amp plugin
//#define DB_CO(g) ((g) > GAIN_MIN ? powf(10.0f, (g) * 0.05f) : 0.0f)



static int current_kit_changed = 0;


CDrumrox::CDrumrox()
{
  kit = 0;
  map = NULL;
  current_path = NULL;
  curReq = -1;
  ignore_velocity = false;
  ignore_note_off = true;

#ifndef DRUMROX_MULTI

  panlaw = 0;

#endif

#ifdef DRUMROX_MULTI

  for (size_t i = 0; i < 32; i++)
      {
       channels[i] = NULL;
      }

#endif

}


CDrumrox::~CDrumrox()
{
  delete kit;
}


static void* load_thread (void* arg)
{
  std::cout << "static void* load_thread \n";

  CDrumrox* drumrox = (CDrumrox*)arg;

  CHydrogenKit *new_kit;
  CHydrogenKit *old_kit;

  char *request; //path to drumkit xml???
  char *request_orig;

  for(;;)
     {
      pthread_mutex_lock (&drumrox->load_mutex);
      pthread_cond_wait (&drumrox->load_cond, &drumrox->load_mutex);
      pthread_mutex_unlock (&drumrox->load_mutex);

      old_kit = drumrox->kit;

      //ссылаемся на drumrox->request_buf[drumrox->curReq]
      //как понримаю, drumrox->curReq это текущий индекс кита
      //откуда он берется?

      request = drumrox->request_buf[drumrox->curReq];

      //типа сохраняем исходный указатель
      request_orig = request;

      //если в имени кита каким-то боком file://, смещаем начало указателя на 7 символов
      //if (! strncmp (request, "file://", 7))
        //  request += 7;

      std::cout << "request: " << request << std::endl;

      new_kit = new CHydrogenKit();

      //загружаем кит, в request у нас полное имя xml или txt файла
      new_kit->load (request, drumrox->rate);


      if (new_kit->v_samples.size() == 0) //если кит пустой
         {
          fprintf (stderr, "Failed to load kit at: %s\n", request);
          pthread_mutex_lock (&drumrox->load_mutex);

          //нужно ли? ведь мы загрузили новый кит во временный new_kit
          drumrox->kit = NULL;

          delete new_kit;

          pthread_mutex_unlock (&drumrox->load_mutex);
         }
     else
         {
          // just lock for the critical moment when we swap in the new kit
          //!!how it is good when DAW is playing?

          printf ("loaded kit at: %s\n", request);
          pthread_mutex_lock (&drumrox->load_mutex);
          drumrox->kit = new_kit;

          if (old_kit)
             delete old_kit;

          pthread_mutex_unlock (&drumrox->load_mutex);
         }

     //новый путь = request_orig
     drumrox->current_path = request_orig;

     current_kit_changed = 1;
    }

  return 0;
}


static LV2_Handle instantiate (const LV2_Descriptor* descriptor,
                               double rate,
                               const char* bundle_path,
                               const LV2_Feature* const* features)
{
  init_db();

//  std::cout << "INSTANCE!!!!!!!!!!!!!!!!!!! - 1" << std::endl;

  CDrumrox *drumrox = new CDrumrox;

  drumrox->rate = rate;

  if (pthread_mutex_init (&drumrox->load_mutex, 0))
     {
      fprintf (stderr, "Could not initialize load_mutex.\n");
      delete drumrox;
      return 0;
     }


  if (pthread_cond_init (&drumrox->load_cond, 0))
     {
      fprintf (stderr, "Could not initialize load_cond.\n");
      delete drumrox;
      return 0;
     }


  while (*features)
        {
         if (! strcmp((*features)->URI, LV2_URID_URI "#map"))
             drumrox->map = (LV2_URID_Map *)((*features)->data);

         features++;
        }


  if (! drumrox->map)
     {
      fprintf (stderr, "LV2 host does not support urid#map.\n");
      delete drumrox;
      return 0;
     }

  map_drumrox_uris (drumrox->map, &(drumrox->uris));
  
  lv2_atom_forge_init (&drumrox->forge, drumrox->map);

  if (pthread_create (&drumrox->load_thread, 0, load_thread, drumrox))
     {
      fprintf (stderr, "Could not initialize loading thread.\n");
      delete drumrox;
      return 0;
     }


  for (size_t i = 0; i < 16; i++)
      drumrox->request_buf[i] = 0;

#ifndef DRUMROX_MULTI

  for (int i = 0; i < 32; i++)
      {
       drumrox->gains[i] = NULL;
       drumrox->pans[i] = NULL;
      }

#endif
//  std::cout << "INSTANCE!!!!!!!!!!!!!!!!!!! - 2" << std::endl;

  return (LV2_Handle)drumrox;
}


#ifndef DRUMROX_MULTI

static void connect_port (LV2_Handle instance, uint32_t port, void* data)
{
//  std::cout << "void connect_port (LV2_Handle instance, uint32_t port, void* data)  -1" << std::endl;

  CDrumrox* drumrox = (CDrumrox*)instance;
  DrumroxPortIndex port_index = (DrumroxPortIndex)port;

  switch (port_index)
         {
          case DRUMROX_CONTROL:
                               drumrox->control_port = (LV2_Atom_Sequence*)data;
                               break;

          case DRUMROX_CORE_EVENT:
                                  drumrox->core_event_port = (LV2_Atom_Sequence*)data;
                                  break;

          case DRUMROX_LEFT:
                            drumrox->left = (float*)data;
                            break;

          case DRUMROX_RIGHT:
                             drumrox->right = (float*)data;
                             break;

          case DRUMROX_BASENOTE:
                                if (data)
                                   drumrox->baseNote = (float*)data;
          default:
                  break;
         }



  //link LV controls gains
  if (port_index >= DRUMROX_GAIN_01 && port_index <= DRUMROX_GAIN_32)
     {
      size_t gain_offset = port_index - DRUMROX_GAIN_01;
      drumrox->gains[gain_offset] = (float*)data;
     }


  //link LV controls pans
  if (port_index >= DRUMROX_PAN_01 && port_index <= DRUMROX_PAN_32)
     {
      size_t pan_offset = port_index - DRUMROX_PAN_01;
      drumrox->pans[pan_offset] = (float*)data;
     }

   //std::cout << "void connect_port (LV2_Handle instance, uint32_t port, void* data)  -2" << std::endl;
}

#else


static void connect_port (LV2_Handle instance, uint32_t port, void* data)
{
//  std::cout << "void connect_port (LV2_Handle instance, uint32_t port, void* data)  -1" << std::endl;

  CDrumrox* drumrox = (CDrumrox*)instance;
  DrumroxPortIndex port_index = (DrumroxPortIndex)port;

  switch (port_index)
         {
          case DRUMROX_CONTROL:
                               drumrox->control_port = (LV2_Atom_Sequence*)data;
                               break;

          case DRUMROX_CORE_EVENT:
                                  drumrox->core_event_port = (LV2_Atom_Sequence*)data;
                                  break;

          case DRUMROX_CH01:
                            drumrox->channels[0] = (float*)data;
                            break;

          case DRUMROX_CH02:
                            drumrox->channels[1] = (float*)data;
                            break;

          case DRUMROX_CH03:
                            drumrox->channels[2] = (float*)data;
                            break;

          case DRUMROX_CH04:
                            drumrox->channels[3] = (float*)data;
                            break;

          case DRUMROX_CH05:
                            drumrox->channels[4] = (float*)data;
                            break;


          case DRUMROX_CH06:
                            drumrox->channels[5] = (float*)data;
                            break;

          case DRUMROX_CH07:
                            drumrox->channels[6] = (float*)data;
                            break;

          case DRUMROX_CH08:
                            drumrox->channels[7] = (float*)data;
                            break;

          case DRUMROX_CH09:
                            drumrox->channels[8] = (float*)data;
                            break;

          case DRUMROX_CH10:
                            drumrox->channels[9] = (float*)data;
                            break;

          case DRUMROX_CH11:
                            drumrox->channels[10] = (float*)data;
                            break;
          case DRUMROX_CH12:
                            drumrox->channels[11] = (float*)data;
                            break;
          case DRUMROX_CH13:
                            drumrox->channels[12] = (float*)data;
                            break;
          case DRUMROX_CH14:
                            drumrox->channels[13] = (float*)data;
                            break;
          case DRUMROX_CH15:
                            drumrox->channels[14] = (float*)data;
                            break;
          case DRUMROX_CH16:
                            drumrox->channels[15] = (float*)data;
                            break;
          case DRUMROX_CH17:
                            drumrox->channels[16] = (float*)data;
                            break;


          case DRUMROX_CH18:
                            drumrox->channels[17] = (float*)data;
                            break;

          case DRUMROX_CH19:
                            drumrox->channels[18] = (float*)data;
                            break;
          case DRUMROX_CH20:
                            drumrox->channels[19] = (float*)data;
                            break;
          case DRUMROX_CH21:
                            drumrox->channels[20] = (float*)data;
                            break;
          case DRUMROX_CH22:
                            drumrox->channels[21] = (float*)data;
                            break;
          case DRUMROX_CH23:
                            drumrox->channels[22] = (float*)data;
                            break;
          case DRUMROX_CH24:
                            drumrox->channels[23] = (float*)data;
                            break;
          case DRUMROX_CH25:
                            drumrox->channels[24] = (float*)data;
                            break;

          case DRUMROX_CH26:
                            drumrox->channels[25] = (float*)data;
                            break;
          case DRUMROX_CH27:
                            drumrox->channels[26] = (float*)data;
                            break;
          case DRUMROX_CH28:
                            drumrox->channels[27] = (float*)data;
                            break;
          case DRUMROX_CH29:
                            drumrox->channels[28] = (float*)data;
                            break;

          case DRUMROX_CH30:
                            drumrox->channels[29] = (float*)data;
                            break;
          case DRUMROX_CH31:
                            drumrox->channels[30] = (float*)data;
                            break;
          case DRUMROX_CH32:
                            drumrox->channels[31] = (float*)data;
                            break;


          case DRUMROX_BASENOTE:
                                if (data)
                                   drumrox->baseNote = (float*)data;
          default:
                  break;
         }


   //std::cout << "void connect_port (LV2_Handle instance, uint32_t port, void* data)  -2" << std::endl;
}

#endif


/*
  build_update_message

  вызывается при смене кита

*/
static inline LV2_Atom *build_update_message (CDrumrox *drumrox)
{
  std::cout << "drumrox.cpp::LV2_Atom *build_update_message (CDrumrox *drumrox): " << drumrox->current_path << std::endl;;

  LV2_Atom_Forge_Frame set_frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object (&drumrox->forge, &set_frame, 1, drumrox->uris.ui_msg);

  if (drumrox->current_path)
     {
      lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.kit_path, 0);
      lv2_atom_forge_string (&drumrox->forge, drumrox->current_path, strlen (drumrox->current_path));
     }

  lv2_atom_forge_pop (&drumrox->forge, &set_frame);

  //       std::cout << "LV2_Atom *build_update_message (CDrumrox *drumrox) - 2 \n";

  return msg;
}


static inline LV2_Atom *build_state_message (CDrumrox *drumrox)
{
//    std::cout << "LV2_Atom *build_state_message (CDrumrox *drumrox) - 1 \n";

  LV2_Atom_Forge_Frame set_frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&drumrox->forge, &set_frame, 1, drumrox->uris.get_state);

  if (drumrox->current_path)
     {
      lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.kit_path, 0);
      lv2_atom_forge_string (&drumrox->forge, drumrox->current_path, strlen (drumrox->current_path));
     }

  lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.velocity_toggle, 0);

  lv2_atom_forge_bool(&drumrox->forge, drumrox->ignore_velocity?true:false);
  //lv2_atom_forge_bool (&drumrox->forge, drumrox->ignore_velocity);

  lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.note_off_toggle,0);
  lv2_atom_forge_bool(&drumrox->forge, drumrox->ignore_note_off?true:false);
//  lv2_atom_forge_bool (&drumrox->forge, drumrox->ignore_note_off);
#ifndef DRUMROX_MULTI
  lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.panlaw, 0);
  lv2_atom_forge_int (&drumrox->forge, drumrox->panlaw);
#endif

  lv2_atom_forge_pop (&drumrox->forge, &set_frame);

  //    std::cout << "LV2_Atom *build_state_message (CDrumrox *drumrox) - 2 \n";

  return msg;
}


static inline LV2_Atom *build_midi_info_message (CDrumrox *drumrox, uint8_t *data)
{
//       std::cout << " LV2_Atom *build_midi_info_message (CDrumrox *drumrox, uint8_t *data) \n";

  LV2_Atom_Forge_Frame set_frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object (&drumrox->forge, &set_frame, 1, drumrox->uris.midi_info);
  lv2_atom_forge_property_head (&drumrox->forge, drumrox->uris.midi_event, 0);
  lv2_atom_forge_write (&drumrox->forge, data, 3); //what is 3?
  lv2_atom_forge_pop (&drumrox->forge, &set_frame);

  return msg;
}


static inline void untrigger_sample (CDrumrox *drumrox,
                                     int note_number,
                                     uint32_t offset)
{
  pthread_mutex_lock (&drumrox->load_mutex);

  if (note_number >= 0 && note_number < drumrox->kit->v_samples.size())
     {
      CDrumSample *s = drumrox->kit->v_samples[note_number];
      s->current_layer = s->map_velo_to_layer_number (s->velocity);
      s->active = 0;
      s->v_layers[s->current_layer]->dataoffset = offset;
     }

  pthread_mutex_unlock (&drumrox->load_mutex);
}


static inline void trigger_sample (CDrumrox *drumrox,
                                   int note_number, //translated to kit's sample index
                                   uint8_t* const data,
                                   uint32_t offset)
{
  // need to mutex this to avoid getting the samples array
  // changed after the check that the midi-note is valid
  pthread_mutex_lock (&drumrox->load_mutex);

  if (note_number >= 0 && note_number < drumrox->kit->v_samples.size())
     {
      CDrumSample *s = drumrox->kit->v_samples[note_number]; //point to the sample


      if (drumrox->ignore_velocity)
         s->velocity = 1.0f;
      else
         s->velocity = ((float)data[2]) / VELOCITY_MAX;


       s->current_layer = s->map_velo_to_layer_number (s->velocity);

  //    std::cout << "velo: " << s->velocity << " layer: " << s->current_layer << std::endl;


      if (data)
         {
          lv2_atom_forge_frame_time (&drumrox->forge, 0);
          build_midi_info_message (drumrox, data);
         }

      s->active = 1;
      s->v_layers[s->current_layer]->offset = 0;

      s->v_layers[s->current_layer]->dataoffset = offset;


  //also untrigger open hihat if closed hihat triggering
   // so find the open hihat
    if (s->hihat_close)
       {
        for (size_t i = 0; i < drumrox->kit->v_samples.size(); i++)
            {
             CDrumSample *s2 = drumrox->kit->v_samples[i]; //point to the sample
             if (s2->hihat_open)
               s2->active = 0;
            }
       }

     }

  pthread_mutex_unlock (&drumrox->load_mutex);
}


#ifndef DRUMROX_MULTI

static void run (LV2_Handle instance, uint32_t n_samples)
{
  //std::cout << "void run (LV2_Handle instance, uint32_t n_samples) - 1" << std::endl;

  CDrumrox* drumrox = (CDrumrox*)instance;

 // if (! drumrox)
   //  std::cout << "! drumrox\n";

  //baseNote = (int)floorf(*(drumrox->baseNote));
  int baseNote = (int)*(drumrox->baseNote);

  const uint32_t event_capacity = drumrox->core_event_port->atom.size;

  lv2_atom_forge_set_buffer (&drumrox->forge, (uint8_t*)drumrox->core_event_port, event_capacity);
  LV2_Atom_Forge_Frame seq_frame;
  lv2_atom_forge_sequence_head (&drumrox->forge, &seq_frame, 0);

   //std::cout << "===1\n";

  /*
   LV2_Atom_Event ev is:
   https://lv2plug.in/c/html/group__atom.html#structLV2__Atom__Event

   */

  LV2_ATOM_SEQUENCE_FOREACH (drumrox->control_port, ev)
  {
    if (ev->body.type == drumrox->uris.midi_event)
       {
        //std::cout << "ev->body.type == drumrox->uris.midi_event\n";

        uint8_t note_number;
        uint8_t* const data = (uint8_t* const)(ev + 1);

        //uint32_t offset = (ev->time.frames > 0 && ev->time.frames < n_samples) ? ev->time.frames : 0;
        uint32_t offset = 0;
        //ev->time.frames is in the range between 0 and n_samples
        if (ev->time.frames > 0 && ev->time.frames < n_samples)
           offset = ev->time.frames;


          /*
            LV2_Atom_Event.time
            union LV2_Atom_Event.time
            Time stamp.
            Which type is valid is determined by context.
            int64_t	frames	Time in audio frames.
            double	beats	Time in beats.

           */


        switch ((*data) >> 4)
               {
                case 8: //sjaehn: The MIDI_NOTE_OFF byte is 0x8X with X is the channel number (0x0 to 0xF). And 0x8X shifted by 4 bytes is 8.
                       if (! drumrox->ignore_note_off)
                          {
                           note_number = data[1];
                           note_number -= baseNote;
                           untrigger_sample (drumrox, note_number, offset);
                          }
                       break;

                case 9: //sjaehn: Same as above, but for MIDI_NOTE_ON which is 0x
                       {
                        note_number = data[1];
                        note_number -= baseNote;
                        trigger_sample (drumrox, note_number, data, offset);
                        break;
                       }

                default:
                     //printf("Unhandeled status: %i\n",(*data)>>4);
                       break;
               }
        }
         else
             if (ev->body.type == drumrox->uris.atom_object)
                {
                 const LV2_Atom_Object *obj = (LV2_Atom_Object*)&ev->body;

                 if (obj->body.otype == drumrox->uris.ui_msg)
                    {
                     const LV2_Atom* path = NULL;
                     const LV2_Atom* trigger = NULL;
                     const LV2_Atom* ignvel = NULL;
                     const LV2_Atom* ignno = NULL;
                     const LV2_Atom* panlaw = NULL;

                     lv2_atom_object_get (obj,
                                          drumrox->uris.kit_path, &path,
                                          drumrox->uris.sample_trigger, &trigger,
                                          drumrox->uris.velocity_toggle, &ignvel,
                                          drumrox->uris.note_off_toggle, &ignno,
                                          drumrox->uris.panlaw, &panlaw,
                                         0);

                     if (path)
                        {
                         int reqPos = (drumrox->curReq + 1) % REQ_BUF_SIZE;

//                         std::cout << "reqPos:" << std::endl;

                         char *tmp = NULL;

                         if (reqPos >= 0 && drumrox->request_buf[reqPos])
                             tmp = drumrox->request_buf[reqPos]; //save drumrox->request_buf[reqPos];

                         drumrox->request_buf[reqPos] = strdup((char *)LV2_ATOM_BODY(path)); //set new value
                         drumrox->curReq = reqPos; //set new pos

                         if (tmp) //free saved at tmp drumrox->request_buf[reqPos];
                           free (tmp);
                       }

                     if (trigger)
                        {
                         int32_t sample_index = ((const LV2_Atom_Int*)trigger)->body;
                         uint8_t mdata[3];

                         //uint32_t offset = (ev->time.frames > 0 && ev->time.frames < n_samples) ? ev->time.frames : 0;
                         uint32_t offset = 0;

                         if (ev->time.frames > 0 && ev->time.frames < n_samples)
                             offset = ev->time.frames;

                         mdata[0] = 0x90; // note on
                         mdata[1] = sample_index + baseNote;
                         mdata[2] = 0x7f;
                         trigger_sample (drumrox, sample_index, mdata, offset);
                       }

                     if (ignvel)
                        drumrox->ignore_velocity = ((const LV2_Atom_Bool*)ignvel)->body;

                     if (ignno)
                        drumrox->ignore_note_off = ((const LV2_Atom_Bool*)ignno)->body;

                     if (panlaw)
                        drumrox->panlaw = ((const LV2_Atom_Int*)panlaw)->body;
                    }
                else
                    if (obj->body.otype == drumrox->uris.get_state)
                       {
              //   std::cout << "obj->body.otype == drumrox->uris.get_state\n";
                        lv2_atom_forge_frame_time (&drumrox->forge, 0);
                        build_state_message (drumrox);
                       }
               }
    //else printf("unrecognized event\n");
       }

   if ((drumrox->curReq >= 0) && drumrox->request_buf[drumrox->curReq] &&
      (! drumrox->current_path || strcmp (drumrox->current_path, drumrox->request_buf[drumrox->curReq])))
      pthread_cond_signal(&drumrox->load_cond);


   //если кит сменился, скидываем флаг и вызываем build_update_message
   if (current_kit_changed)
      {
       current_kit_changed = 0;
       lv2_atom_forge_frame_time (&drumrox->forge, 0);
       build_update_message (drumrox);
      }


   lv2_atom_forge_pop (&drumrox->forge, &seq_frame);

   /*
   for (size_t i = 0; i < n_samples; i++)
       {
        drumrox->left[i] = 0.0f;
        drumrox->right[i] = 0.0f;
       }
*/

   memset (drumrox->left, 0, sizeof(float)*n_samples);
   memset (drumrox->right, 0, sizeof(float)*n_samples);

   pthread_mutex_lock (&drumrox->load_mutex);

   //if (! drumrox->kit)
     //  std::cout << "! drumrox->kit\n";

   if (drumrox->kit)
   for (size_t i = 0; i < drumrox->kit->v_samples.size(); i++)
      {
       int pos;
       int lim;

       CDrumSample *current_sample = drumrox->kit->v_samples[i];

       if (current_sample->v_layers.size() == 0) //is sample empty?
          continue;

       CDrumLayer *drum_layer = current_sample->v_layers[current_sample->current_layer];

       if ((current_sample->active || drum_layer->dataoffset) && (drum_layer->samples_count > 0))
          {
           float coef_right;
           float coef_left;

           if (i < 32)
              {
                //float gain = DB_CO(*(drumrox->gains[i]));
               float gain = db2lin(*(drumrox->gains[i]));

               /*
               float pan_right = ((*drmr->pans[i]) + 1) / 2.0f;
               float pan_left = 1 - pan_right;
               */

               float pan_right = 0;
               float pan_left = 0;

               float pan = *drumrox->pans[i];

               if (drumrox->panlaw == PANLAW_LINEAR6)
                  pan_linear6 (pan_left, pan_right, pan);

               if (drumrox->panlaw == PANLAW_LINEAR0)
                  pan_linear0 (pan_left, pan_right, pan);

               if (drumrox->panlaw == PANLAW_SQRT)
                   pan_sqrt (pan_left, pan_right, pan);

               if (drumrox->panlaw == PANLAW_SINCOS)
                  pan_sincos (pan_left, pan_right, pan);


               coef_right = pan_right * gain * current_sample->velocity;
               coef_left = pan_left * gain * current_sample->velocity;

              }
           else
               {
                coef_right = 1.0f;
                coef_left = 1.0f;
               }

           int datastart;
           int dataend;

           if (current_sample->active)
              {
               datastart = drum_layer->dataoffset;
               dataend = n_samples;
              }
           else
               {
                datastart = 0;
                dataend = drum_layer->dataoffset;
               }

           drum_layer->dataoffset = 0;

           if (drum_layer->channels == 1)
              { // play mono sample

               if (n_samples < (drum_layer->samples_count - drum_layer->offset))
                  lim = n_samples;
               else
                   lim = drum_layer->samples_count - drum_layer->offset;

               for (pos = datastart; pos < lim && pos < dataend; pos++)
                   {
                    drumrox->left[pos] += drum_layer->data[drum_layer->offset] * coef_left;
                    drumrox->right[pos] += drum_layer->data[drum_layer->offset] * coef_right;
                    drum_layer->offset++;
                   }
               }
           else
               {
               // play stereo sample
               lim = (drum_layer->samples_count - drum_layer->offset) / drum_layer->/*info.*/channels;

               if (lim > n_samples)
                   lim = n_samples;

               for (pos = datastart; pos < lim && pos < dataend; pos++)
                   {
                    drumrox->left[pos] += drum_layer->data[drum_layer->offset++] * coef_left;
                    drumrox->right[pos] += drum_layer->data[drum_layer->offset++] * coef_right;
                   }
               }

           if (drum_layer->offset >= drum_layer->samples_count)
               current_sample->active = 0;
       }
      }

//   std::cout << "void run (LV2_Handle instance, uint32_t n_samples) - 2" << std::endl;

  pthread_mutex_unlock (&drumrox->load_mutex);
}

#else

static void run (LV2_Handle instance, uint32_t n_samples)
{
  //std::cout << "void run (LV2_Handle instance, uint32_t n_samples) - 1" << std::endl;

  CDrumrox* drumrox = (CDrumrox*)instance;

  if (! drumrox)
     std::cout << "! drumrox\n";

  //baseNote = (int)floorf(*(drumrox->baseNote));
  int baseNote = (int)*(drumrox->baseNote);

  const uint32_t event_capacity = drumrox->core_event_port->atom.size;

  lv2_atom_forge_set_buffer (&drumrox->forge, (uint8_t*)drumrox->core_event_port, event_capacity);
  LV2_Atom_Forge_Frame seq_frame;
  lv2_atom_forge_sequence_head (&drumrox->forge, &seq_frame, 0);

   //std::cout << "===1\n";

  /*
   LV2_Atom_Event ev is:
   https://lv2plug.in/c/html/group__atom.html#structLV2__Atom__Event

   */

  LV2_ATOM_SEQUENCE_FOREACH (drumrox->control_port, ev)
  {
    if (ev->body.type == drumrox->uris.midi_event)
       {
        //std::cout << "ev->body.type == drumrox->uris.midi_event\n";

        uint8_t note_number;
        uint8_t* const data = (uint8_t* const)(ev + 1);

        //uint32_t offset = (ev->time.frames > 0 && ev->time.frames < n_samples) ? ev->time.frames : 0;
        uint32_t offset = 0;
        //ev->time.frames is in the range between 0 and n_samples
        if (ev->time.frames > 0 && ev->time.frames < n_samples)
           offset = ev->time.frames;

        switch ((*data) >> 4)
               {
                case 8: //sjaehn: The MIDI_NOTE_OFF byte is 0x8X with X is the channel number (0x0 to 0xF). And 0x8X shifted by 4 bytes is 8.
                       if (! drumrox->ignore_note_off)
                          {
                           note_number = data[1];
                           note_number -= baseNote;
                           untrigger_sample (drumrox, note_number, offset);
                          }
                       break;

                case 9: //sjaehn: Same as above, but for MIDI_NOTE_ON which is 0x
                       {
                        note_number = data[1];
                        note_number -= baseNote;
                        trigger_sample (drumrox, note_number, data, offset);
                        break;
                       }

                default:
                     //printf("Unhandeled status: %i\n",(*data)>>4);
                       break;
               }
        }
         else
             if (ev->body.type == drumrox->uris.atom_object)
                {
                 const LV2_Atom_Object *obj = (LV2_Atom_Object*)&ev->body;

                 if (obj->body.otype == drumrox->uris.ui_msg)
                    {
                     const LV2_Atom* path = NULL;
                     const LV2_Atom* trigger = NULL;
                     const LV2_Atom* ignvel = NULL;
                     const LV2_Atom* ignno = NULL;

                     lv2_atom_object_get (obj,
                                          drumrox->uris.kit_path, &path,
                                          drumrox->uris.sample_trigger, &trigger,
                                          drumrox->uris.velocity_toggle, &ignvel,
                                          drumrox->uris.note_off_toggle, &ignno,
                                         0);

                     if (path)
                        {
                         int reqPos = (drumrox->curReq + 1) % REQ_BUF_SIZE;

//                         std::cout << "reqPos:" << std::endl;

                         char *tmp = NULL;

                         if (reqPos >= 0 && drumrox->request_buf[reqPos])
                             tmp = drumrox->request_buf[reqPos]; //save drumrox->request_buf[reqPos];

                         drumrox->request_buf[reqPos] = strdup((char *)LV2_ATOM_BODY(path)); //set new value
                         drumrox->curReq = reqPos; //set new pos

                         if (tmp) //free saved at tmp drumrox->request_buf[reqPos];
                           free (tmp);
                       }

                     if (trigger)
                        {
                         int32_t sample_index = ((const LV2_Atom_Int*)trigger)->body;
                         uint8_t mdata[3];

                         //uint32_t offset = (ev->time.frames > 0 && ev->time.frames < n_samples) ? ev->time.frames : 0;
                         uint32_t offset = 0;

                         if (ev->time.frames > 0 && ev->time.frames < n_samples)
                             offset = ev->time.frames;

                         mdata[0] = 0x90; // note on
                         mdata[1] = sample_index + baseNote;
                         mdata[2] = 0x7f;
                         trigger_sample (drumrox, sample_index, mdata, offset);
                       }

                     if (ignvel)
                        drumrox->ignore_velocity = ((const LV2_Atom_Bool*)ignvel)->body;

                     if (ignno)
                        drumrox->ignore_note_off = ((const LV2_Atom_Bool*)ignno)->body;

                    }
                else
                    if (obj->body.otype == drumrox->uris.get_state)
                       {
              //   std::cout << "obj->body.otype == drumrox->uris.get_state\n";
                        lv2_atom_forge_frame_time (&drumrox->forge, 0);
                        build_state_message (drumrox);
                       }
               }
    //else printf("unrecognized event\n");
       }

   if ((drumrox->curReq >= 0) && drumrox->request_buf[drumrox->curReq] &&
      (! drumrox->current_path || strcmp (drumrox->current_path, drumrox->request_buf[drumrox->curReq])))
      pthread_cond_signal(&drumrox->load_cond);


   //если кит сменился, скидываем флаг и вызываем build_update_message
   if (current_kit_changed)
      {
       current_kit_changed = 0;
       lv2_atom_forge_frame_time (&drumrox->forge, 0);
       build_update_message (drumrox);
      }


   lv2_atom_forge_pop (&drumrox->forge, &seq_frame);


   if (drumrox->kit)
   for (size_t i = 0; i < drumrox->kit->v_samples.size(); i++)
       {
        memset (drumrox->channels[i], 0, sizeof(float)*n_samples);
       }


   pthread_mutex_lock (&drumrox->load_mutex);

   if (! drumrox->kit)
       std::cout << "! drumrox->kit\n";

   if (drumrox->kit)
   for (size_t i = 0; i < drumrox->kit->v_samples.size(); i++)
      {
       int pos;
       int lim;

       CDrumSample *current_sample = drumrox->kit->v_samples[i];

       if (current_sample->v_layers.size() == 0) //is sample empty?
          continue;

       CDrumLayer *drum_layer = current_sample->v_layers[current_sample->current_layer];

       if ((current_sample->active || drum_layer->dataoffset) && (drum_layer->samples_count > 0))
          {

           int datastart;
           int dataend;

           if (current_sample->active)
              {
               datastart = drum_layer->dataoffset;
               dataend = n_samples;
              }
           else
               {
                datastart = 0;
                dataend = drum_layer->dataoffset;
               }

           drum_layer->dataoffset = 0;

           if (drum_layer->channels == 1)
              { // play mono sample

               if (n_samples < (drum_layer->samples_count - drum_layer->offset))
                  lim = n_samples;
               else
                   lim = drum_layer->samples_count - drum_layer->offset;


               //current_sample->velocity;


               for (pos = datastart; pos < lim && pos < dataend; pos++)
                   {
                    drumrox->channels[i][pos] += drum_layer->data[drum_layer->offset] * current_sample->velocity;
                    //drumrox->left[pos] += drum_layer->data[drum_layer->offset] * coef_left;
                    //drumrox->right[pos] += drum_layer->data[drum_layer->offset] * coef_right;
                    drum_layer->offset++;
                   }
               }
           else
               {
               // play stereo sample. NOT GOOD! DRUMROX-MULTI MUST BE WITH MONO SAMPLES ONLY!
               lim = (drum_layer->samples_count - drum_layer->offset) / drum_layer->channels;

               if (lim > n_samples)
                   lim = n_samples;

               for (pos = datastart; pos < lim && pos < dataend; pos++)
                   {
                    //drumrox->left[pos] += drum_layer->data[drum_layer->offset++] * coef_left;
                    //drumrox->right[pos] += drum_layer->data[drum_layer->offset++] * coef_right;
                    drumrox->channels[i][pos] += drum_layer->data[drum_layer->offset++] * current_sample->velocity;
                    drumrox->channels[i][pos] += drum_layer->data[drum_layer->offset] * current_sample->velocity;

                   }
               }

           if (drum_layer->offset >= drum_layer->samples_count)
               current_sample->active = 0;
       }
      }

//   std::cout << "void run (LV2_Handle instance, uint32_t n_samples) - 2" << std::endl;

  pthread_mutex_unlock (&drumrox->load_mutex);
}


#endif


static void cleanup (LV2_Handle instance)
{
  std::cout << "void cleanup (LV2_Handle instance) //DRUMROX \n";

  CDrumrox* drumrox = (CDrumrox*)instance;
  pthread_cancel (drumrox->load_thread);
  pthread_join (drumrox->load_thread, 0);

  delete drumrox;
}


static LV2_State_Status save_state (LV2_Handle instance,
                                    LV2_State_Store_Function store,
                                    void* handle,
                                    uint32_t flags,
                                    const LV2_Feature* const *features)
{
  std::cout << "LV2_State_Status save_state" << std::endl;


  CDrumrox *drumrox = (CDrumrox*) instance;

  int32_t flag;
  LV2_State_Status stat = LV2_STATE_SUCCESS;


  if (drumrox->current_path != NULL)  //drmr->current_path is absolute path
     {
      const char* path = drumrox->kit->kit_filename.c_str();

      stat = store (handle,
                    drumrox->uris.kit_path,
                    path,
                    strlen (path) + 1,
                    drumrox->uris.string_urid,
                    LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

      if (stat)
         return stat;
     }

  if (drumrox->ignore_velocity)
     flag = 1;
  else
      flag = 0;

  stat = store (handle,
                drumrox->uris.velocity_toggle,
                &flag,
                sizeof(int32_t),
                drumrox->uris.bool_urid,
                LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  if (stat)
     return stat;


  if (drumrox->ignore_note_off)
     flag = 1;
  else
      flag = 0;


  stat = store (handle,
                drumrox->uris.note_off_toggle,
                &flag,
                sizeof(uint32_t),
                drumrox->uris.bool_urid,
                LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  if (stat)
     return stat;

#ifndef DRUMROX_MULTI
  stat = store (handle,
                drumrox->uris.panlaw,
                &drumrox->panlaw,
                sizeof(int),
                drumrox->uris.int_urid,
                LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
#endif

  return stat;
}


static LV2_State_Status restore_state (LV2_Handle instance,
                                       LV2_State_Retrieve_Function retrieve,
                                       void* handle,
                                       uint32_t flags,
                                       const LV2_Feature *const *features)
{
  std::cout << "LV2_State_Status restore_state " << std::endl;

  CDrumrox* drumrox = (CDrumrox*)instance;

  size_t      size;
  uint32_t    type;
  uint32_t    fgs;

  const char *kit_path = (char*) retrieve (handle, drumrox->uris.kit_path, &size, &type, &fgs);

  if (kit_path)
     { // safe as we're in "Instantiation" threading class
      int reqPos = (drumrox->curReq + 1) % REQ_BUF_SIZE;
      char *tmp = NULL;

      if (reqPos >= 0 && drumrox->request_buf[reqPos])
         tmp = drumrox->request_buf[reqPos];

      drumrox->request_buf[reqPos] = strdup (kit_path);
      drumrox->curReq = reqPos;

      if (tmp)
         free(tmp);
    }

  const uint32_t* ignore_velocity = (uint32_t*) retrieve (handle, drumrox->uris.velocity_toggle, &size, &type, &fgs);

//  if (ignore_velocity)
  //    drumrox->ignore_velocity = *ignore_velocity?true:false;

    if (ignore_velocity)
       drumrox->ignore_velocity = *ignore_velocity?true:false;


  const uint32_t* ignore_note_off = (uint32_t*) retrieve (handle, drumrox->uris.note_off_toggle, &size, &type, &fgs);

  if (ignore_note_off)
     drumrox->ignore_note_off = *ignore_note_off ? true : false;

#ifndef DRUMROX_MULTI
  const int* panlaw = (int*) retrieve (handle, drumrox->uris.panlaw, &size, &type, &fgs);

  if (panlaw)
      drumrox->panlaw = *panlaw;
#endif


  return LV2_STATE_SUCCESS;
}


static const void* extension_data (const char* uri)
{
  static const LV2_State_Interface state_iface = { save_state, restore_state };

  if (! strcmp (uri, LV2_STATE__interface))
      return &state_iface;

  return NULL;
}


static const LV2_Descriptor descriptor = {
  DRUMROX_URI,
  instantiate,
  connect_port,
  NULL, // activate
  run,
  NULL, // deactivate
  cleanup,
  extension_data
};


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  switch (index) {
  case 0:
    return &descriptor;
  default:
    return NULL;
  }
}

