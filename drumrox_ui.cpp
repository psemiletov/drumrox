/* drumrox_ui.cpp
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

#include <limits.h>
#include <stdlib.h>

#include <iostream>


#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "drumrox.h"
#include "nknob.h"
#include "dsp.h"

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

//#include <gdk/gdkx.h>


#define DRMR_UI_URI "https://github.com/psemiletov/drumrox#ui"
#define NO_KIT_STRING "[No Current Kit]"

class CDrumroxGTKGUI
{
public:

  LV2UI_Write_Function write;
  LV2UI_Controller controller;
  LV2_Atom_Forge forge;
  LV2_URID_Map *map;
  SDrumroxUris uris;

  GtkWidget *drumrox_widget;
  GtkLabel *current_kit_label;
  GtkTable *sample_table;
  GtkComboBox *kit_combo;
  GtkWidget *no_kit_label;
  GtkSpinButton *base_spin;
  GtkLabel *base_label;
  GtkListStore *kit_store;

  GtkWidget** gain_sliders;
  GtkWidget** pan_sliders;

  float *gain_vals;
  float *pan_vals;

  GtkWidget** notify_leds;
  GtkWidget *panlaw_combo_box, *velocity_checkbox, *note_off_checkbox;

  gchar *bundle_path;

  int cols;
  int panlaw;

  gboolean forceUpdate;

  int samples;
  int baseNote;

  GQuark gain_quark, pan_quark, trigger_quark;

  int current_kit_index; //current kit index
  int kitReq;
  //s_kits* kits;

//  CHydrogenKits kits;

  CHydrogenKitsScanner kits;

};


static GdkPixbuf *led_on_pixbuf = NULL, *led_off_pixbuf = NULL;


static gboolean gain_callback (GtkRange* range, GtkScrollType type, gdouble value, gpointer data)
{
  std::cout << "gain_callback\n";

  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  int gidx = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(range),ui->gain_quark));
  float gain = (float)value;
  ui->gain_vals[gidx] = gain;
  ui->write (ui->controller, gidx + DRUMROX_GAIN_01, 4, 0, &gain);
  return FALSE;
}


static gboolean pan_callback (GtkRange* range, GtkScrollType type, gdouble value, gpointer data)
{
  std::cout << "pan_callback\n";

  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  int pidx = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(range), ui->pan_quark));
  float pan = (float)value;
  ui->pan_vals[pidx] = pan;
  ui->write (ui->controller, pidx + DRUMROX_PAN_01, 4, 0, &pan);
  return FALSE;
}


static void send_ui_msg (CDrumroxGTKGUI* ui, void (*add_data)(CDrumroxGTKGUI* ui, gpointer data), gpointer data)
{
  std::cout << "send_ui_msg\n";

  LV2_Atom_Forge_Frame set_frame;
  uint8_t msg_buf[1024];

  lv2_atom_forge_set_buffer (&ui->forge, msg_buf, 1024);
  LV2_Atom *msg = (LV2_Atom*)lv2_atom_forge_resource (&ui->forge, &set_frame, 1, ui->uris.ui_msg);
  (*add_data)(ui, data);
  lv2_atom_forge_pop (&ui->forge, &set_frame);
  ui->write (ui->controller, DRUMROX_CONTROL, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
}


static void led_data (CDrumroxGTKGUI *ui, gpointer data)
{
  lv2_atom_forge_property_head (&ui->forge, ui->uris.sample_trigger, 0);
  lv2_atom_forge_int (&ui->forge, GPOINTER_TO_INT(data));
}


static void ignore_velocity_data (CDrumroxGTKGUI* ui, gpointer data)
{
  std::cout << "void ignore_velocity_data\n";

  lv2_atom_forge_property_head (&ui->forge, ui->uris.velocity_toggle, 0);
  lv2_atom_forge_bool(&ui->forge, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)));
}


static void ignore_note_off_data (CDrumroxGTKGUI* ui, gpointer data)
{
  std::cout << "void ignore_note_off_data\n";

  lv2_atom_forge_property_head (&ui->forge, ui->uris.note_off_toggle,0);
  lv2_atom_forge_bool(&ui->forge, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)));
}


static gboolean trigger_led_clicked (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  send_ui_msg (ui, &led_data, g_object_get_qdata(G_OBJECT(widget),ui->trigger_quark));
  return FALSE;
}


static gboolean ignore_velocity_toggled (GtkToggleButton *button, gpointer data)
{
  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  send_ui_msg (ui, &ignore_velocity_data, button);
  return FALSE;
}


static gboolean ignore_note_off_toggled (GtkToggleButton *button, gpointer data)
{
  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  send_ui_msg (ui, &ignore_note_off_data, button);
  return FALSE;
}


static void fill_sample_table (CDrumroxGTKGUI* ui, int samples_count, int kit_index, GtkWidget** notify_leds, GtkWidget** gain_sliders, GtkWidget** pan_sliders)

//static void fill_sample_table (CDrumroxGTKGUI* ui, int samples, /*char** names, */GtkWidget** notify_leds, GtkWidget** gain_sliders, GtkWidget** pan_sliders)
{
  std::cout << "fill_sample_table\n";

  if (samples_count == 0)
     return;

  int row = 0;
  int col = 0;

  gchar buf[64];

  int rows = (samples_count / ui->cols);

 // int rows = (samples / 6);


  if (samples_count % ui->cols != 0)
      rows++;

  gtk_table_resize (ui->sample_table, rows, ui->cols);

  for (int si = 0; si < samples_count; si++)
      {
       GtkWidget *frame, *vbox, *hbox, *gain_vbox, *pan_vbox;
       GtkWidget *button_box, *led_event_box, *led;
       GtkWidget* gain_slider;
       GtkWidget* pan_slider;
       GtkWidget* gain_label;
       GtkWidget* pan_label;
       gboolean slide_expand;

//       snprintf (buf, 64, "<b>%s</b> (%i)", names[si],si);

     //  snprintf (buf, 64, "<b>%s</b> (%i)", ui->kits.v_kits_names[si].c_str(),si);

       const char *sample_name = ui->kits.v_scanned_kits[kit_index]->v_samples[si]->name.c_str();

       snprintf (buf, 64, "<b>%s</b> (%i)", sample_name, si);



       frame = gtk_frame_new (buf);
       gtk_label_set_use_markup (GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))),true);
       gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_OUT);

       vbox = gtk_vbox_new (false,5);
       hbox = gtk_hbox_new (false,0);

#ifdef NO_NKNOB
//       gain_slider = gtk_vscale_new_with_range(GAIN_MIN,6.0,1);
       gain_slider = gtk_vscale_new_with_range (GAIN_MIN, 6.0, 0.1);

       gtk_widget_set_usize (gain_slider, -1, 64);
       gtk_scale_set_value_pos(GTK_SCALE(gain_slider),GTK_POS_BOTTOM);
//        gtk_scale_set_digits(GTK_SCALE(gain_slider),1);
       gtk_scale_set_digits(GTK_SCALE(gain_slider),2);

       gtk_scale_add_mark(GTK_SCALE(gain_slider),0.0,GTK_POS_RIGHT,"0 dB");
    // Hrmm, -inf label is at top in ardour for some reason
    //gtk_scale_add_mark(GTK_SCALE(gain_slider),GAIN_MIN,GTK_POS_RIGHT,"-inf");
       gtk_range_set_inverted (GTK_RANGE(gain_slider), true);
       slide_expand = true;
#else
//    gain_slider = n_knob_new_with_range(0.0,GAIN_MIN,6.0,1.0);
       gain_slider = n_knob_new_with_range (0.0, GAIN_MIN, 6.0, 0.1);

       n_knob_set_load_prefix(N_KNOB(gain_slider),ui->bundle_path);
       gtk_widget_set_has_tooltip(gain_slider,TRUE);
       slide_expand = false;
#endif

       g_object_set_qdata (G_OBJECT(gain_slider), ui->gain_quark, GINT_TO_POINTER(si));

       if (gain_sliders)
           gain_sliders[si] = gain_slider;

       if (si < 32)
          gtk_range_set_value(GTK_RANGE(gain_slider), ui->gain_vals[si]);
       else // things are gross if we have > 32 samples, what to do?
           gtk_range_set_value(GTK_RANGE(gain_slider), 0.0);

       g_signal_connect(G_OBJECT(gain_slider), "change-value", G_CALLBACK(gain_callback),ui);
       gain_label = gtk_label_new ("Gain");
       gain_vbox = gtk_vbox_new (false, 0);

#ifdef NO_NKNOB
       pan_slider = gtk_hscale_new_with_range (0, 1.0, 0.1);
       gtk_scale_add_mark(GTK_SCALE(pan_slider), 0.0, GTK_POS_TOP,NULL);
#else
       pan_slider = n_knob_new_with_range (0.5, 0, 1.0, 0.1);
       n_knob_set_load_prefix(N_KNOB(pan_slider),ui->bundle_path);
       gtk_widget_set_has_tooltip(pan_slider,TRUE);
#endif

      if (pan_sliders)
          pan_sliders[si] = pan_slider;

      if (si < 32)
         gtk_range_set_value (GTK_RANGE(pan_slider), (gdouble) ui->pan_vals[si]);
       //  gtk_range_set_value (GTK_RANGE(pan_slider), 0.5);
      else
          gtk_range_set_value (GTK_RANGE(pan_slider), 0.5f);

      g_object_set_qdata (G_OBJECT(pan_slider),ui->pan_quark,GINT_TO_POINTER(si));
      g_signal_connect (G_OBJECT(pan_slider),"change-value",G_CALLBACK(pan_callback),ui);

      pan_label = gtk_label_new ("Pan");
      pan_vbox = gtk_vbox_new (false, 0);
    
      gtk_box_pack_start(GTK_BOX(gain_vbox),gain_slider,slide_expand,slide_expand,0);
      gtk_box_pack_start(GTK_BOX(gain_vbox),gain_label,false,false,0);

      gtk_box_pack_start(GTK_BOX(pan_vbox),pan_slider,slide_expand,slide_expand,0);
      gtk_box_pack_start(GTK_BOX(pan_vbox),pan_label,false,false,0);

      gtk_box_pack_start(GTK_BOX(hbox),gain_vbox,true,true,0);
      gtk_box_pack_start(GTK_BOX(hbox),pan_vbox,true,true,0);

      gtk_box_pack_start(GTK_BOX(vbox),hbox,true,true,0);

      button_box = gtk_hbox_new (false, 2);

      led_event_box = gtk_event_box_new();
      g_object_set_qdata(G_OBJECT(led_event_box), ui->trigger_quark, GINT_TO_POINTER(si));
      g_signal_connect(G_OBJECT(led_event_box),"button-release-event", G_CALLBACK(trigger_led_clicked),ui);

      led = gtk_image_new_from_pixbuf(led_off_pixbuf);
      if (notify_leds)
         notify_leds[si] = led;

      gtk_container_add(GTK_CONTAINER(led_event_box),led);

      gtk_box_pack_start(GTK_BOX(button_box),led_event_box,false,false,0);
      gtk_box_pack_start(GTK_BOX(button_box),gtk_label_new(""),true,true,0);


      gtk_box_pack_start(GTK_BOX(hbox),button_box,false,false,0);
      g_object_set(vbox,"border-width", 5, NULL);
      gtk_container_add(GTK_CONTAINER(frame),vbox);

      gtk_table_attach_defaults (ui->sample_table, frame, col, col + 1, row, row + 1);

      col++;

      if (col >= ui->cols)
         {
          row++;
          col = 0;
         }
     }

  gtk_widget_queue_resize (GTK_WIDGET(ui->sample_table));
}


static gboolean unset_bg (gpointer data)
{
  if (GTK_IS_IMAGE (data))
      gtk_image_set_from_pixbuf (GTK_IMAGE(data), led_off_pixbuf);

  return FALSE;
}


static void sample_triggered (CDrumroxGTKGUI *ui, int si)
{
  if (ui->notify_leds && si < ui->samples)
     {
      //gtk_widget_modify_bg (ui->frames[si],
        //              GtkStateType state,
          //            const GdkColor *color);

      gtk_image_set_from_pixbuf(GTK_IMAGE(ui->notify_leds[si]),led_on_pixbuf);
      g_timeout_add(200,unset_bg,ui->notify_leds[si]);


     }
}


static const char* nstrs = "C C#D D#E F F#G G#A A#B ";
static char baseLabelBuf[32];

static void setBaseLabel (int noteIdx)
{
  int oct = (noteIdx / 12) - 1;
  int nmt = (noteIdx % 12) * 2;
  snprintf (baseLabelBuf, 32, "Midi Base Note <b>(%c%c%i)</b>:", nstrs[nmt], nstrs[nmt + 1], oct);
}


static void base_changed (GtkSpinButton *base_spin, gpointer data)
{
    std::cout << "static void base_changed (\n";


  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  float base = (float)gtk_spin_button_get_value(base_spin);

  if (base >= 21.0f && base <= 107.0f)
     {
      setBaseLabel((int)base);
      ui->write (ui->controller, DRUMROX_BASENOTE, 4, 0, &base);
      gtk_label_set_markup (ui->base_label, baseLabelBuf);
      ui->baseNote = (int)base;
     }
  else
      fprintf(stderr,"Base spin got out of range: %f\n",base);
}


static void fill_kit_combo (GtkComboBox* combo, std::vector <std::string> v_kits_names)
{
   std::cout << "void fill_kit_combo \n";

  if (v_kits_names.size() == 0)
     return;

  GtkTreeIter iter;
  GtkListStore *store = GTK_LIST_STORE (gtk_combo_box_get_model(combo));

  for (size_t i = 0; i < v_kits_names.size(); i++)
      {
       gtk_list_store_append (store, &iter);
       gtk_list_store_set(store, &iter, 0, v_kits_names[i].c_str(), -1);
      }
}


static gboolean idle = FALSE;

static gboolean kit_callback (gpointer data)
{
  std::cout << "gboolean kit_callback  \n";

  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;

  if (ui->forceUpdate || (ui->kitReq != ui->current_kit_index))
     {
      ui->forceUpdate = false;
      //int samples = (ui->kitReq<ui->kits->num_kits && ui->kitReq >= 0)? ui->kits->kits[ui->kitReq].samples: 0;

      int samples_count; //samples count in the kit (kitReq index)
      //= (ui->kitReq<ui->kits->num_kits && ui->kitReq >= 0) ? ui->kits->kits[ui->kitReq].samples: 0;

      samples_count = ui->kits.v_scanned_kits[ui->kitReq]->v_samples.size();


      GtkWidget** notify_leds;
      GtkWidget** gain_sliders;
      GtkWidget** pan_sliders;

      if (ui->sample_table)
         {
          notify_leds = ui->notify_leds;
          gain_sliders = ui->gain_sliders;
          pan_sliders = ui->pan_sliders;

          ui->samples = 0;
          ui->notify_leds = NULL;
          ui->gain_sliders = NULL;
          ui->pan_sliders = NULL;

          if (notify_leds)
             free (notify_leds);

          if (gain_sliders)
             free (gain_sliders);

          if (pan_sliders)
             free (pan_sliders);

          gtk_widget_destroy(GTK_WIDGET(ui->sample_table));
          ui->sample_table = NULL;
         }

      if (samples_count > 0)
         {
          ui->sample_table = GTK_TABLE (gtk_table_new (1, 1, true));
         //ui->sample_table = GTK_TABLE (gtk_table_new (6, 6, true));

          gtk_table_set_col_spacings (ui->sample_table, 5);
          gtk_table_set_row_spacings (ui->sample_table, 5);

          notify_leds = (GtkWidget**) malloc (samples_count * sizeof (GtkWidget*));
          gain_sliders = (GtkWidget**) malloc (samples_count * sizeof (GtkWidget*));
          pan_sliders = (GtkWidget**) malloc (samples_count * sizeof (GtkWidget*));

          fill_sample_table (ui, samples_count, ui->kitReq, notify_leds, gain_sliders, pan_sliders);

          gtk_box_pack_start(GTK_BOX(ui->drumrox_widget),GTK_WIDGET(ui->sample_table), true,true,5);
          gtk_box_reorder_child(GTK_BOX(ui->drumrox_widget),GTK_WIDGET(ui->sample_table), 1);
          gtk_widget_show_all(GTK_WIDGET(ui->sample_table));

          ui->samples = samples_count;
          ui->notify_leds = notify_leds;
          ui->gain_sliders = gain_sliders;
          ui->pan_sliders = pan_sliders;

//      gtk_label_set_text(ui->current_kit_label,ui->kits->kits[ui->kitReq].name);

          gtk_label_set_text (ui->current_kit_label, ui->kits.v_scanned_kits[ui->kitReq]->name.c_str());

          ui->current_kit_index = ui->kitReq;
          gtk_combo_box_set_active(ui->kit_combo, ui->current_kit_index); //SETS CURRENT KIT
          gtk_widget_show(GTK_WIDGET(ui->kit_combo));
          gtk_widget_hide(ui->no_kit_label);
         }
     else
         {
          gtk_widget_show (ui->no_kit_label);
          gtk_label_set_text (ui->current_kit_label,NO_KIT_STRING);
          gtk_widget_hide (GTK_WIDGET(ui->kit_combo));
         }
    }

  idle = FALSE;
  return FALSE; // don't keep calling
}

//lv2_atom_forge_object
static LV2_Atom* build_path_message (CDrumroxGTKGUI *ui, const char* path)
{
  std::cout << "LV2_Atom* build_path_message\n";

  std::cout << "path: " << path << std::endl;


  LV2_Atom_Forge_Frame set_frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_resource (&ui->forge, &set_frame, 1, ui->uris.ui_msg);
  lv2_atom_forge_property_head (&ui->forge, ui->uris.kit_path,0);
  lv2_atom_forge_path (&ui->forge, path, strlen(path));
  lv2_atom_forge_pop (&ui->forge, &set_frame);
  return msg;
}


static LV2_Atom* build_get_state_message (CDrumroxGTKGUI *ui)
{
   std::cout << "LV2_Atom* build_get_state_message\n";

  LV2_Atom_Forge_Frame set_frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_resource (&ui->forge, &set_frame, 1, ui->uris.get_state);
  lv2_atom_forge_pop (&ui->forge,&set_frame);
  return msg;
}


static void kit_combobox_changed (GtkComboBox* box, gpointer data)
{
   std::cout << "void kit_combobox_changed \n";


  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  gint new_kit_index = gtk_combo_box_get_active (GTK_COMBO_BOX(box));

  if (ui->current_kit_index != new_kit_index)
     {
      uint8_t msg_buf[1024];
      lv2_atom_forge_set_buffer(&ui->forge, msg_buf, 1024);

      LV2_Atom *msg = build_path_message (ui, ui->kits.v_scanned_kits[new_kit_index]->kit_xml_filename.c_str());
      ui->write (ui->controller, DRUMROX_CONTROL, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
     }
}


static void panlaw_data (CDrumroxGTKGUI *ui, gpointer data)
{
   std::cout << "void panlaw_data \n";

  lv2_atom_forge_property_head (&ui->forge, ui->uris.panlaw, 0);
  lv2_atom_forge_int(&ui->forge, GPOINTER_TO_INT(data));
}


static void panlaw_combobox_changed (GtkComboBox* box, gpointer data)
{
  std::cout << "panlaw_combobox_changed  \n";


  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;

  gint i = gtk_combo_box_get_active (GTK_COMBO_BOX(box));
  if (i != ui->panlaw)
     {
      ui->panlaw = i;
    //  ui->forceUpdate = true; //?
    //  kit_callback (ui);
    //????????????????
      send_ui_msg (ui, &panlaw_data, GINT_TO_POINTER(i));
     }
}

static GtkWidget *create_panlaw_combo (void)
{
  GtkWidget *combo;
  GtkListStore *list_store;
  GtkCellRenderer *cell;
  GtkTreeIter iter;

  list_store = gtk_list_store_new (1, G_TYPE_STRING);

  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, "linear panner, law: -6 dB", -1);
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, "linear panner, law: 0 dB", -1);
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, "square root panner, law: -3 dB", -1);
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set (list_store, &iter, 0, "sin/cos panner, law: -3 dB", -1);

  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL(list_store));

  gtk_combo_box_set_active (GTK_COMBO_BOX(combo),0);

  g_object_unref (list_store);

  cell = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell, "text", 0, NULL);

  return combo;
}


static gulong expose_id;

static gboolean expose_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    std::cout << "gboolean expose_callback  \n";


  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)data;
  uint8_t msg_buf[1024];

  lv2_atom_forge_set_buffer (&ui->forge, msg_buf, 1024);
  LV2_Atom *msg = build_get_state_message(ui);
  ui->write (ui->controller, DRUMROX_CONTROL, lv2_atom_total_size(msg), ui->uris.atom_eventTransfer, msg);
  g_signal_handler_disconnect (widget, expose_id);

  return FALSE;
}


static void load_led_pixbufs (CDrumroxGTKGUI* ui)
{
  GError *error = NULL;
  gchar *pixbuf_path;

  pixbuf_path = g_build_path (G_DIR_SEPARATOR_S, ui->bundle_path, "led_on.png", NULL);

  if (pixbuf_path)
     {
      led_on_pixbuf = gdk_pixbuf_new_from_file (pixbuf_path,&error);

      if (! led_on_pixbuf)
         fprintf (stderr, "Could not load led_on pixbuf: %s\n", error->message);

      g_free(pixbuf_path);
     }
  else
      fprintf (stderr, "Could not build path to load led_on pixbuf\n");


  pixbuf_path = g_build_path (G_DIR_SEPARATOR_S, ui->bundle_path, "led_off.png", NULL);

  if (pixbuf_path)
     {
      led_off_pixbuf = gdk_pixbuf_new_from_file (pixbuf_path, &error);

      if (! led_off_pixbuf)
         fprintf (stderr,"Could not load led_off pixbuf: %s\n", error->message);

      g_free(pixbuf_path);
     }
  else
      fprintf( stderr, "Could not build path to load led_off pixbuf\n");
}


#define PADVAL 5

static void build_drmr_ui (CDrumroxGTKGUI* ui)
{
  std::cout << "void build_drmr_ui \n";


  GtkWidget *drumrox_ui_widget;
  GtkWidget *opts_hbox1, *opts_hbox2,
    *kit_combo_box, *kit_label, *no_kit_label,
    *base_label, *base_spin, *panlaw_label;
  GtkCellRenderer *cell_rend;
  GtkAdjustment *base_adj;
  
  PangoAttrList	*attr_lst;
  PangoAttribute *attr;

  drumrox_ui_widget = gtk_vbox_new (false, 0);
  expose_id = g_signal_connect (drumrox_ui_widget, "expose-event", G_CALLBACK (expose_callback), ui);
  g_object_set (drumrox_ui_widget, "border-width", 2, NULL);

  ui->kit_store = gtk_list_store_new (1, G_TYPE_STRING);

  ui->current_kit_label = GTK_LABEL (gtk_label_new(NO_KIT_STRING));
  attr = pango_attr_weight_new (PANGO_WEIGHT_HEAVY);
  attr_lst = pango_attr_list_new();
  pango_attr_list_insert (attr_lst, attr);
  gtk_label_set_attributes (ui->current_kit_label, attr_lst);
  pango_attr_list_unref(attr_lst);

  opts_hbox1 = gtk_hbox_new (false,0);
  opts_hbox2 = gtk_hbox_new (false,0);
  kit_combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL(ui->kit_store));
  kit_label = gtk_label_new ("Kit:");

  no_kit_label = gtk_label_new ("<b>No/Invalid Kit Selected</b>");
  gtk_label_set_use_markup (GTK_LABEL (no_kit_label), true);

  cell_rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(kit_combo_box), cell_rend, true);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(kit_combo_box), cell_rend,"text",0,NULL);

  base_label = gtk_label_new ("Midi Base Note <b>(C 2)</b>:");
  gtk_label_set_use_markup(GTK_LABEL(base_label),true);
  base_adj = GTK_ADJUSTMENT (gtk_adjustment_new (36.0, // val
                                                 21.0,107.0, // min/max
                                                 1.0, // step
                                                 5.0,0.0)); // page adj/size

  base_spin = gtk_spin_button_new (base_adj, 1.0, 0);

  panlaw_label = gtk_label_new ("Panning mode");
  ui->panlaw_combo_box = create_panlaw_combo();

  ui->velocity_checkbox = gtk_check_button_new_with_label("Ignore Velocity");
  ui->note_off_checkbox = gtk_check_button_new_with_label("Ignore Note Off");

  gtk_box_pack_start (GTK_BOX(opts_hbox1), kit_label, false, false, PADVAL);
  gtk_box_pack_start (GTK_BOX(opts_hbox1),no_kit_label, true,true,0);
  gtk_box_pack_start (GTK_BOX(opts_hbox1),kit_combo_box, true,true,0);
  gtk_box_pack_start (GTK_BOX(opts_hbox1),base_label, false,false,PADVAL);
  gtk_box_pack_start (GTK_BOX(opts_hbox1),base_spin, true,true,0);

  gtk_box_pack_start(GTK_BOX(opts_hbox2),panlaw_label, false,false,PADVAL);
  gtk_box_pack_start(GTK_BOX(opts_hbox2),ui->panlaw_combo_box, false,false,0);
  gtk_box_pack_start(GTK_BOX(opts_hbox2),ui->velocity_checkbox, true,true,PADVAL);
  gtk_box_pack_start(GTK_BOX(opts_hbox2),ui->note_off_checkbox, true,true,PADVAL);

  gtk_box_pack_start(GTK_BOX(drumrox_ui_widget),GTK_WIDGET(ui->current_kit_label), false,false,5);
  gtk_box_pack_start(GTK_BOX(drumrox_ui_widget),gtk_hseparator_new(), false,false,5);
  gtk_box_pack_start(GTK_BOX(drumrox_ui_widget),opts_hbox1,false,false,5);
  gtk_box_pack_start(GTK_BOX(drumrox_ui_widget),opts_hbox2,false,false,5);


  ui->drumrox_widget = drumrox_ui_widget;
  ui->sample_table = NULL;
  ui->kit_combo = GTK_COMBO_BOX(kit_combo_box);
  ui->base_label = GTK_LABEL(base_label);
  ui->base_spin = GTK_SPIN_BUTTON(base_spin);
  ui->no_kit_label = no_kit_label;

  g_signal_connect(G_OBJECT(kit_combo_box),"changed",G_CALLBACK(kit_combobox_changed),ui);
  g_signal_connect(G_OBJECT(base_spin),"value-changed",G_CALLBACK(base_changed),ui);
  g_signal_connect(G_OBJECT(ui->panlaw_combo_box),"changed",G_CALLBACK(panlaw_combobox_changed),ui);
  g_signal_connect(G_OBJECT(ui->velocity_checkbox),"toggled",G_CALLBACK(ignore_velocity_toggled),ui);
  g_signal_connect(G_OBJECT(ui->note_off_checkbox),"toggled",G_CALLBACK(ignore_note_off_toggled),ui);

  gtk_widget_show_all(drumrox_ui_widget);
  gtk_widget_hide(no_kit_label);
}


static LV2UI_Handle instantiate (const LV2UI_Descriptor *descriptor,
                                 const char* plugin_uri,
                                 const char* bundle_path,
                                 LV2UI_Write_Function      write_function,
                                 LV2UI_Controller          controller,
                                 LV2UI_Widget*             widget,
                                 const LV2_Feature* const* features)
{

  //DrMrUi *ui = (DrMrUi*)malloc(sizeof(DrMrUi));

  std::cout << "  CDrumroxGTKGUI *ui = new CDrumroxGTKGUI = start" << std::endl;;

  CDrumroxGTKGUI *ui = new CDrumroxGTKGUI;

  ui->write = write_function;
  ui->controller = controller;
  ui->drumrox_widget = NULL;
  ui->map = NULL;
  ui->current_kit_index = -1;
  ui->samples = 0; //WHAT IS THAT?
  *widget = NULL;

  while (*features)
        {
         if (! strcmp ((*features)->URI, LV2_URID_URI "#map"))
            ui->map = (LV2_URID_Map *)((*features)->data);
         features++;
       }

  if (! ui->map)
     {
      fprintf (stderr, "LV2 host does not support urid#map.\n");
      free (ui);
      return 0;
     }

  map_drmr_uris (ui->map, &(ui->uris));

  ui->bundle_path = g_strdup (bundle_path);

  load_led_pixbufs(ui);

  lv2_atom_forge_init (&ui->forge, ui->map);

  build_drmr_ui (ui);

//  ui->kits = scan_kits();

  ui->kits.scan();

  ui->gain_quark = g_quark_from_string ("drumrox_gain_quark");
  ui->pan_quark = g_quark_from_string ("drumrox_pan_quark");
  ui->trigger_quark = g_quark_from_string ("drumrox_trigger_quark");

  ui->gain_sliders = NULL;
  ui->pan_sliders = NULL;
  ui->notify_leds = NULL;

  // store previous gain/pan vals to re-apply to sliders when we
  // change kits

  ui->gain_vals = (float*)malloc(32*sizeof(float));
  memset(ui->gain_vals,0,32*sizeof(float));

  ui->pan_vals = (float*) malloc(32*sizeof(float));
  memset (ui->pan_vals, 0, 32 * sizeof(float));


  ui->cols = 6;

  ui->forceUpdate = false;
  fill_kit_combo (ui->kit_combo, ui->kits.v_kits_names);

  ui->panlaw = PANLAW_LINEAR6;

  //Window w = gdk_x11_drawable_get_xid(gtk_widget_get_window(ui->drumrox_widget));
  //*widget = reinterpret_cast<LV2UI_Widget>(w);

  *widget = ui->drumrox_widget;

  return ui;
}


static void cleanup (LV2UI_Handle handle)
{
  std::cout << "void cleanup (LV2UI_Handle handle) /// GUI \n";


  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)handle;
  // seems qtractor likes to destory us
  // before calling, avoid double-destroy
  if (GTK_IS_WIDGET(ui->drumrox_widget))
      gtk_widget_destroy(ui->drumrox_widget);

  if (ui->notify_leds)
      free(ui->notify_leds);

  if (ui->gain_sliders)
      free(ui->gain_sliders);

  if (ui->pan_sliders) free(ui->pan_sliders);
     g_free(ui->bundle_path);

  if (led_on_pixbuf)
     g_object_unref(led_on_pixbuf);

  if (led_off_pixbuf)
     g_object_unref(led_off_pixbuf);

  //free_kits(ui->kits);
  //free(ui);

  delete ui;
}


struct slider_callback_data
{
  GtkRange* range;
  float val;
};


static gboolean slider_callback (gpointer data)
{
  struct slider_callback_data *cbd = (struct slider_callback_data*) data;

  if (GTK_IS_RANGE(cbd->range))
      gtk_range_set_value (cbd->range, cbd->val);

  free (cbd);

  return FALSE; // don't keep calling
}


static void port_event (LV2UI_Handle handle,
                        uint32_t     port_index,
                        uint32_t     buffer_size,
                        uint32_t     format,
                        const void*  buffer)
{
  std::cout << "GUI void port_event\n";

  DrumroxPortIndex index = (DrumroxPortIndex)port_index;
  CDrumroxGTKGUI* ui = (CDrumroxGTKGUI*)handle;

  if (index == DRUMROX_CORE_EVENT)
     {
      if (format == ui->uris.atom_eventTransfer)
         {
          LV2_Atom* atom = (LV2_Atom*)buffer;
          if (atom->type == ui->uris.atom_resource)
             {
              LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;

              if (obj->body.otype == ui->uris.get_state || obj->body.otype == ui->uris.ui_msg)
                 {
                   // both state and ui_msg are the same at the moment
                  const LV2_Atom* path = NULL;

                  lv2_atom_object_get (obj, ui->uris.kit_path, &path, 0);
                  if (path)
                     {
                      char *kitpath = (char*)LV2_ATOM_BODY(path);
                      if (! strncmp (kitpath, "file://", 7))
                          kitpath += 7;

                     char *realp = realpath (kitpath, NULL);
                     if (! realp)
                        {
                         fprintf (stderr, "Passed a path I can't resolve, bailing out\n");
                         return;
                        }

                     int kit_index;

                     //REWRITE!
                     for (kit_index = 0; kit_index < ui->kits.v_scanned_kits.size(); kit_index++)
                         if (! strcmp (ui->kits.v_scanned_kits[kit_index]->kit_xml_filename.c_str(), realp))
                              break;

                     if (kit_index < ui->kits.v_scanned_kits.size())
                        {
                         ui->kitReq = kit_index;
                         g_idle_add (kit_callback, ui);
                        }
                     else
                         fprintf(stderr,"Couldn't find kit %s\n",realp);

                     free (realp);
                    }

                  if (obj->body.otype == ui->uris.get_state)
                     { // read out extra state info
                      const LV2_Atom* ignvel = NULL;
                      const LV2_Atom* ignno = NULL;
                      const LV2_Atom* panlaw = NULL;

                      lv2_atom_object_get (obj, ui->uris.velocity_toggle, &ignvel, ui->uris.note_off_toggle, &ignno, ui->uris.panlaw, &panlaw, 0);

                      if (ignvel)
                          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->velocity_checkbox), ((const LV2_Atom_Bool*)ignvel)->body);

                      if (ignno)
                          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->note_off_checkbox), ((const LV2_Atom_Bool*)ignno)->body);

                      if (panlaw)
                          gtk_combo_box_set_active(GTK_COMBO_BOX(ui->panlaw_combo_box), ((const LV2_Atom_Int*)panlaw)->body);
                     }
                 }
             else
                 if (obj->body.otype == ui->uris.midi_info)
                    {
                     const LV2_Atom *midi_atom = NULL;
                     lv2_atom_object_get (obj, ui->uris.midi_event, &midi_atom, 0);

                     if (! midi_atom)
                        {
                         fprintf(stderr,"Midi info with no midi data\n");
                         return;
                        }

                     const uint8_t *data = (const uint8_t*)midi_atom;
                     uint8_t nn = data[1] - ui->baseNote;
                     sample_triggered (ui, nn);
                    }
                 else
                     fprintf(stderr, "Unknown resource type passed to ui.\n");
            }
            else
                fprintf(stderr, "Non resource message passed to ui.\n");
           }
          else
              fprintf(stderr, "Unknown format.\n");
         }
      else
          if (index == DRUMROX_BASENOTE)
             {
              int base_note = (int)(*((float*)buffer));
              if (base_note >= 21 && base_note <= 107)
                 {
                  setBaseLabel((int)base_note);
                  gtk_spin_button_set_value (ui->base_spin, base_note);
                  gtk_label_set_markup (ui->base_label, baseLabelBuf);
                  ui->baseNote = base_note;
                 }
             }
         else
             if (index >= DRUMROX_GAIN_01 && index <= DRUMROX_GAIN_32)
                {
                 float gain = *(float*)buffer;
                 int idx = index - DRUMROX_GAIN_01;
                 ui->gain_vals[idx] = gain;

                 if (idx < ui->samples && ui->gain_sliders)
                    {
                     struct slider_callback_data* data = (slider_callback_data*)malloc(sizeof(struct slider_callback_data));
                     data->range = GTK_RANGE(ui->gain_sliders[idx]);
                     data->val = gain;
                     g_idle_add (slider_callback, data);
                     //GtkRange* range = GTK_RANGE(ui->gain_sliders[idx]);
                     //gtk_range_set_value(range,gain);
                    }
                }
            else
                if (index >= DRUMROX_PAN_01 && index <= DRUMROX_PAN_32)
                   {
                    float pan = *(float*)buffer;
                    int idx = index - DRUMROX_PAN_01;
                    ui->pan_vals[idx] = pan;

                    if (idx < ui->samples && ui->pan_sliders)
                       {
                        struct slider_callback_data* data = (slider_callback_data*) malloc(sizeof(struct slider_callback_data));
                        data->range = GTK_RANGE(ui->pan_sliders[idx]);
                        data->val = pan;
                        g_idle_add (slider_callback, data);
                       }
                  }
}


static const void* extension_data (const char* uri)
{
  return NULL;
}

static const LV2UI_Descriptor descriptor = {
  DRMR_UI_URI,
  instantiate,
  cleanup,
  port_event,
  extension_data
};


LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
  if (index == 0)
     return &descriptor;
  else
      return NULL;

  /*
  switch (index)
         {
          case 0:
                 return &descriptor;
          default:
                 return NULL;
         }
         */
}

