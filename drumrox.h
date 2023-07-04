/* drumrox.h
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


#ifndef DRUMROX_MULTI

#define DRUMROX_URI "https://github.com/psemiletov/drumrox"

#else

#define DRUMROX_URI "https://github.com/psemiletov/drumrox-multi"

#endif


#define GAIN_MIN -60.0f
#define GAIN_MAX 6.0f

#define REQ_BUF_SIZE 10

#ifndef DRUMROX_MULTI

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

#else

typedef enum {
  DRUMROX_CONTROL = 0,
  DRUMROX_CH01,
  DRUMROX_CH02,
  DRUMROX_CH03,
  DRUMROX_CH04,
  DRUMROX_CH05,
  DRUMROX_CH06,
  DRUMROX_CH07,
  DRUMROX_CH08,
  DRUMROX_CH09,
  DRUMROX_CH10,
  DRUMROX_CH11,
  DRUMROX_CH12,
  DRUMROX_CH13,
  DRUMROX_CH14,
  DRUMROX_CH15,
  DRUMROX_CH16,
  DRUMROX_CH17,
  DRUMROX_CH18,
  DRUMROX_CH19,
  DRUMROX_CH20,
  DRUMROX_CH21,
  DRUMROX_CH22,
  DRUMROX_CH23,
  DRUMROX_CH24,
  DRUMROX_CH25,
  DRUMROX_CH26,
  DRUMROX_CH27,
  DRUMROX_CH28,
  DRUMROX_CH29,
  DRUMROX_CH30,
  DRUMROX_CH31,
  DRUMROX_CH32,
  DRUMROX_BASENOTE,
  DRUMROX_CORE_EVENT,
  DRUMROX_NUM_PORTS
} DrumroxPortIndex;


#endif

typedef struct
{
  LV2_URID midi_event;
  LV2_URID ui_msg;
  LV2_URID kit_path;
  LV2_URID atom_eventTransfer;
  LV2_URID atom_object;
  LV2_URID string_urid;
  LV2_URID bool_urid;
  LV2_URID int_urid;
  LV2_URID get_state;
  LV2_URID midi_info;
  LV2_URID sample_trigger;
  LV2_URID velocity_toggle;
  LV2_URID note_off_toggle;

#ifndef DRUMROX_MULTI

  LV2_URID panlaw;

#endif

} SDrumroxUris;


class CDrumrox
{
public:

  // Ports, pointers to LV's channels
#ifndef DRUMROX_MULTI

  float* left;
  float* right;

#else

  float* channels[32];

#endif

  LV2_Atom_Sequence *control_port;
  LV2_Atom_Sequence *core_event_port;

  LV2_Atom_Forge forge;

  // params
  bool ignore_velocity;
  bool ignore_note_off;

#ifndef DRUMROX_MULTI

  int panlaw;

  //32 gains and pans

  float* gains[32];
  float* pans[32];

#endif


  float* baseNote;
  double rate;

  // URIs
  LV2_URID_Map* map;
  SDrumroxUris uris;

  // Kit info
  char* current_path; //absolute path to drumkit.xml

  char* request_buf[REQ_BUF_SIZE];
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


static inline void map_drumrox_uris (LV2_URID_Map *map, SDrumroxUris *uris)
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

#ifndef DRUMROX_MULTI
  uris->panlaw = map->map(map->handle, DRUMROX_URI "#panlaw");
#endif

  uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
  uris->atom_object = map->map(map->handle, LV2_ATOM__Object);
}


#endif // DRUMROX_H
