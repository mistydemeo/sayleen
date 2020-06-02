/*
  RCP converter engine

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Feb.05.1999
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

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifdef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif

#include "rcp.h"
#include "smf.h"
#include "rcp_functions.h"
#include "gettext_wrapper.h"
#include "version.h"

/* ------------------------------------------------------------------- */

static unsigned char *rcptomid_name      = "rcptomid";
static unsigned char *undefined_date     = "2000-01-01";
static unsigned char *undefined_filename = "(NO NAME)";

/* ------------------------------------------------------------------- */

#ifdef RCP_DEBUG
# define RCPMSG(x,y) fprintf(stderr,x,y)
#else
# define RCPMSG(x,y)
#endif


typedef struct _TEMPO_EVENT_SEQUENCE {

  int absolute_step;
  int t1;
  int t2;
  int t3;

  struct _TEMPO_EVENT_SEQUENCE *next;

} TEMPO_EVENT_SEQUENCE;

/* ------------------------------------------------------------------- */

static TEMPO_EVENT_SEQUENCE *TES_top = (TEMPO_EVENT_SEQUENCE *)NULL;

int rcptomid_init_track_buffer( RCP_DATA * );
int rcptomid_read_rcp_header( RCP_DATA * );
int rcptomid_set_new_event( RCP_DATA *, int );

static int flush_event( RCP_DATA *, int );
static int set_tempo_track( RCP_DATA * );
static unsigned char *set_smf_data( RCP_DATA * );
static int insert_tempo_event( RCP_DATA * );
static int init_track_header( RCP_DATA *, int );

/* ------------------------------------------------------------------- */

/* Converter engine */

unsigned char *rcptomid( RCP_DATA *rcp ) {

  unsigned char *smf;
  int track;

  int gate_min;
  int n,nt;
  int s;

  if ( rcptomid_init_track_buffer( rcp ) != 0 ) return NULL;
  if ( rcptomid_read_rcp_header( rcp )   != 0 ) return NULL;

  for ( track = 0 ; track < rcp->tracks ; track++ ) {
    rcp->roland_dev_id   = 0x10;
    rcp->roland_model_id = 0x16;
    rcp->yamaha_dev_id   = 0x10;
    rcp->yamaha_model_id = 0x16;
    
    rcp->step            = 0;

    init_track_header( rcp, track );
    rcp->result_smf[0]   = SMF_TERM;

    while ( rcp->track[track].finished == FLAG_FALSE ) {

      if ( rcp->track[track].all_notes_expired == FLAG_TRUE )
	goto rcptomid_expire_event;
      
      /* checks for each notes' gate time is expired */
      gate_min = rcp->track[track].step;
      nt = -1;
      for ( n = rcp->track[track].notes_min ;
	    n <= rcp->track[track].notes_max ; n++ ) {
	if ( rcp->track[track].notes[n] == 0 ) continue;
	if ( rcp->track[track].notes[n] <= gate_min ) {
	  gate_min = rcp->track[track].notes[n];
	  nt = n;
	}
      }
      if ( nt < 0 ) goto rcptomid_expire_event;

      /* note-off */
      rcp->step += gate_min;
      rcp->track[track].delta_step += gate_min;
      rcp->track[track].total_step += gate_min;
      rcp->track[track].step -= gate_min;
      for ( n = rcp->track[track].notes_min ;
	    n <=rcp->track[track].notes_max ; n++ ) {
	if ( rcp->track[track].notes[n] == gate_min ) {
	  rcp_note_off( rcp, track, n );
	  flush_event( rcp, track );
	}
	rcp->track[track].notes[n] -= gate_min;
	if ( rcp->track[track].notes[n] < 0 ) {
	  rcp->track[track].notes[n] = 0;
	}
      }
      continue;

      /* checks for step time is expired */

    rcptomid_expire_event:

      s = rcp->track[track].step;
      rcp->step += s;
      rcp->track[track].delta_step += s;
      rcp->track[track].total_step += s;
      rcp->track[track].step = 0;
      for ( n = rcp->track[track].notes_min ; 
	    n <=rcp->track[track].notes_max ; n++ ) {
	rcp->track[track].notes[n] -= s;
	if ( rcp->track[track].notes[n] < 0 ) {
	  rcp->track[track].notes[n] = 0;
	}
      }
      while ( rcp->track[track].step == 0 ) {
	rcptomid_set_new_event( rcp, track );
	if ( rcp->track[track].finished == FLAG_FALSE )
	  flush_event( rcp, track );
	else break;
      }
    }

    while ( 1 ) {
      gate_min = 65536*2; /* large enough */
      for ( n=0 ; n<RCP_MAX_NOTES ; n++ ) {
	if ( rcp->track[track].notes[n] <= 0 ) continue;
	if ( rcp->track[track].notes[n] <= gate_min ) {
	  gate_min = rcp->track[track].notes[n];
	}
      }
      if ( gate_min == 65536*2 ) break;
      for ( n=0 ; n<RCP_MAX_NOTES ; n++ ) {
	if ( rcp->track[track].notes[n] == gate_min ) {
	  rcp_note_off( rcp, track, n );
	  flush_event( rcp, track );
	}
	rcp->track[track].notes[n]-=gate_min;
	if ( rcp->track[track].notes[n] < 0 )
	  rcp->track[track].notes[n] = 0;
      }
    }

    rcp->result_smf[0] = MIDI_META;
    rcp->result_smf[1] = META_EOT;
    rcp->result_smf[2] = 0;
    rcp->result_smf[3] = SMF_TERM;
    flush_event( rcp, track );
  }

  /* set up first / tempo track */
  set_tempo_track( rcp );

  /* create SMF data */
  smf = set_smf_data( rcp );
  
  return smf;
}

/* ------------------------------------------------------------------- */

int rcptomid_init_track_buffer( RCP_DATA *rcp ) {

  int track,n;

  rcp->roland_dev_id   = 0x10;
  rcp->roland_model_id = 0x16;
  rcp->yamaha_dev_id   = 0x10;
  rcp->yamaha_model_id = 0x16;

  rcp->step            = -1;
  rcp->result_smf[0]   = SMF_TERM;

  for ( track=0 ; track<RCP_MAX_TRACKS ; track++ ) {
    rcp->track[track].enabled    = FLAG_FALSE;
    rcp->track[track].finished   = FLAG_FALSE;

    rcp->track[track].gate       = 0;
    rcp->track[track].step       = 1;
    rcp->track[track].delta_step = -1;   /* initial value should be -1 */
    rcp->track[track].total_step = -1;

    rcp->track[track].yamaha_base[0]  = -1; /* not specified */
    rcp->track[track].roland_base[0]  = -1; /* not specified */

    rcp->track[track].loop_depth      = 0;
    rcp->track[track].same_measure_flag = FLAG_FALSE;

    rcp->track[track].current_ptr     = 0;

    for ( n=0 ; n<RCP_MAX_NOTES ; n++ )
      rcp->track[track].notes[n] = 0;
    rcp->track[track].notes_min = 127;
    rcp->track[track].notes_max = 0;
    rcp->track[track].all_notes_expired = FLAG_TRUE;

    rcp->track[track].smf =
      (unsigned char *)malloc(sizeof(unsigned char)*RCP_DEFAULT_SMF_SIZE);
    if ( rcp->track[track].smf == NULL ) {
      /* Memory exhaust */
      return 1;
    }
    rcp->track[track].smf_ptr   = 0;
  }

  TES_top = (TEMPO_EVENT_SEQUENCE *)malloc(sizeof(TEMPO_EVENT_SEQUENCE));
  if ( TES_top == (TEMPO_EVENT_SEQUENCE *)NULL ) {
    /* Memory exhaust */
    return 1;
  }
  TES_top->absolute_step = 0;    /* Initial value */
  TES_top->next = NULL;

  rcp->smf_tempo_track = (unsigned char *)malloc(sizeof(unsigned char)*RCP_DEFAULT_SMF_SIZE);
  if ( rcp->smf_tempo_track == (unsigned char *)NULL ) {
    /* Memory exhaust */
    return 1;
  }
  rcp->smf_tempo_track_ptr = 0;

  return 0;
}

int rcptomid_read_rcp_header( RCP_DATA *rcp ) {

  int i,j;
  int t;
  int base,size,max_st;
  unsigned char *ptr;

  ptr = rcp->data;

  /* check 1st header */

  if ( strncmp( ptr, RCP_HEADER_STRING, 28 ) ==0 ) {
    rcp->rcp = FLAG_TRUE;
    rcp->g36 = FLAG_FALSE;
  } else if ( strncmp( ptr, STEDDATA_HEADER_STRING, 8 ) ==0 &&
	      strncmp( ptr+12, STEDDATA_HEADER_STRING+12, 16 ) ==0 ) {
    rcp->steddata = FLAG_TRUE;
    rcp->rcp = FLAG_TRUE;
    rcp->g36 = FLAG_FALSE;
  } else {
    return -1;
  }

  /* title, memo */

  for ( i=0 ; i<65 ; i++ )
    rcp->title[i] = '\0';
  for ( i=0 ; i<337 ; i++ )
    rcp->memo[i] = '\0';
  memcpy( rcp->title, ptr+0x0020, 64  );  /* It should be SJIS */
  memcpy( rcp->memo,  ptr+0x0060, 336 );  /* It should be SJIS */

  /* timebase, tempo, etc */

  rcp->timebase  = ptr[0x01c0] + ptr[0x01e7]*256;
  if ( rcp->timebase>480 || rcp->timebase<0 ) return 1;
  rcp->tempo     = ptr[0x01c1];
  rcp->realtempo = ptr[0x01c1];
  rcp->beat_h    = ptr[0x01c2];
  rcp->beat_l    = ptr[0x01c3];
  rcp->key       = ptr[0x01c4];
  rcp->play_bias = ptr[0x01c5];
  rcp->rtm_delta = rcp->timebase / 24;

  rcp->tracks   = ptr[0x01e6];
  if ( rcp->tracks == 0 ) rcp->tracks = 18;
  /*if ( rcp->tracks !=18 && rcp->tracks !=36 ) return 1;*/

  t = 1000 * 1000 * 60 / rcp->tempo;
  TES_top->t1 = (int)((t>>16)&0xff);
  TES_top->t2 = (int)((t>> 8)&0xff);
  TES_top->t3 = (int)(t&0xff);

  RCPMSG("TITLE     : %s\n", rcp->title);
  RCPMSG("MEMO      : %s\n", rcp->memo);
  RCPMSG("TIMEBASE  : %d\n", rcp->timebase);
  RCPMSG("TEMPO     : %d\n", rcp->tempo);
  RCPMSG("BEAT_H    : %d\n", rcp->beat_h);
  RCPMSG("BEAT_L    : %d\n", rcp->beat_l);
  RCPMSG("KEY       : %d\n", rcp->key);
  RCPMSG("PLAY BIAS : %d\n", rcp->play_bias);
  RCPMSG("TRACKS    : %d\n\n", rcp->tracks);

  /* user exclusive definition */

  for ( i=0 ; i<RCP_MAX_USER_EXCLUSIVE ; i++ ) {
    for ( j=0 ; j<RCP_USER_EXCLUSIVE_SIZE ; j++ ) {
      rcp->user_exclusive[i][j] = ptr[0x0406 + i*48 + 24+j];
    }
  }

  /* track headers */

  base = 0x0586;
  size = 0;
  max_st = 0;
  for ( t=0 ; t<rcp->tracks ; t++ ) {
    int ch;

    base = base + size;
    rcp->track[t].top = base;

    size = ptr[base+0] + ptr[base+1]*256;
    rcp->track[t].size = size;

    ch = ptr[base+4];
    if ( ch == 0xff ) {
      rcp->track[t].enabled = FLAG_FALSE;
      rcp->track[t].midi_ch = 0;
      rcp->track[t].port    = 0;
    }
    else {
      rcp->track[t].enabled = FLAG_TRUE;
      rcp->track[t].midi_ch = ch&0x0f;
      if ( rcp->steddata ) {
	rcp->track[t].port    = ch>>4&0x0f;
      } else {
	rcp->track[t].port  = ch>0x10 ? 1:0;
      }
    }

    if ( ptr[base+5] >= 0x80 )
      rcp->track[t].key     = 0x80;
    else {
      if ( ptr[base+5] > 63 ) 
	rcp->track[t].key   = ptr[base+5]-128;
      else
	rcp->track[t].key   = ptr[base+5];
    }
    rcp->track[t].st        = ptr[base+6];
    rcp->track[t].mode      = ptr[base+7];

    i = (rcp->track[t].st > 0x7f) ? rcp->track[t].st-0x100:0;
    if ( max_st > i ) max_st = i;

    if ( rcp->track[t].mode == 1 )
      rcp->track[t].enabled = FLAG_FALSE;

    for ( j=0 ; j<RCP_MAX_COMMENT_SIZE ; j++ ) {
      rcp->track[t].comment[j] = ptr[base+8+j];   /* It should be SJIS */
    }

    rcp->track[t].current_ptr = base+8+RCP_MAX_COMMENT_SIZE;

    RCPMSG("Track   : %d\n", t);
    RCPMSG("Base    : %d\n", rcp->track[t].top);
    RCPMSG("Size    : %d\n", rcp->track[t].size);
    RCPMSG("MIDI CH : %d\n", rcp->track[t].midi_ch);
    RCPMSG("Key     : %d\n", rcp->track[t].key);
    RCPMSG("ST      : %d\n", rcp->track[t].st);
    RCPMSG("Mode    : %d\n", rcp->track[t].mode);
    RCPMSG("Comment : %s\n\n", rcp->track[t].comment);
  }

  for ( t=0 ; t<rcp->tracks ; t++ ) {
    i = rcp->track[t].st > 0x7f ? rcp->track[t].st - 0x100 : rcp->track[t].st;
    rcp->track[t].step = 1+i-max_st;
  }

  return 0;
}

static int init_track_header( RCP_DATA *rcp, int track ) {

  int i,j;

  if ( rcp->track[track].enabled == FLAG_FALSE ) return 0;

  /* track memo */
  i=0;
  rcp->result_smf[i++] = MIDI_META;
  rcp->result_smf[i++] = META_SEQNAME;
  rcp->result_smf[i++] = RCP_MAX_COMMENT_SIZE;
  for ( j=0 ; j<RCP_MAX_COMMENT_SIZE ; j++ ) {
    unsigned char c = rcp->track[track].comment[j];
    if ( c == 0 ) c = 0x20;
    rcp->result_smf[i++] = c;
  }

#ifdef ENABLE_PORT_CHANGE
  /* MIDI channel */
  rcp->result_smf[i++] = 0; /* delta time = 0 */
  rcp->result_smf[i++] = MIDI_META;
  rcp->result_smf[i++] = META_PORT;
  rcp->result_smf[i++] = 1;  /* one byte follow */
  rcp->result_smf[i++] = rcp->track[track].port;
#endif /* ENABLE_PORT_CHANGE */

  rcp->result_smf[i++] = SMF_TERM;
  rcp->track[track].delta_step = 0;
  flush_event( rcp, track );

  rcp->step = -1;
  rcp->track[track].delta_step = -1;

  return 0;
}

int rcptomid_set_new_event( RCP_DATA *rcp, int t ) {

  int ptr;
  unsigned char *data;
  int ret;

  ret = 0;
  data = rcp->data;
  ptr  = rcp->track[t].current_ptr;

  if ( ptr > rcp->track[t].top + rcp->track[t].size ) {
    rcp->track[t].finished = FLAG_TRUE;
    return -1;
  }

  rcp->track[t].event = data[ptr+0];
  rcp->track[t].step  = data[ptr+1];
  rcp->track[t].gate  = data[ptr+2];
  rcp->track[t].vel   = data[ptr+3];

  /*
  fprintf(stderr,"%d : %02x %d %d %d\n", t, 
	  rcp->track[t].event,
	  rcp->track[t].step,
	  rcp->track[t].gate,
	  rcp->track[t].vel );
	  */

  if ( rcp->track[t].event < 0x80 ) {     /* note event */
    rcp_note_on( rcp, t );
  } else {
    switch ( rcp->track[t].event ) {

    case RCP_USER_EXCLUSIVE_1:
    case RCP_USER_EXCLUSIVE_2:
    case RCP_USER_EXCLUSIVE_3:
    case RCP_USER_EXCLUSIVE_4:
    case RCP_USER_EXCLUSIVE_5:
    case RCP_USER_EXCLUSIVE_6:
    case RCP_USER_EXCLUSIVE_7:
    case RCP_USER_EXCLUSIVE_8:
      rcp_user_exclusive( rcp, t );
      break;

    case RCP_CH_EXCLUSIVE:
      rcp_ch_exclusive( rcp, t );
      break;

    case RCP_EXEC_EXTERN_PROG:
      rcp_exec_extern_prog( rcp, t );
      ret = 1;
      break;

    case RCP_BANK_AND_PROG:
      rcp_bank_and_prog( rcp, t );
      break;

    case RCP_KEY_SCAN:
      rcp_key_scan( rcp, t );
      break;

    case RCP_MIDI_CH_CHANGE:
      rcp_midi_ch_change( rcp, t );
      break;

    case RCP_TEMPO_CHANGE:
      rcp_tempo_change( rcp, t );
      break;

    case RCP_AFTER_TOUCH:
      rcp_after_touch( rcp, t );
      break;

    case RCP_CONTROL_CHANGE:
      rcp_control_change( rcp, t );
      break;

    case RCP_PROGRAM_CHANGE:
      rcp_program_change( rcp, t );
      break;

    case RCP_AFTER_TOUCH_POLY:
      rcp_after_touch_poly( rcp, t );
      break;

    case RCP_PITCH_BEND:
      rcp_pitch_bend( rcp, t );
      break;

    case RCP_YAMAHA_BASE:
      rcp_yamaha_base( rcp, t );
      break;

    case RCP_YAMAHA_DEV_NUM:
      rcp_yamaha_dev_name( rcp, t );
      break;

    case RCP_YAMAHA_ADDR:
      rcp_yamaha_addr( rcp, t );
      break;

    case RCP_YAMAHA_XG_AD:
      rcp_yamaha_xg_ad( rcp, t );
      break;

    case RCP_ROLAND_BASE:
      rcp_roland_base( rcp, t );
      break;

    case RCP_ROLAND_PARA:
      rcp_roland_para( rcp, t );
      break;

    case RCP_ROLAND_DEV:     
      rcp_roland_dev( rcp, t );
      break;

    case RCP_KEY_CHANGE:
      rcp_key_change( rcp, t );
      ret = 1;
      break;

    case RCP_COMMENT_START:
      rcp_comment_start( rcp, t );
      ret = 1;
      break;

    case RCP_LOOP_END:
      rcp_loop_end( rcp, t );
      ret = 1;
      break;

    case RCP_LOOP_START:
      rcp_loop_start( rcp, t );
      ret = 1;
      break;

    case RCP_SAME_MEASURE:
      rcp_same_measure( rcp, t );
      ret = 1;
      break;

    case RCP_MEASURE_END:
      rcp_measure_end( rcp, t );
      ret = 1;
      break;

    case RCP_END_OF_TRACK:
      rcp_end_of_track( rcp, t );
      ret=-1;
      break;

    case RCP_DX7_FUNCTION:
      rcp_dx7_function( rcp, t );
      break;

    case RCP_DX_PARAMETER:
      rcp_dx_parameter( rcp, t );
      break;

    case RCP_DX_RERF:
      rcp_dx_rerf( rcp, t );
      break;

    case RCP_TX_FUNCTION:
      rcp_tx_function( rcp, t );
      break;

    case RCP_FB01_PARAMETER:
      rcp_fb01_parameter( rcp, t );
      break;

    case RCP_FB01_SYSTEM:
      rcp_fb01_system( rcp, t );
      break;

    case RCP_TX81Z_VCED:
      rcp_tx81z_vced( rcp, t );
      break;

    case RCP_TX81Z_ACED:
      rcp_tx81z_aced( rcp, t );
      break;

    case RCP_TX81Z_PCED:
      rcp_tx81z_pced( rcp, t );
      break;

    case RCP_TX81Z_SYSTEM:
      rcp_tx81z_system( rcp, t );
      break;

    case RCP_TX81Z_EFFECT:
      rcp_tx81z_effect( rcp, t );
      break;

    case RCP_DX7_2_REMOTE_SW:
      rcp_dx7_2_remote_sw( rcp, t );
      break;

    case RCP_DX7_2_ACED:
      rcp_dx7_2_aced( rcp, t );
      break;

    case RCP_DX7_2_PCED:
      rcp_dx7_2_pced( rcp, t );
      break;

    case RCP_TX802_PCED:
      rcp_tx802_pced( rcp, t );
      break;

    case RCP_MKS_7:
      rcp_mks_7( rcp, t );
      break;

    case RCP_2ND_EVENT:
      ret = 1;
      break;

    default:
      ret=-1;
      break;
    }
  }

  if ( ret == 1 )
    rcp->track[t].step = 0;

  ptr=rcp->track[t].current_ptr;
  ptr+=4;
  if ( ptr >= rcp->track[t].top + rcp->track[t].size ||
       ret == -1 ) {
    rcp->track[t].finished = FLAG_TRUE;
    rcp->track[t].step=-1;
    ptr = -1;
  }
  rcp->track[t].current_ptr = ptr;

  return  ret;
}

static int flush_event( RCP_DATA *rcp, int track ) {

  int i,*len;
  int ptr;

  if ( rcp->result_smf[0] == SMF_TERM ) return 0;

  /* is the event set-tempo ? */
  if ( rcp->result_smf[0] == MIDI_META &&
       rcp->result_smf[1] == META_TEMPO ) {
    
    insert_tempo_event( rcp );
    goto flush_event_end;
  }
  
  /* is the track disabled ? */
  if ( rcp->track[track].enabled == FLAG_FALSE && 
       rcp->result_smf[0] != MIDI_NOTEOFF ) {
    goto flush_event_end;
  }
  
  /* flushing smf data */
  i=0;
  ptr = rcp->track[track].smf_ptr;
  
  if ( rcp->track[track].delta_step < 0 ) {
    fprintf(stderr,"%d\n",(int)rcp->track[track].delta_step);
  }

  len = smf_number_conversion( rcp->track[track].delta_step );
  do {
    rcp->track[track].smf[ptr++] = len[i];
    if ( (ptr % RCP_DEFAULT_SMF_SIZE) == 0 ) {
      int size;
      size = (1+ptr/RCP_DEFAULT_SMF_SIZE) * RCP_DEFAULT_SMF_SIZE;
      if ( (rcp->track[track].smf = realloc( rcp->track[track].smf,
		    sizeof(unsigned char)*size )) == (unsigned char *)NULL ) {
	/* Memory exhaust */
	return 1;
      }
    }
  } while ( len[i++] >= 0x80 );
  
  i=0;
  while ( rcp->result_smf[i] != SMF_TERM ) {
    rcp->track[track].smf[ptr++] = rcp->result_smf[i++];
    
    if ( (ptr % RCP_DEFAULT_SMF_SIZE) == 0 ) {
      int size;
      size = (1+ptr/RCP_DEFAULT_SMF_SIZE) * RCP_DEFAULT_SMF_SIZE;
      if ( (rcp->track[track].smf = realloc( rcp->track[track].smf,
		    sizeof(unsigned char)*size )) == (unsigned char *)NULL ) {
	/* Memory exhaust */
	return 1;
      }
    }
  }
  rcp->track[track].smf_ptr = ptr;
  rcp->track[track].delta_step = 0;
  
flush_event_end:
  rcp->result_smf[0] = SMF_TERM;

  return 0;
}

static int insert_tempo_event( RCP_DATA *rcp ) {

  TEMPO_EVENT_SEQUENCE *t, *t_last, *t_new;

  t_new = (TEMPO_EVENT_SEQUENCE *)malloc(sizeof(TEMPO_EVENT_SEQUENCE));
  if ( t_new == (TEMPO_EVENT_SEQUENCE *)NULL ) {
    /* Memory exhaust */
    return 1;
  }
  t_new->absolute_step = rcp->step;
  t_new->t1            = rcp->result_smf[3];
  t_new->t2            = rcp->result_smf[4];
  t_new->t3            = rcp->result_smf[5];

  t = TES_top;
  t_last = TES_top;
  while ( t != (TEMPO_EVENT_SEQUENCE *)NULL ) {
    if ( t->absolute_step > t_new->absolute_step ) break;
    
    t_last = t;
    t = t->next;
  }
  t_last->next = t_new;
  t_new->next  = t;
  
  return 0;
}

static int set_tempo_track ( RCP_DATA *rcp ) {

  TEMPO_EVENT_SEQUENCE *t = TES_top, *tn;
  int p;
  int dt,*d;
  int i,j,k;
  unsigned char *data;
  unsigned char buf[1024];

  p = rcp->smf_tempo_track_ptr;
  data = rcp->smf_tempo_track;

  /* Track header */
  /* Title */

  data[p++] = 0;
  data[p++] = MIDI_META;
  data[p++] = META_SEQNAME;
  for ( i=0 ; i<64 ; i++ ) {
    unsigned char c = rcp->title[i];
    if ( c == 0 ) break;
  }
  data[p++] = i;
  for ( j=0 ; j<i ; j++ ) {
    data[p++] = rcp->title[j];
  }

  /* Copyright notice */
  if ( rcp->copyright != NULL ) {
    data[p++] = 0;
    data[p++] = MIDI_META;
    data[p++] = META_COPYRIGHT;
    i=0;
    while ( rcp->copyright[i++] != 0 );
    data[p++] = i;
    for ( j=0 ; j<i ; j++ ) {
      data[p++] = rcp->copyright[j];
    }
  }

  /* converter notice */
  if ( rcp->enable_converter_notice == FLAG_TRUE ) {
    int *len;

    sprintf( buf,"
This file was converted by %s version %s (%s - %s).
Original RCP file is \"%s\" (%s, %d bytes).",
	     (rcp->command_name!=NULL)?rcp->command_name:rcptomid_name,
	     VERSION_ID, VERSION_TEXT1, VERSION_TEXT2,
	     (rcp->file_name!=NULL)?rcp->file_name:undefined_filename,
	     (rcp->date!=NULL)?rcp->date:undefined_date,
	     rcp->length );
    
    i=0;
    while ( buf[i]!=0 ) i++;
    
    data[p++] = 0; /* delta time = 0 */
    data[p++] = MIDI_META;
    data[p++] = META_TEXT;

    len = smf_number_conversion( i );
    j=0;
    do {
      data[p++] = len[j];
    } while ( len[j++] >= 0x80 );

    for ( j=0 ; j<i ; j++ ) {
      data[p++] = buf[j];
    }
  }

  /* key */

  j = rcp->key > 0x0f ? 1:0;
  i = rcp->key%0x10;
  if ( i > 0x07 ) i = (0x100-i)%0x100;

  data[p++] = 0;
  data[p++] = MIDI_META;
  data[p++] = META_KEYSIG;
  data[p++] = 0x02;
  data[p++] = i;
  data[p++] = j;

  /* beat */

#if 0
  data[p++] = 0;
  data[p++] = MIDI_META;
  data[p++] = META_TIMESIG;
  data[p++] = 0x04;
  data[p++] = 0;
  data[p++] = 0;
  data[p++] = 0;
  data[p++] = 0;
#endif

  /* Tempo */

  dt = t->absolute_step;
  while ( t != NULL ) {
    k = p / RCP_DEFAULT_SMF_SIZE;

    d = smf_number_conversion((long)dt);
    i=0;
    do {
      data[p++] = d[i];
    } while ( d[i++] >= 0x80 );

    data[p++] = MIDI_META;
    data[p++] = META_TEMPO;
    data[p++] = 3;
    data[p++] = t->t1;
    data[p++] = t->t2;
    data[p++] = t->t3;
    if ( p > RCP_DEFAULT_SMF_SIZE*(k+1) ) {
      int size;
      size = (1+p/RCP_DEFAULT_SMF_SIZE) * RCP_DEFAULT_SMF_SIZE;
      if ( (rcp->smf_tempo_track = realloc( rcp->smf_tempo_track,
		    sizeof(unsigned char)*size )) == (unsigned char *)NULL ) {
	/* Memory exhaust */
	return 1;
      }
    }

    if ( t->next != NULL )
      dt = t->next->absolute_step - t->absolute_step;
    t = t->next;
  }

  data[p++] = 0;
  data[p++] = MIDI_META;
  data[p++] = META_EOT;
  data[p++] = 0;

  rcp->smf_tempo_track_ptr = p;

  t = TES_top;
  while ( t != NULL ) {
    tn = t;
    t = t->next;
    free(tn);
  }
  
  return 0;
}

static unsigned char *set_smf_data( RCP_DATA *rcp ) {

  long smf_size;
  unsigned char *smf_data;
  int t,tr;
  int ptr;
  int i;
  
  /* Allocate memory for SMF data */

  smf_size = SMF_MTHD_HEADER_SIZE +
    4+rcp->smf_tempo_track_ptr;
  tr=1;
  for (t=0 ; t<rcp->tracks ; t++ ) {
    if ( rcp->track[t].enabled == FLAG_TRUE ) {
      smf_size += 4+rcp->track[t].smf_ptr + 256;
      tr++;
    }
  }
  if ( smf_size > 0xffffffffL ) return NULL;
  smf_data = (unsigned char *)malloc(sizeof(unsigned char *) * smf_size);
  if ( smf_data == (unsigned char *)NULL )
    return NULL;

  /* Set Header Chunk */

  smf_data[0] = 'M';  /* Chunk type */
  smf_data[1] = 'T';
  smf_data[2] = 'h';
  smf_data[3] = 'd';

  smf_data[4] = 0;
  smf_data[5] = 0;
  smf_data[6] = 0;
  smf_data[7] = 6;

  smf_data[8] = 0;    /* Data format ( format 1 ) */
  smf_data[9] = 1;

  smf_data[10] = tr / 256;
  smf_data[11] = tr % 256;

  smf_data[12] = rcp->timebase / 256;
  smf_data[13] = rcp->timebase % 256;

  /* Set first Track Chunk ( tempo track ) */

  ptr = 14;
  smf_data[ptr++] = 'M';
  smf_data[ptr++] = 'T';
  smf_data[ptr++] = 'r';
  smf_data[ptr++] = 'k';

  smf_data[ptr++] = (rcp->smf_tempo_track_ptr>>24)&0xff;
  smf_data[ptr++] = (rcp->smf_tempo_track_ptr>>16)&0xff;
  smf_data[ptr++] = (rcp->smf_tempo_track_ptr>>8 )&0xff;
  smf_data[ptr++] = (rcp->smf_tempo_track_ptr    )&0xff;

  for ( i=0 ; i<rcp->smf_tempo_track_ptr ; i++ ) {
    smf_data[ptr++] = rcp->smf_tempo_track[i];
  }

  /* Set all other Track Chunk ( Music track ) */

  for ( t=0 ; t<rcp->tracks ; t++ ) {
    if ( rcp->track[t].enabled == FLAG_FALSE ) continue;

    smf_data[ptr++] = 'M';
    smf_data[ptr++] = 'T';
    smf_data[ptr++] = 'r';
    smf_data[ptr++] = 'k';
    
    smf_data[ptr++] = (rcp->track[t].smf_ptr>>24)&0xff;
    smf_data[ptr++] = (rcp->track[t].smf_ptr>>16)&0xff;
    smf_data[ptr++] = (rcp->track[t].smf_ptr>>8 )&0xff;
    smf_data[ptr++] = (rcp->track[t].smf_ptr    )&0xff;

    for ( i=0 ; i<rcp->track[t].smf_ptr ; i++ ) {
      smf_data[ptr++] = rcp->track[t].smf[i];
    }
  }

  /* free all smf data */

  free(rcp->smf_tempo_track);

  for ( t=0 ; t<RCP_MAX_TRACKS ; t++ ) {
    /*fprintf(stderr,"%d: %d\n", t, rcp->track[t].smf_ptr);*/
    if ( rcp->track[t].smf != NULL )
      free(rcp->track[t].smf);
  }
  rcp->smf_size = ptr;
  rcp->smf_data = smf_data;

#if 0
  smf_data[4] = (rcp->smf_size / (256*256*256)) % 256; /* Data length */
  smf_data[5] = (rcp->smf_size / (256*256)) % 256;
  smf_data[6] = (rcp->smf_size / 256) % 256;
  smf_data[7] = (rcp->smf_size) % 256;
#endif  

  return smf_data;
}

