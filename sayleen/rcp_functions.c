/*
  RCP converter functions

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Mar.11.1999
  Oct.16.2002


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* ------------------------------------------------------------------- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

#include "rcp.h"
#include "rcp_functions.h"
#include "smf.h"

/* ------------------------------------------------------------------- */

/*
  Each functions return 0 if processing succeed.

  The result of processing will be stored in the member of structure
  RCP_DATA. It is available in 

      int ((RCP_DATA)rcp).result_smf[]

  The result SMF data will be stored with the terminater SMF_TERM.
  The max size of result_smf[] is RCP_MAX_RESULT_SMF.
  */

/* ------------------------------------------------------------------- */

int rcp_note_on(RCP_DATA *rcp, int track ) {

  int note, vel;

  note = rcp->track[track].event;
  vel  = rcp->track[track].vel;

  if ( vel != 0 && rcp->track[track].gate != 0 &&
       rcp->track[track].enabled == FLAG_TRUE ) {

    if ( note > 127 || note < 0 ) {
      /* invalid note number */
      return 1;
    }
    if ( vel > 127 || vel < 0 ) {
      /* invalid velocity value */
      return 1;
    }
    
    if ( rcp->track[track].key != 0x80 )
      note = (note+rcp->play_bias+rcp->track[track].key)%128;
    if ( note < 0 ) note=0;
    if ( note >127) note=127;
    
    if ( rcp->track[track].notes[note] == 0 ) {
      rcp->result_smf[0] = MIDI_NOTEON + rcp->track[track].midi_ch;
      rcp->result_smf[1] = note;
      rcp->result_smf[2] = vel;
      rcp->result_smf[3] = SMF_TERM;
    }

    rcp->track[track].notes[note] = rcp->track[track].gate;
    
    if ( rcp->track[track].notes_min > note )
      rcp->track[track].notes_min = note;
    if ( rcp->track[track].notes_max < note )
      rcp->track[track].notes_max = note;
    
    rcp->track[track].all_notes_expired = FLAG_FALSE;
  }

  return 0;
}

int rcp_note_off(RCP_DATA *rcp, int track, int note ) {

  int n;

  rcp->result_smf[0] = MIDI_NOTEOFF + rcp->track[track].midi_ch;
  rcp->result_smf[1] = note;
  rcp->result_smf[2] = 0;
  rcp->result_smf[3] = SMF_TERM;

  if ( rcp->track[track].notes_min == note ) {
    /* Was I the lowest note ? */
    for ( n=note+1 ; n<=rcp->track[track].notes_max ; n++ ) {
      if ( rcp->track[track].notes[n]==0 ) continue;
      rcp->track[track].notes_min = n;
      break;
    }
    if ( rcp->track[track].notes_min == note )
      rcp->track[track].notes_min = 127;
  }

  if ( rcp->track[track].notes_max == note ) {
    /* Was I the highest note ? */
    for  ( n=note-1 ; n>=rcp->track[track].notes_min ; n-- ) {
      if ( rcp->track[track].notes[n]==0 ) continue;
      rcp->track[track].notes_max = n;
      break;
    }
    if ( rcp->track[track].notes_max == note )
      rcp->track[track].notes_max = 0;
  }

  if ( rcp->track[track].notes_min == 127 &&
       rcp->track[track].notes_max == 0 ) {
    /* All notes expired. */
    rcp->track[track].all_notes_expired = FLAG_TRUE;
  }

  return 0;
}

int rcp_user_exclusive(RCP_DATA *rcp, int track ){

  int id;
  int v1,v2;
  int i,j,d,r;
  int size,check_sum;
  int finished;

  id = rcp->track[track].event - 0x90;
  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( id < 0 || id > 7 ) {
    /* Invalid number of user exclusive id */
    return 1;
  }
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of user exclusive */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of user exclusive */
    return 1;
  }

  check_sum = 0;
  finished = FLAG_FALSE;

  size=0;
  i=0;
  while ( i < RCP_USER_EXCLUSIVE_SIZE &&
	  rcp->user_exclusive[id][i] != 0xf7 ) {
    if ( rcp->user_exclusive[id][i] != 0x83 )
      size++;
    i++;
  }
  size++;

  rcp->result_smf[0] = 0xf0;
  rcp->result_smf[1] = size;
  i=0;
  j=2;
  while ( i<RCP_USER_EXCLUSIVE_SIZE &&
	  finished == FLAG_FALSE ) {
    d = rcp->user_exclusive[id][i++];

    switch( d ) {
    case 0x80:
      r = v1;
      break;

    case 0x81:
      r = v2;
      break;

    case 0x82:
      r = rcp->track[track].midi_ch;
      break;

    case 0x83:
      check_sum = 0;
      continue;
      break;

    case 0x84:
      r = 0x80 - (check_sum & 0x7f);
      break;

    case 0xf7:
      r = d;
      finished = FLAG_TRUE;
      break;

    default:
      r = d;
      break;
    }
    rcp->result_smf[j++] = r;
    check_sum += r;
    if ( finished == FLAG_TRUE ) break;
  }
  if ( i==RCP_USER_EXCLUSIVE_SIZE )
    rcp->result_smf[j++] = 0xf7;
  rcp->result_smf[j] = SMF_TERM;

  return 0;
}

int rcp_ch_exclusive(RCP_DATA *rcp, int track ){

  int v1,v2;
  int i,j,*l;
  int size,check_sum;
  int finished;
  int ptr;
  unsigned char *data = rcp->data;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of user exclusive */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of user exclusive */
    return 1;
  }

  ptr = rcp->track[track].current_ptr+4;
  size=0;
  while ( data[ptr+0] == RCP_2ND_EVENT ) {
    if ( data[ptr+2]==0xf7 ) size++;
    else size+=2;
    if ( data[ptr+2] == 0x83 ) size--;
    if ( data[ptr+3] == 0x83 ) size--;
    ptr+=4;
  }
  l = smf_number_conversion((long)size);

  ptr = rcp->track[track].current_ptr+4;
  j=0;
  check_sum = 0;
  finished = FLAG_FALSE;

  rcp->result_smf[j++] = 0xf0;
  i=0;
  do {
    rcp->result_smf[j++] = l[i];
  } while ( l[i++] >= 0x80 );

  while ( finished == FLAG_FALSE ) {
    int d[2],r;
    int i;
    
    if ( data[ptr+0] != RCP_2ND_EVENT ) break;

    d[0] = data[ptr+2];
    d[1] = data[ptr+3];
    ptr += 4;

    for ( i=0 ; i<2 ; i++ ) {
      switch( d[i] ) {
      case 0x80:
	r = v1;
	break;
	
      case 0x81:
	r = v2;
	break;
	
      case 0x82:
	r = rcp->track[track].midi_ch;
	break;
	
      case 0x83:
	check_sum = 0;
	continue;
	break;
	
      case 0x84:
	r = 0x80 - (check_sum & 0x7f);
	break;
	
      case 0xf7:
	r = d[i];
	finished = FLAG_TRUE;
	break;
	
      default:
	r = d[i];
	break;
      }
      rcp->result_smf[j] = r;
      check_sum += r;
      if ( j < RCP_MAX_RESULT_SMF_SIZE-1 ) j++;
      if ( finished == FLAG_TRUE ) break;
    }
  }
  rcp->result_smf[j] = SMF_TERM;
  rcp->track[track].current_ptr = ptr-4;

  return 0;
}

int rcp_exec_extern_prog(RCP_DATA *rcp, int track ){

  /* exec_extern_prog is not supported */

  /* skip all 2nd events */
  int p;
  p = rcp->track[track].current_ptr+4;
  while ( rcp->data[p+0] == RCP_2ND_EVENT ) {
    p+=4;
  }
  rcp->track[track].current_ptr = p-4;

  return 1;
}

int rcp_bank_and_prog(RCP_DATA *rcp, int track ){
  int prog, bank;

  prog = rcp->track[track].gate;
  bank = rcp->track[track].vel;

  if ( prog > 127 || prog < 0 ) {
    /* invalid program number */
    return 1;
  }
  if ( bank > 127 || bank < 0 ) {
    /* invalid tone bank number */
    return 1;
  }

  rcp->result_smf[0] = MIDI_CONTROL + rcp->track[track].midi_ch;
  rcp->result_smf[1] = SMF_CTRL_BANK_SELECT_M;
  rcp->result_smf[2] = bank;
  rcp->result_smf[3] = 0;        /* step time = 0 */
                        /* Should I send SMF_CTRL_BANK_SELECT_L in here ? */
  rcp->result_smf[4] = MIDI_PROGRAM + rcp->track[track].midi_ch;
  rcp->result_smf[5] = prog;
  rcp->result_smf[6] = SMF_TERM;

  return 0;
}

int rcp_key_scan(RCP_DATA *rcp, int track ){

  /* KeyScan is not supported */
  return -1;
}

int rcp_midi_ch_change(RCP_DATA *rcp, int track ){

  int ch;
  int last_port;

  ch = rcp->track[track].gate;

  if ( ch > 32 || ch < 0 ) {
    /* invalid midi channel number */
    return 1;
  }

  if ( ch == 0 ) {
    /* part off */
    rcp->track[track].enabled = FLAG_FALSE;
    return 0;
  }

  rcp->track[track].enabled = FLAG_TRUE;
  ch--;
  last_port = rcp->track[track].port;

  if ( ch < 16 ) rcp->track[track].port = 0;
  else if ( ch < 32 ) rcp->track[track].port = 1;
  /* port 2,3,... will follow */
 
  rcp->track[track].midi_ch = ch%16;

#ifdef ENABLE_PORT_CHANGE
  if ( last_port != rcp->track[track].port ) {
    rcp->result_smf[0] = MIDI_META;
    rcp->result_smf[1] = META_PORT;
    rcp->result_smf[2] = 1;      /* a byte follows */
    rcp->result_smf[3] = rcp->track[track].port;
    rcp->result_smf[4] = SMF_TERM;
  }
#endif /* ENABLE_PORT_CHANGE */

  return 0;
}

int rcp_tempo_change(RCP_DATA *rcp, int track ){

  int t1,t2,t3;
  long t;

  rcp->realtempo = rcp->tempo * rcp->track[track].gate / 64;
  t = 1000 * 1000 * 60 / (rcp->tempo * rcp->track[track].gate / 64);

  t1 = (int)((t>>16)&0xff);
  t2 = (int)((t>> 8)&0xff);
  t3 = (int)(t&0xff);

  rcp->result_smf[0] = MIDI_META;
  rcp->result_smf[1] = META_TEMPO;
  rcp->result_smf[2] = 3;
  rcp->result_smf[3] = t1;
  rcp->result_smf[4] = t2;
  rcp->result_smf[5] = t3;
  rcp->result_smf[6] = SMF_TERM;

  return 0;
}

int rcp_after_touch(RCP_DATA *rcp, int track ){
  int after_touch;

  after_touch = rcp->track[track].gate;
  if ( after_touch > 127 || after_touch < 0 ) {
    /* invalid after touch value */
    return 1;
  }

  rcp->result_smf[0] = MIDI_CHANPRES + rcp->track[track].midi_ch;
  rcp->result_smf[1] = after_touch;
  rcp->result_smf[2] = SMF_TERM;

  return 0;
}

int rcp_control_change(RCP_DATA *rcp, int track ){

  int number, val;

  number = rcp->track[track].gate;
  val    = rcp->track[track].vel;
  if ( number > 127 || number < 0 ) {
    /* invalid control number */
    return 1;
  }
  if ( val > 127 || val < 0 ) {
    /* invalid control value */
    return 1;
  }

  rcp->result_smf[0] = MIDI_CONTROL + rcp->track[track].midi_ch;
  rcp->result_smf[1] = number;
  rcp->result_smf[2] = val;
  rcp->result_smf[3] = SMF_TERM;
  
  return 0;
}

int rcp_program_change(RCP_DATA *rcp, int track ){

  int prog;

  prog = rcp->track[track].gate;
  if ( prog > 127 || prog < 0 ) {
    /* invalid program number */
    return 1;
  }

  rcp->result_smf[0] = MIDI_PROGRAM + rcp->track[track].midi_ch;
  rcp->result_smf[1] = prog;
  rcp->result_smf[2] = SMF_TERM;
  
  return 0;
}

int rcp_after_touch_poly(RCP_DATA *rcp, int track ){

  int key, val;

  key = rcp->track[track].gate;
  val = rcp->track[track].vel;
  if ( key > 127 || key < 0 ) {
    /* invalid after touch key number */
    return 1;
  }
  if ( val > 127 || val < 0 ) {
    /* invalid after touch value */
    return 1;
  }

  rcp->result_smf[0] = MIDI_PRESSURE + rcp->track[track].midi_ch;
  rcp->result_smf[1] = key;
  rcp->result_smf[2] = val;
  rcp->result_smf[3] = SMF_TERM;
  
  return 0;
}

int rcp_pitch_bend(RCP_DATA *rcp, int track ){

  int v1, v2;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of pitch bend */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of pitch bend */
    return 1;
  }

  rcp->result_smf[0] = MIDI_PITCHB + rcp->track[track].midi_ch;
  rcp->result_smf[1] = v1;
  rcp->result_smf[2] = v2;
  rcp->result_smf[3] = SMF_TERM;
  
  return 0;
}

int rcp_yamaha_base(RCP_DATA *rcp, int track ){

  int v1, v2;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of yamaha base */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of yamaha base */
    return 1;
  }

  rcp->track[track].yamaha_base[0] = v1;
  rcp->track[track].yamaha_base[1] = v2;
  
  return 0;
}

int rcp_yamaha_dev_name(RCP_DATA *rcp, int track ){

  int v1, v2;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of yamaha device number */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of yamaha device number */
    return 1;
  }

  rcp->yamaha_dev_id   = v1;
  rcp->yamaha_model_id = v2;
  
  return 0;
}

int rcp_yamaha_addr(RCP_DATA *rcp, int track ){

  int d1,d2;
  int a1,a2,a3;
  int v;

  d1 = rcp->yamaha_dev_id;
  d2 = rcp->yamaha_model_id;

  a1 = rcp->track[track].yamaha_base[0];
  a2 = rcp->track[track].yamaha_base[1];
  a3 = rcp->track[track].gate;

  v   = rcp->track[track].vel;

  if ( v > 127 || v < 0 ) {
    /* invalid value of yamaha device number */
    return 1;
  }
  if ( d1 > 127 || d1 < 0 ||
       d2 > 127 || d2 < 0 ) {
    return 1;
  }
  if ( a1 > 127 || a1 < 0 ||
       a2 > 127 || a2 < 0 || 
       a3 > 127 || a3 < 0 ) {
    /* Invalid value of yamaha address */
    return 1;
  }

  rcp->result_smf[0] = 0xf0;   /* Exclusive status */
  rcp->result_smf[1] = 8;      /* Packet length */
  rcp->result_smf[2] = 0x43;   /* Vendor ID - YAMAHA */
  rcp->result_smf[3] = d1;     /* Device ID */
  rcp->result_smf[4] = d2;     /* Model ID */
  rcp->result_smf[5] = a1;
  rcp->result_smf[6] = a2;
  rcp->result_smf[7] = a3;
  rcp->result_smf[8] = v;
  rcp->result_smf[9] = 0xf7;
  rcp->result_smf[10] = SMF_TERM;

  return 0;
}

int rcp_yamaha_xg_ad(RCP_DATA *rcp, int track ){

  int d1,d2;
  int a1,a2,a3;
  int v;

  d1 = 0x10;
  d2 = 0x4c;

  a1 = rcp->track[track].yamaha_base[1];
  a2 = rcp->track[track].yamaha_base[1];
  a3 = rcp->track[track].gate;

  v   = rcp->track[track].vel;

  if ( v > 127 || v < 0 ) {
    /* invalid value of yamaha device number */
    return 1;
  }
  if ( a1 > 127 || a1 < 0 ||
       a2 > 127 || a2 < 0 || 
       a3 > 127 || a3 < 0 ) {
    /* Invalid value of yamaha address */
    return 1;
  }

  rcp->result_smf[0] = 0xf0;   /* Exclusive status */
  rcp->result_smf[1] = 8;      /* Packet length */
  rcp->result_smf[2] = 0x43;   /* Vendor ID - YAMAHA */
  rcp->result_smf[3] = d1;     /* Device ID */
  rcp->result_smf[4] = d2;     /* Model ID */
  rcp->result_smf[5] = a1;
  rcp->result_smf[6] = a2;
  rcp->result_smf[7] = a3;
  rcp->result_smf[8] = v;
  rcp->result_smf[9] = 0xf7;
  rcp->result_smf[10] = SMF_TERM;

  return 0;

}

int rcp_roland_dev(RCP_DATA *rcp, int track ){

  int v1, v2;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of roland device number */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of roland device number */
    return 1;
  }

  rcp->roland_dev_id   = v1;
  rcp->roland_model_id = v2;
  
  return 0;
}

int rcp_roland_base(RCP_DATA *rcp, int track ){

  int v1, v2;

  v1 = rcp->track[track].gate;
  v2 = rcp->track[track].vel;
  if ( v1 > 127 || v1 < 0 ) {
    /* invalid value of roland base */
    return 1;
  }
  if ( v2 > 127 || v2 < 0 ) {
    /* invalid value of roland base */
    return 1;
  }

  rcp->track[track].roland_base[0] = v1;
  rcp->track[track].roland_base[1] = v2;
  
  return 0;
}

int rcp_roland_para(RCP_DATA *rcp, int track ){

  int d1,d2;
  int a1,a2,a3;
  int v;

  d1 = rcp->roland_dev_id;
  d2 = rcp->roland_model_id;

  a1 = rcp->track[track].roland_base[0];
  a2 = rcp->track[track].roland_base[1];
  a3 = rcp->track[track].gate;

  v  = rcp->track[track].vel;

  if ( v > 127 || v < 0 ) {
    /* Invalid value of roland data */
    return 1;
  }
  if ( d1 > 127 || d1 < 0 ||
       d2 > 127 || d2 < 0 ) {
    return 1;
  }
  if ( a1 > 127 || a1 < 0 ||
       a2 > 127 || a2 < 0 || 
       a3 > 127 || a3 < 0 ) {
    /* Invalid value of roland address */
    return 1;
  }

  rcp->result_smf[0] = 0xf0;   /* Exclusive status */
  rcp->result_smf[1] = 10;     /* Packet length */
  rcp->result_smf[2] = 0x41;   /* Vendor ID - Roland */
  rcp->result_smf[3] = d1;     /* Device ID */
  rcp->result_smf[4] = d2;     /* Model ID */
  rcp->result_smf[5] = 0x12;   /* Roland Command - DT1 */
  rcp->result_smf[6] = a1;
  rcp->result_smf[7] = a2;
  rcp->result_smf[8] = a3;
  rcp->result_smf[9] = v;
  rcp->result_smf[10]= 0x80-((a1+a2+a3+v)&0x7f);
  rcp->result_smf[11]= 0xf7;
  rcp->result_smf[12]= SMF_TERM;

  return 0;
}

int rcp_key_change(RCP_DATA *rcp, int track ){

  int sf,mi;
  int v, vv;

  v = rcp->track[track].step;
  vv = v%0x10;
  sf = vv > 0x07 ? (0x100-vv)%0x100 : vv;
  mi = v  > 0x0f ? 1:0;

  rcp->result_smf[0] = MIDI_META;
  rcp->result_smf[1] = META_KEYSIG;
  rcp->result_smf[2] = 0x02;
  rcp->result_smf[3] = sf;
  rcp->result_smf[4] = mi;
  rcp->result_smf[5] = SMF_TERM;

  return 0;
}

int rcp_comment_start(RCP_DATA *rcp, int track ){

  int ptr;
  int length;
  int i,j;
  int *l;
  unsigned char *data = rcp->data;

  length=2;
  ptr = rcp->track[track].current_ptr+4;
  while ( data[ptr+0] == RCP_2ND_EVENT ) {
    length+=2;
    ptr+=4;
    if ( ptr >= rcp->track[track].top + rcp->track[track].size )
      break;
  }

  i=0;
  rcp->result_smf[i++] = MIDI_META;
  rcp->result_smf[i++] = META_TEXT;

  l=smf_number_conversion((long)length);
  j=0;
  do {
    rcp->result_smf[i] = l[j];
    if ( i < RCP_MAX_RESULT_SMF_SIZE ) i++;
  }  while ( l[j++] >= 0x80 );

  ptr = rcp->track[track].current_ptr;
  rcp->result_smf[i++] = data[ptr+2] == 0 ? 0x20 : data[ptr+2];
  rcp->result_smf[i++] = data[ptr+3] == 0 ? 0x20 : data[ptr+3];
  ptr+=4;
  while( data[ptr+0] == RCP_2ND_EVENT ) {
    if ( i >= RCP_MAX_RESULT_SMF_SIZE - 2 ) continue;

    rcp->result_smf[i++] = data[ptr+2] == 0 ? 0x20 : data[ptr+2];
    rcp->result_smf[i++] = data[ptr+3] == 0 ? 0x20 : data[ptr+3];
    ptr+=4;
    if ( ptr >= rcp->track[track].top + rcp->track[track].size ) {
      /* Comment overflow */
      return 1;
    }
  }

  rcp->result_smf[i] = SMF_TERM;
  rcp->track[track].current_ptr = ptr-4;

  return 0;
}

int rcp_loop_end(RCP_DATA *rcp, int track ){

  LOOP_DATA *l;
  int c;
  int d = rcp->track[track].loop_depth;
  if ( d == 0 ) return -1;

  l = &rcp->track[track].loop[d];
  c = l->loop_count;
  if ( c == -1 ) {
    c = rcp->track[track].step;
    if ( rcp->is_player == FLAG_FALSE ) {
      if ( c==0 || c==255 ) c=2;

      /* loop_count =0 means infinite loops.
	 loop_count = 255 also means pseudo infinite loops.
	 We treat these infinite loops as two times loops.
	 This conversion prevents the result of conversion from increasing
	 the size, but the music may be curious...
	 I'm planning to treats these with some fade-out features.
	 */
    }
  }
  c--;
  if ( c != 0 ) {
    rcp->track[track].current_ptr = l->top_ptr;
    rcp->track[track].same_measure_flag = l->same_measure_flag;
    rcp->track[track].same_measure_ptr = l->same_measure_ptr;
    if ( c == -1 ) c=0; /* for infinite loop */
  }
  else
    d--;

  l->loop_count = c;
  rcp->track[track].loop_depth = d;

  return 0;
}

int rcp_loop_start(RCP_DATA *rcp, int track ){

  LOOP_DATA *l;
  int d = rcp->track[track].loop_depth;
  d++;
  if ( d >= RCP_MAX_LOOPS ) return -1;

  l = &rcp->track[track].loop[d];

  l->top_ptr = rcp->track[track].current_ptr;
  l->loop_count = -1;                        /* initial value */
  l->same_measure_flag = rcp->track[track].same_measure_flag;
  l->same_measure_ptr  = rcp->track[track].same_measure_ptr;

  rcp->track[track].loop_depth = d;

  return 0;
}

int rcp_same_measure(RCP_DATA *rcp, int track ){

  if (  rcp->track[track].same_measure_flag == FLAG_FALSE ) {
    /* enter the first SAME_MEASURE */
    int ptr,adr;
    rcp->track[track].same_measure_ptr = rcp->track[track].current_ptr;
    rcp->track[track].same_measure_flag = FLAG_TRUE;

  resame:
    adr = rcp->track[track].current_ptr;
    ptr = (rcp->data[adr+2]&0xfc) + rcp->data[adr+3]*256 + rcp->track[track].top;
    if ( ptr != adr &&
	 ptr < rcp->track[track].top + rcp->track[track].size ) {
      rcp->track[track].current_ptr = ptr;
      if ( rcp->data[ptr+0] == RCP_SAME_MEASURE ) { goto resame; }
      rcp->track[track].current_ptr-=4;
    }
  }
  else {             /* same as measure_end */
    rcp->track[track].same_measure_flag = FLAG_FALSE;
    rcp->track[track].current_ptr = rcp->track[track].same_measure_ptr;
  }

  return 0;
}

int rcp_measure_end(RCP_DATA *rcp, int track ){

  if (  rcp->track[track].same_measure_flag == FLAG_TRUE ) {
    rcp->track[track].same_measure_flag = FLAG_FALSE;
    rcp->track[track].current_ptr = rcp->track[track].same_measure_ptr;
  }

  return 0;
}

int rcp_end_of_track(RCP_DATA *rcp, int track ){

  rcp->result_smf[0] = MIDI_META;
  rcp->result_smf[1] = META_EOT;
  rcp->result_smf[2] = 0;
  rcp->result_smf[3] = SMF_TERM;

  return 0;
}

/* ------------------------------------------------------------------- */
/*
  These following functions are not implemented.
  But I think these are tooooo obsolete so the sabotage will not cause
  any problems.
  */

int rcp_dx7_function(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_dx_parameter(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_dx_rerf(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx_function(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_fb01_parameter(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_fb01_system(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx81z_vced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx81z_aced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx81z_pced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx81z_system(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx81z_effect(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_dx7_2_remote_sw(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_dx7_2_aced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_dx7_2_pced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_tx802_pced(RCP_DATA *rcp, int track ){
  return -1;
}

int rcp_mks_7(RCP_DATA *rcp, int track ){
  return -1;
}
