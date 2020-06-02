/*
  RCP player engine

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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
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

#include "rcp.h"
#include "smf.h"
#include "rcp_functions.h"
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
# define RCPPLAY_WAIT_INTERVAL   1 /* (ms) */
#else
# if (HZ>100)
#  define RCPPLAY_WAIT_INTERVAL  2  /* (ms) */
# else
#  define RCPPLAY_WAIT_INTERVAL  10  /* (ms) */
# endif
#endif

/* ------------------------------------------------------------------- */

#ifdef HAVE_STED2_SUPPORT
#include "rcddef.h"
static struct RCD_HEAD *rcd;
static int is_sted2_support = FLAG_FALSE;
void (*sted2_md_put)( char );
#endif /* HAVE_STED2_SUPPORT */

/* ------------------------------------------------------------------- */
/* player's functions */

static int set_signals( void );
static int put_event( RCP_DATA *, int );
static int priority_init( void );

#ifdef HAVE_STED2_SUPPORT
static int sted2_init( RCP_DATA * );
static int sted2_close( void );
#endif /* HAVE_STED2_SUPPORT */

extern int rcptomid_init_track_buffer( RCP_DATA * );
extern int rcptomid_read_rcp_header( RCP_DATA * );
extern int rcptomid_set_new_event( RCP_DATA *, int );

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
/* RCP player engine */
int rcpplay( RCP_DATA *rcp ) {

  struct timeval st,et;
  MIDI_DEV dev;
  
  if ( rcptomid_init_track_buffer( rcp ) != 0 ) return 1;
  if ( rcptomid_read_rcp_header( rcp )   != 0 ) return 1;

#ifdef HAVE_STED2_SUPPORT
  /* prepare for STed2 support */

  sted2_init( rcp );
#endif /* HAVE_STED2_SUPPORT */

  /* prepare for output device (/dev/midi, /dev/ttyS0, etc) */

  dev.is_buffered = rcp->is_buffered;
  dev.is_multiport = rcp->is_multiport;
  dev.output_device = rcp->output_device;
  dev.output_devices[0] = rcp->output_devices[0];
  dev.output_devices[1] = rcp->output_devices[1];

  if ( open_midi_device( &dev ) ) {
#ifdef HAVE_STED2_SUPPORT
    if ( is_sted2_support == FLAG_FALSE ) 
#endif /* HAVE_STED2_SUPPORT */
      {
	/* Cannot open midi device */
	fprintf(stderr,_("Cannot open midi device\n"));
	return 1;
      }
  }
#ifdef HAVE_STED2_SUPPORT
  else {
    sted2_md_put = NULL;
  }
#endif /* HAVE_STED2_SUPPORT */

  /* prepare for signal */

  set_signals();

  /* prepare for priority ('setuid root' is required) */

  priority_init();

  /* start playing */

  send_midi_reset();
  myusleep( 50 * 1000 );

  /* get first current time */
  gettimeofday( &st, NULL );

#ifdef HAVE_STED2_SUPPORT
  if ( is_sted2_support == FLAG_TRUE ) {
    int trk;
    rcd->stepcount = 0;
    for ( trk=0 ; trk < TRK_NUM ; trk++ ) {
      rcd->step[trk]=-1;
      rcd->bar[trk]=-1;
    }
  }
#endif /* HAVE_STED2_SUPPORT */

  if ( rcp->is_send_rtm == FLAG_TRUE ) {
    send_rtm_start();
  }
  rcp->rtm_delta = rcp->timebase / 24;

  is_player_alive = FLAG_TRUE;
  while( is_player_alive == FLAG_TRUE ) {
    int track;
    int is_all_tracks_finished;

    /* wait a delta time */

    st.tv_usec += 1000 * 1000 * 60 / (rcp->realtempo * rcp->timebase);
    while ( st.tv_usec >= 1000*1000 ) {
      st.tv_usec-=1000*1000;
      st.tv_sec++;
    }

    rcp->rtm_delta--;
    if ( rcp->rtm_delta <= 0 ) {
      if ( rcp->is_send_rtm == FLAG_TRUE ) {
	send_rtm_timingclock();
      }
      rcp->rtm_delta = rcp->timebase / 24;
    }

    {
      long s;
      flush_midi();
      gettimeofday( &et, NULL );
      s = timerdiff( &st, &et );
      if ( s > RCPPLAY_WAIT_INTERVAL*1000 ) {
	myusleep(s);
	gettimeofday( &et, NULL );
      }
    }
    if ( et.tv_sec - st.tv_sec > 1 ) {
      st.tv_sec  = et.tv_sec;
      st.tv_usec = et.tv_usec;
    }

    is_all_tracks_finished = FLAG_TRUE;

    rcp->step++;

#ifdef HAVE_STED2_SUPPORT
    if ( is_sted2_support == FLAG_TRUE ) {
      int trk;
      rcd->stepcount++;
      for ( trk=0 ; trk < TRK_NUM ; trk++ ) {
	rcd->step[trk]=-1;
	rcd->bar[trk]=-1;
      }
    }
#endif /* HAVE_STED_SUPPORT */

    for ( track=0 ; track < rcp->tracks ; track++ ) {
      int n;

      /* Have the track finished ? */
      if ( rcp->track[track].finished == FLAG_TRUE &&
	   rcp->track[track].all_notes_expired == FLAG_TRUE )
	continue;

      is_all_tracks_finished = FLAG_FALSE;

      /* checks each notes whether the gate time is expired */
      if ( rcp->track[track].all_notes_expired == FLAG_FALSE ) {
	for ( n = rcp->track[track].notes_min ;
	      n <=rcp->track[track].notes_max ; n++ ) {
	  int gate = rcp->track[track].notes[n];
	  gate--;
	  if ( gate == 0 ) {
	    rcp_note_off( rcp, track, n );
	    put_event( rcp, track );
	  }
	  if ( gate <  0 ) gate = 0;
	  rcp->track[track].notes[n] = gate;
	}
      }

      /* checks whether the step time is expired */
      rcp->track[track].delta_step++;
      rcp->track[track].total_step++;
      rcp->track[track].step--;
      while ( rcp->track[track].step == 0 ) {
	if ( rcp->track[track].finished == FLAG_TRUE ) break;
	rcptomid_set_new_event( rcp, track );
	put_event( rcp, track );
      }
    }
    if ( is_all_tracks_finished == FLAG_TRUE ) break;
  }

  if ( rcp->is_send_rtm == FLAG_TRUE ) {
    send_rtm_stop();
  }

#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#endif
#ifdef _POSIX_MEMLOCK
  munlockall();
#endif

#ifdef HAVE_STED2_SUPPORT
  sted2_close();
#endif /* HAVE_STED2_SUPPORT */

  close_midi_device();

  return 0;
}

/* ------------------------------------------------------------------- */

RETSIGTYPE sigexit( int num ) {
#ifdef HAVE_STED2_SUPPORT
  sted2_close();
#endif /* HAVE_STED2_SUPPORT */

#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#endif
#ifdef _POSIX_MEMLOCK
  munlockall();
#endif

  send_rtm_stop();
  send_midi_reset();
  close_midi_device();

  fprintf(stderr,"Signal caught : %d\n", num);
  exit(1);
}

RETSIGTYPE sig_stop_play( int num ) {
#ifdef HAVE_STED2_SUPPORT
  sted2_close();
#endif /* HAVE_STED2_SUPPORT */

  send_rtm_stop();
  send_midi_reset();

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

/* ------------------------------------------------------------------- */
#ifdef HAVE_STED2_SUPPORT
static int sted2_init ( RCP_DATA *rcp ) {

  char *p;
  int shmid;

  is_sted2_support = FLAG_FALSE;
  sted2_md_put = NULL;

  p = getenv("STED_RCD_SHMID");
  if ( p == NULL ) return FLAG_FALSE;

  shmid = atoi(p);
  rcd = (struct RCD_HEAD *)shmat( shmid, 0, 0 );
  if ( rcd == NULL ) return FLAG_FALSE;

  is_sted2_support = FLAG_TRUE;
  /*sted2_md_put = rcd->md_put;*/

  return FLAG_TRUE;
}

static int sted2_close( void ) {

  return 0;
}
#endif /* HAVE_STED2_SUPPORT */

/* ------------------------------------------------------------------- */
static int put_event( RCP_DATA *rcp, int track ) {

  int i;
  static int current_port = -1;

  if ( rcp->result_smf[0] == SMF_TERM ) return 0;

  /* is the track disabled ? */
  if ( rcp->track[track].enabled == FLAG_FALSE && 
       rcp->result_smf[0] != MIDI_NOTEOFF ) {
    goto put_event_end;
  }
  
  if ( rcp->result_smf[0] == MIDI_META ) {
    switch( rcp->result_smf[1] ) {
    case META_PORT:
      break;

    case META_TEXT:
    case META_SEQNUM:
    case META_COPYRIGHT:
    case META_SEQNAME:
    case META_INSTNAME:
    case META_LYRIC:
      if ( rcp->enable_verbose == FLAG_TRUE ) {
	i=2;
	while( rcp->result_smf[i++] >= 0x80 ) {};
	while( rcp->result_smf[i] != SMF_TERM ) {
	  fprintf(stderr,"%c",rcp->result_smf[i++]);
	}
	fprintf(stderr,"\n");
      }
      break;

    default:
      break;
    }
    goto put_event_end;
  }

  /* flushing MIDI data */
  
  if ( rcp->track[track].delta_step < 0 ) {
    fprintf(stderr,"%d\n",(int)rcp->track[track].delta_step);
  }

  if ( current_port != rcp->track[track].port ) {
    change_midi_port( rcp->track[track].port );
    current_port = rcp->track[track].port;
  }

  i=0;
  if ( rcp->result_smf[0] == 0xf0 || rcp->result_smf[0] == 0xf7 ) {
    put_midi(rcp->result_smf[0]);
    i=2;
  }
  while ( rcp->result_smf[i] != SMF_TERM ) {
    put_midi(rcp->result_smf[i]);
    i++;
  }

  rcp->track[track].delta_step = 0;
  
put_event_end:
  rcp->result_smf[0] = SMF_TERM;

  return 0;
}
