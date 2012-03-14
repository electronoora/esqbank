/*

  ESQ-1/SQ-80 program data structure

  (c) 2012 Jani Halme <jani.halme@gmail.com>

*/

#ifndef _ESQ_PCB_H_
#define _ESQ_PCB_H_

typedef unsigned char u8;
typedef char s8;

typedef struct {
  s8 l1, l2, l3;
  u8 t1, t2, t3, t4;
  u8 lv;
  u8 t1v;
  u8 tk;
} esq_envelope;

typedef struct {
  u8 freq_ms;
  u8 l1_ms;
  u8 l2_ws;
  u8 delay_rh;
} esq_lfo;

typedef struct {
  u8 semitone;
  u8 finetune;
  u8 fm_src;
  s8 fm_modamt1;
  s8 fm_modamt2;
  u8 waveform;
  u8 dca_level_e;
  u8 am_src;
  s8 am_amt1;
  s8 am_amt2;
} esq_osc;

typedef struct {
  u8 dca4_modamt_a;
  u8 filter_fc_s;
  u8 q;
  u8 fc_src;
  s8 fc_modamt1_r;
  s8 fc_modamt2_m;
  s8 fc_modamt3_er;
  u8 glide_w;
  u8 split_point;
  u8 layer_program;
  u8 split_program;
  u8 split_layer_prog;
  u8 pan;
  u8 pan_mod_amt;
} esq_misc;

typedef struct {
  char name[6];
  esq_envelope env1;
  esq_envelope env2;
  esq_envelope env3;
  esq_envelope env4;
  esq_lfo lfo1;
  esq_lfo lfo2;
  esq_lfo lfo3;
  esq_osc osc1;
  esq_osc osc2;
  esq_osc osc3;
  esq_misc misc;
} esq_pcb;

#endif
