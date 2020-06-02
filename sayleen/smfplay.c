/*
  SMF player engine

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.29.2000
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifdef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <signal.h>

#ifdef _POSIX_PRIORITY_SCHEDULING
# include <sched.h>
#endif
#ifdef _POSIX_MEMLOCK
# include <sys/mman.h>
#endif

#include "smfplay.h"
#include "smf.h"
#include "mididev.h"
#include "gettext_wrapper.h"
#include "version.h"

#ifndef timercmp
# define       timercmp(tvp, uvp, cmp)\
               ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
               (tvp)->tv_sec == (uvp)->tv_sec &&\
               (tvp)->tv_usec cmp (uvp)->tv_usec)
#endif

#ifdef HAVE_DEV_RTC
# define SMFPLAY_WAIT_INTERVAL  1 /* (ms) */
#else
# if (HZ>100)
#  define SMFPLAY_WAIT_INTERVAL 2  /* (ms) */
# else
#  define SMFPLAY_WAIT_INTERVAL 10  /* (ms) */
# endif
#endif

/* ------------------------------------------------------------------- */

/* player's functions */

static int set_signals( void );
static int put_event( SMF_DATA *, int );
static int priority_init( void );

static int smf_init_track_buffer( SMF_DATA * );
static int read_smf_header( SMF_DATA * );
static int set_new_event( SMF_DATA *, int );
static long get_smf_number( SMF_DATA *, int );

static int is_player_alive;

inline long timerdiff( struct timeval *st, struct timeval *et ) {
  long reth, retl;
  retl = st->tv_usec - et->tv_usec;
  reth = st->tv_sec - et->tv_sec;
  if ( retl < 0 ) {
    reth--;
    retl+=(1000*1000);
  }
  return (reth*1000*1000)+retl;
}

/* ------------------------------------------------------------------- */

/* SMF player engine */
int smfplay( SMF_DATA *smf ) {

  struct timeval st,et;
  int rtm_delta=0;
  MIDI_DEV dev;

  if ( smf_init_track_buffer( smf ) != 0 ) return 1;
  if ( read_smf_header( smf )   != 0 ) return 1;

  /* prepare for output device (/dev/midi, /dev/ttyS0, etc) */

  dev.is_buffered = smf->is_buffered;
  dev.is_multiport = smf->is_multiport;
  dev.output_device = smf->output_device;
  dev.output_devices[0] = smf->output_devices[0];
  dev.output_devices[1] = smf->output_devices[1];

  if ( open_midi_device( &dev ) ) {
    /* Cannot open midi device */
    fprintf(stderr,_("Cannot open midi device\n"));
    return 1;
  }

  /* prepare for signal */

  set_signals();

  /* prepare for priority ('setuid root' is required) */

  priority_init();

  /* start playing */

  send_midi_reset();
  myusleep( 50 * 1000 );

  /* get first current time */
  gettimeofday( &st, NULL );

  if ( smf->is_send_rtm == FLAG_TRUE ) {
    send_rtm_start();
  }
  smf->rtm_delta = smf->timebase / 24;

  is_player_alive = FLAG_TRUE;
  while( is_player_alive == FLAG_TRUE ) {
    int track;
    int is_all_tracks_finished;

    /* wait a delta time */

    st.tv_usec += smf->tempo_delta;
    while ( st.tv_usec >= 1000*1000 ) {
      st.tv_usec-=1000*1000;
      st.tv_sec++;
    }

    smf->rtm_delta--;
    if ( smf->rtm_delta <= 0 ) {
      if ( smf->is_send_rtm == FLAG_TRUE ) {
	send_rtm_timingclock();
      }
      smf->rtm_delta = smf->timebase / 24;
    }

    {
      long s;
      flush_midi();
      gettimeofday( &et, NULL );
      s = timerdiff( &st, &et );
      if ( s > SMFPLAY_WAIT_INTERVAL*1000 ) {
	myusleep(s);
	gettimeofday( &et, NULL );
      }
    }
    if ( et.tv_sec - st.tv_sec > 1 ) {
      st.tv_sec  = et.tv_sec;
      st.tv_usec = et.tv_usec;
    }

    is_all_tracks_finished = FLAG_TRUE;

    smf->step++;

    for ( track=0 ; track < smf->tracks ; track++ ) {
      /* Have the track finished ? */
      if ( smf->track[track].finished == FLAG_TRUE )
	continue;

      is_all_tracks_finished = FLAG_FALSE;

      /* checks whether the step time is expired */
      smf->track[track].delta_step++;
      smf->track[track].total_step++;
      smf->track[track].step--;

      while ( smf->track[track].step == 0 ) {
	if ( smf->track[track].finished == FLAG_TRUE ) break;
	set_new_event( smf, track );
	put_event( smf, track );
      }
    }
    if ( is_all_tracks_finished == FLAG_TRUE ) break;

  }

  if ( smf->is_send_rtm == FLAG_TRUE ) {
    send_rtm_stop();
  }

#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#endif
#ifdef _POSIX_MEMLOCK
  munlockall();
#endif

  close_midi_device();

  return 0;
}

/* ------------------------------------------------------------------- */

static int put_event( SMF_DATA *smf, int track ) {

  int i;
  static int current_port = -1;

  if ( smf->result[0] == SMF_TERM ) return 0;

  /* is the track disabled ? */
  if ( smf->track[track].enabled == FLAG_FALSE && 
       smf->result[0] != MIDI_NOTEOFF ) {
    goto put_event_end;
  }
  
  /* flushing MIDI data */
  
  if ( current_port != smf->track[track].port ) {
    change_midi_port( smf->track[track].port );
    current_port = smf->track[track].port;
  }

  i=0;
  while ( smf->result[i] != SMF_TERM ) {
    put_midi(smf->result[i++]);
  }

  switch( smf->result[0]&0xf0 ) {
  case 0x80:
    smf->track[track].notes[smf->result[1]]=0;
    break;

  case 0x90:
    smf->track[track].notes[smf->result[1]]=smf->result[2];
    break;

  default:
    break;
  }

  smf->track[track].delta_step = 0;
  
put_event_end:
  smf->result[0] = SMF_TERM;

  return 0;
}

static int set_new_event( SMF_DATA *smf, int t ) {

  int ptr;
  unsigned char *data;
  int ret;

  ret = 0;
  data = smf->data;
  ptr  = smf->track[t].current_ptr;

  if ( ptr >= smf->track[t].top + smf->track[t].size ) {
    smf->track[t].finished = FLAG_TRUE;
    return 1;
  }

  if ( data[ptr+0] < 0x80 )
    smf->result[0] = smf->track[t].last_event;
  else {
    smf->track[t].last_event = data[ptr+0];
    smf->result[0] = data[ptr+0];
    ptr++;
  }

  switch ( smf->result[0]&0xf0 ) {
  case 0x80:
  case 0x90:
  case 0xa0:
  case 0xb0:
  case 0xe0:
    smf->result[1] = data[ptr++];
    smf->result[2] = data[ptr++];
    smf->result[3] = SMF_TERM;
    break;

  case 0xc0:
  case 0xd0:
    smf->result[1] = data[ptr++];
    smf->result[2] = SMF_TERM;
    break;

  default: /* exclusive & meta event */
    switch ( smf->result[0] ) {
      int size;
      int type;
      int i;

    case 0xf0:
      i=0;
      smf->result[i++] = 0xf0;
      size = get_smf_number( smf, ptr );
      while( data[ptr++] >= 0x80 ){};
      while ( size>0 && i < SMF_MAX_RESULT_SMF_SIZE ) {
	smf->result[i++] = data[ptr++];
	size--;
      }
      smf->result[i] = SMF_TERM;
      break;

    case 0xf7:
      i=0;
      size = get_smf_number( smf, ptr );
      while( data[ptr++] >= 0x80 ){};
      while ( size>0 && i < SMF_MAX_RESULT_SMF_SIZE ) {
	smf->result[i++] = data[ptr++];
	size--;
      }
      smf->result[i] = SMF_TERM;
      break;

    case 0xff:
      smf->result[0]=SMF_TERM;
      type = data[ptr++];
      size = get_smf_number( smf, ptr );
      while( data[ptr++] >= 0x80 );

      switch(type) {
      case META_TEMPO:
	smf->tempo = (data[ptr+0]<<16) + (data[ptr+1]<<8) + data[ptr+2];
	smf->tempo_delta = smf->tempo / smf->timebase;
	break;

      case META_PORT:
	smf->track[t].port = data[ptr];
	break;

      case META_EOT:
	smf->track[t].finished = FLAG_TRUE;
	break;

      case META_KEYSIG:
	i = data[ptr];
	if ( i>0x80 ) i = 255-i;
	if ( data[ptr] == 1 ) i+=16;
	smf->key = i;
	break;

      case META_TIMESIG:
	break;

      case META_TEXT:
      case META_COPYRIGHT:
      case META_SEQNAME:
      case META_INSTNAME:
      case META_LYRIC:
      case META_MARKER:
      case META_CUEPT:
	i=0;
	while ( i<size && i<SMF_MAX_MESSAGE_SIZE-1 ) {
	  smf->track[t].message[i] = data[ptr+i];
	  i++;
	}
	smf->track[t].message[i] = '\0';
	break;

      default:
	break;
      }
      ptr+=size;
      break;

    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xf5:
      ptr++;
      break;

    default:
      break;
    }
    break;
  }

  if ( smf->track[t].finished == FLAG_FALSE ) {
    smf->track[t].step = get_smf_number( smf, ptr );
    while( data[ptr++] >= 0x80 );
  }
  else {
    smf->track[t].step = 0;
  }

  smf->track[t].current_ptr = ptr;
  return 0;
}

/* ------------------------------------------------------------------- */

static long get_smf_number( SMF_DATA *smf, int ptr ) {

  long ret;
  unsigned char *p;

  p = smf->data+ptr;

  ret = 0;
  while( *p >= 0x80 ) {
    ret = (ret<<7) + (*p&0x7f);
    p++;
  }
  ret = (ret<<7) + *p;

  return ret;
}

/* ------------------------------------------------------------------- */

static int smf_init_track_buffer( SMF_DATA *smf ) {

  int track,n;

  smf->step            = -1;
  smf->result[0]       = SMF_TERM;
  smf->tempo           = 1000*1000*16;

  for ( track=0 ; track<SMF_MAX_TRACKS ; track++ ) {
    smf->track[track].enabled      = FLAG_FALSE;
    smf->track[track].finished     = FLAG_FALSE;

    smf->track[track].step         = 1;
    smf->track[track].delta_step   = -1;   /* initial value should be -1 */
    smf->track[track].total_step   = -1;

    smf->track[track].current_ptr  = 0;

    smf->track[track].last_event   = 0;
    smf->track[track].port         = 0;

    for ( n=0 ; n<SMF_MAX_NOTES ; n++ )
      smf->track[track].notes[n]   = 0;
    smf->track[track].all_notes_expired = FLAG_TRUE;
  }

  return 0;
}

static int read_smf_header( SMF_DATA *smf ) {

  int t;
  unsigned char *data, *ptr;

  data = smf->data;

  /* check 1st header */

  if ( strncmp( data, SMF_HEADER_STRING, 4 ) !=0 ) {
    if ( strncmp( data+128, SMF_HEADER_STRING, 4 ) !=0 ) { /* Mac binary */
      return 1;
    } else {
      data+=128;
    }
  }

  /* Header chunk */

  smf->format = (data[0x08] << 8) + data[0x09];
  if ( smf->format != 0 && smf->format != 1 )
    return 1; /* Only supports format 0 and 1 */

  smf->tracks    = (data[0x0a] << 8) + data[0x0b];
  smf->timebase  = (data[0x0c] << 8) + data[0x0d];
  smf->rtm_delta = smf->timebase / 24;
  if ( data[0x0c] > 0x7f )
    return 1; /* Not supports SMPTE format */

  ptr = data+8+ (data[0x04]<<24) + (data[0x05]<<16) + (data[0x06]<<8) + data[0x07];

  /* Track chunk */

  for ( t=0 ; t<smf->tracks ; t++ ) {
    long size;
    unsigned char *p;

    if ( ptr+8 > smf->data + smf->length )
      break;

    if ( strncmp( ptr, SMF_TRACK_STRING, 4 ) !=0 ) {
      return 1;
    }

    smf->track[t].enabled    = FLAG_TRUE;

    size = (ptr[0x04]<<24) + (ptr[0x05]<<16) + (ptr[0x06]<<8) + ptr[0x07];
    smf->track[t].top = (ptr+8 - smf->data);
    smf->track[t].size = size;

    smf->track[t].step = get_smf_number( smf, smf->track[t].top )+1;
    p = ptr+8;
    while( *(p++) >= 0x80 );
    smf->track[t].current_ptr = (long)(p-smf->data);

    ptr = ptr+8+size;
  }

  return 0;
}

/* ------------------------------------------------------------------- */

/* Signals configuration */

static RETSIGTYPE sigexit( int num ) {
  send_rtm_stop();
  send_midi_reset();

#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#endif
#ifdef _POSIX_MEMLOCK
  munlockall();
#endif

  fprintf(stderr,"Signal caught : %d\n", num);
  exit(1);
}

static RETSIGTYPE sig_stop_play( int num ) {
  send_midi_reset();
  send_rtm_stop();

  signal( SIGINT, SIG_DFL );
  is_player_alive = FLAG_FALSE;

  return;
}

static const int signals[]={SIGHUP,SIGQUIT,SIGILL,SIGABRT,SIGFPE,
                              SIGBUS,SIGSEGV,SIGPIPE,SIGTERM,0};

static int set_signals( void ) {

  int i;

  for ( i=0 ; signals[i]!=0 ; i++ )
    signal( signals[i], sigexit );

  signal( SIGINT, sig_stop_play );

  return 0;
}

/* priority configuration : only when program is setuid root-ed */

static int priority_init( void ) {

#ifdef _POSIX_PRIORITY_SCHEDULING
  struct sched_param ptmp, *priority_param;
  int i;

  priority_param=&ptmp;
  i=sched_get_priority_max( SCHED_FIFO );
  priority_param->sched_priority = i/2; /* no means */
  sched_setscheduler( 0, SCHED_FIFO, priority_param );
#endif
#ifdef _POSIX_MEMLOCK
  mlockall(MCL_CURRENT);
#endif

  return 0;
}
