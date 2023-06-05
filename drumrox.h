/* drmr.h
 * LV2 DrMr plugin
 * Copyright 2012 Nick Lanham <nick@afternight.org>
 *
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/drmr>

 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DRUMROX_H
#define DRUMROX_H

#include <sndfile.h>
#include <pthread.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
// util includes atom.h
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>

#include "kits.h"


#define DRUMROX_URI "https://github.com/psemiletov/drumrox"
#define GAIN_MIN -60.0f
#define GAIN_MAX 6.0f


typedef enum {
  DRUMROX_CONTROL = 0,
  DRUMROX_LEFT,
  DRUMROX_RIGHT,
  DRUMROX_BASENOTE,
  DRUMROX_GAIN_01,
  DRUMROX_GAIN_02,
  DRUMROX_GAIN_03,
  DRUMROX_GAIN_04,
  DRUMROX_GAIN_05,
  DRUMROX_GAIN_06,
  DRUMROX_GAIN_07,
  DRUMROX_GAIN_08,
  DRUMROX_GAIN_09,
  DRUMROX_GAIN_10,
  DRUMROX_GAIN_11,
  DRUMROX_GAIN_12,
  DRUMROX_GAIN_13,
  DRUMROX_GAIN_14,
  DRUMROX_GAIN_15,
  DRUMROX_GAIN_16,
  DRUMROX_GAIN_17,
  DRUMROX_GAIN_18,
  DRUMROX_GAIN_19,
  DRUMROX_GAIN_20,
  DRUMROX_GAIN_21,
  DRUMROX_GAIN_22,
  DRUMROX_GAIN_23,
  DRUMROX_GAIN_24,
  DRUMROX_GAIN_25,
  DRUMROX_GAIN_26,
  DRUMROX_GAIN_27,
  DRUMROX_GAIN_28,
  DRUMROX_GAIN_29,
  DRUMROX_GAIN_30,
  DRUMROX_GAIN_31,
  DRUMROX_GAIN_32,
  DRUMROX_PAN_01,
  DRUMROX_PAN_02,
  DRUMROX_PAN_03,
  DRUMROX_PAN_04,
  DRUMROX_PAN_05,
  DRUMROX_PAN_06,
  DRUMROX_PAN_07,
  DRUMROX_PAN_08,
  DRUMROX_PAN_09,
  DRUMROX_PAN_10,
  DRUMROX_PAN_11,
  DRUMROX_PAN_12,
  DRUMROX_PAN_13,
  DRUMROX_PAN_14,
  DRUMROX_PAN_15,
  DRUMROX_PAN_16,
  DRUMROX_PAN_17,
  DRUMROX_PAN_18,
  DRUMROX_PAN_19,
  DRUMROX_PAN_20,
  DRUMROX_PAN_21,
  DRUMROX_PAN_22,
  DRUMROX_PAN_23,
  DRUMROX_PAN_24,
  DRUMROX_PAN_25,
  DRUMROX_PAN_26,
  DRUMROX_PAN_27,
  DRUMROX_PAN_28,
  DRUMROX_PAN_29,
  DRUMROX_PAN_30,
  DRUMROX_PAN_31,
  DRUMROX_PAN_32,
  DRUMROX_CORE_EVENT,
  DRUMROX_NUM_PORTS
} DrumroxPortIndex;



typedef struct
{
  LV2_URID midi_event;
  LV2_URID ui_msg;
  LV2_URID kit_path;
  LV2_URID atom_eventTransfer;
//  LV2_URID atom_resource;
  LV2_URID atom_object;

  LV2_URID string_urid;
  LV2_URID bool_urid;
  LV2_URID int_urid;
  LV2_URID get_state;
  LV2_URID midi_info;
  LV2_URID sample_trigger;
  LV2_URID velocity_toggle;
  LV2_URID note_off_toggle;
  LV2_URID panlaw;

} SDrumroxUris;


class CDrumrox
{
public:

  // Ports, pointers to LV's channels
  float* left;
  float* right;

  LV2_Atom_Sequence *control_port;
  LV2_Atom_Sequence *core_event_port;

  LV2_Atom_Forge forge;

  // params
  bool ignore_velocity;
  bool ignore_note_off;
  int panlaw;

  //32 gains and pans
  //float** gains;
  //float** pans;

  float* gains[32];
  float* pans[32];

//we use kit's v_samples[n] gain and pan instead

  float* baseNote;
  double rate;

  // URIs
  LV2_URID_Map* map;
  SDrumroxUris uris;

  // Kit info
  char* current_path; //absolute path to drumkit.xml
  char** request_buf;
  int curReq;

  // Samples
  CHydrogenKit *kit;

  // loading thread stuff
  pthread_mutex_t load_mutex;
  pthread_cond_t  load_cond;
  pthread_t load_thread;


  CDrumrox();
~CDrumrox();

};


static inline void map_drmr_uris (LV2_URID_Map *map, SDrumroxUris *uris)
{
  uris->midi_event = map->map (map->handle, "http://lv2plug.in/ns/ext/midi#MidiEvent");
  uris->string_urid = map->map(map->handle, LV2_ATOM__String);
  uris->bool_urid = map->map(map->handle, LV2_ATOM__Bool);
  uris->int_urid = map->map(map->handle, LV2_ATOM__Int);
  uris->ui_msg = map->map(map->handle, DRUMROX_URI "#uimsg");
  uris->kit_path = map->map(map->handle, DRUMROX_URI "#kitpath");
  uris->get_state = map->map(map->handle, DRUMROX_URI "#getstate");
  uris->midi_info = map->map(map->handle, DRUMROX_URI "#midiinfo");
  uris->sample_trigger = map->map(map->handle, DRUMROX_URI "#sampletrigger");
  uris->velocity_toggle = map->map(map->handle, DRUMROX_URI "#velocitytoggle");
  uris->note_off_toggle = map->map(map->handle, DRUMROX_URI "#noteofftoggle");
  uris->panlaw = map->map(map->handle, DRUMROX_URI "#panlaw");
  uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
  //uris->atom_resource = map->map(map->handle, LV2_ATOM__Resource);

  uris->atom_object = map->map(map->handle, LV2_ATOM__Object);

}


#endif // DRUMROX_H
