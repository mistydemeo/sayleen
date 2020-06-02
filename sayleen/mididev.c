/*
  MIDI device handling codes

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Dec.18.1999
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

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <termios.h>
#include <sys/stat.h>

#ifdef HAVE_DEV_RTC
# include <linux/rtc.h>
# include <sys/ioctl.h>
#endif

#include "rcp.h"
#include "mididev.h"

static void start_rtc_timer(void);
static void stop_rtc_timer(void);

/* ------------------------------------------------------------------- */

#define SERIAL_SPEED B38400

/* ------------------------------------------------------------------- */

extern int reset_mode;

/* MIDI reset messages */

static const unsigned char gs_reset[] = {
  0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 
  0x7f, 0x00, 0x41, 0xf7,
  0xff }; /* GS Reset */

static const unsigned char gm_reset[] = {
  0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7,
  0xff };

static const unsigned char S8_reset[] = {
  0xf0, 0x41, 0x10, 0x42, 0x12, 0x00, 0x00,
  0x7f, 0x00, 0x01, 0xf7,
  0xff }; /* Single module mode */

static const unsigned char XG_reset[] = {
  0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00,
  0x7e, 0x00, 0xf7,
  0xff };

static const unsigned char all_reset[] = {
  0xb0, 0x7b, 0x00, 0xb0, 0x78, 0x00, 0xb0, 0x79, 0x00,
  0xb1, 0x7b, 0x00, 0xb1, 0x78, 0x00, 0xb1, 0x79, 0x00,
  0xb2, 0x7b, 0x00, 0xb2, 0x78, 0x00, 0xb2, 0x79, 0x00,
  0xb3, 0x7b, 0x00, 0xb3, 0x78, 0x00, 0xb3, 0x79, 0x00,
  0xb4, 0x7b, 0x00, 0xb4, 0x78, 0x00, 0xb4, 0x79, 0x00,
  0xb5, 0x7b, 0x00, 0xb5, 0x78, 0x00, 0xb5, 0x79, 0x00,
  0xb6, 0x7b, 0x00, 0xb6, 0x78, 0x00, 0xb6, 0x79, 0x00,
  0xb7, 0x7b, 0x00, 0xb7, 0x78, 0x00, 0xb7, 0x79, 0x00,
  0xb8, 0x7b, 0x00, 0xb8, 0x78, 0x00, 0xb8, 0x79, 0x00,
  0xb9, 0x7b, 0x00, 0xb9, 0x78, 0x00, 0xb9, 0x79, 0x00,
  0xba, 0x7b, 0x00, 0xba, 0x78, 0x00, 0xba, 0x79, 0x00,
  0xbb, 0x7b, 0x00, 0xbb, 0x78, 0x00, 0xbb, 0x79, 0x00,
  0xbc, 0x7b, 0x00, 0xbc, 0x78, 0x00, 0xbc, 0x79, 0x00,
  0xbd, 0x7b, 0x00, 0xbd, 0x78, 0x00, 0xbd, 0x79, 0x00,
  0xbe, 0x7b, 0x00, 0xbe, 0x78, 0x00, 0xbe, 0x79, 0x00,
  0xbf, 0x7b, 0x00, 0xbf, 0x78, 0x00, 0xbf, 0x79, 0x00,
  0xff };

static const unsigned char* const reset_exclusives[]={
  gm_reset, gs_reset, S8_reset, XG_reset
};

/* ------------------------------------------------------------------- */

static int           devs[2] = {-1, -1};
static int           block_write = FLAG_TRUE;
static unsigned char midibuf[1024];
static int           midibuf_ptr = 0;
static int           current_port = 0;
static int           multiport = 0;

/* ------------------------------------------------------------------- */

static int
open_device(unsigned char* in_name, int* inout_dev)
{
  struct termios tio;
  struct stat stt;
  int dev;
  
  if ( !inout_dev ) return FLAG_FALSE;
  if ( in_name == (unsigned char *)NULL ) return FLAG_FALSE;

  if ( stat( in_name, &stt ) != 0 ) return FLAG_FALSE;
  if ( ! (S_ISCHR(stt.st_mode) ||
	  S_ISFIFO(stt.st_mode) ||
	  S_ISSOCK(stt.st_mode)) ) {
    return FLAG_FALSE;
  }

  dev = open(in_name, O_RDWR);
  if (dev < 0) {
    return FLAG_FALSE;
  }

  *inout_dev = dev;
  if ( tcgetattr(dev, &tio) < 0 ) {
    return FLAG_TRUE; /* the device might be generic MIDI device , fifo or pipe */
  }

  /* Now we change the speed of device */

  tio.c_iflag = 0;
  tio.c_oflag = 0;
  tio.c_cflag = CS8 | CREAD | CLOCAL;
  tio.c_lflag = 0;
  cfsetispeed(&tio, SERIAL_SPEED);
  cfsetospeed(&tio, SERIAL_SPEED);
  tcsetattr(dev, TCSANOW, &tio);

  return FLAG_TRUE;
}

int open_midi_device( MIDI_DEV* in_dev ) {

  unsigned char* name;
  int is_block_write;

  if (!in_dev) return 1;

  block_write = in_dev->is_buffered;
  multiport = in_dev->is_multiport;
  midibuf_ptr = 0;


  if (!multiport) {
    if (open_device(in_dev->output_device, &devs[0])==FLAG_FALSE)
      return 1;
    devs[1] = -1;
    current_port = 0;
  } else { // multiport
    if (open_device(in_dev->output_devices[0], &devs[0])==FLAG_FALSE)
      return 1;
    if (open_device(in_dev->output_devices[1], &devs[1])==FLAG_FALSE) {
      close(devs[0]);
      devs[0] = -1;
      return 1;
    }
  }

#ifdef HAVE_DEV_RTC
  start_rtc_timer();
#endif

  return 0;
}

int close_midi_device( void ) {
  int i;

#ifdef HAVE_DEV_RTC
  stop_rtc_timer();
#endif

  for (i=0; i<2; i++) {
    if (devs[i]>=0) {
      close(devs[i]);
      devs[i] = -1;
    }
  }
  return 0;
}

int put_midi( int data ) {

  unsigned char d;

  if ( data < 0 || data > 255 ) { return 1; }

  if ( midibuf_ptr >= 1023 ) {
    flush_midi();
  }
  midibuf[midibuf_ptr++] = (unsigned char)data;

  return 0;
}

int flush_midi( void ) {

  int dev;
  int id;

  id = current_port;
  if (id<0 || id>=2) id = 0;
  dev = devs[id];
  if (dev<0) dev = devs[0];
  if (dev<0) return -1;

  if ( midibuf_ptr > 0 ) {
    if ( block_write == FLAG_TRUE ) {
      write( dev, midibuf, midibuf_ptr );
    }
    else {
      int i;
      for ( i=0 ; i<midibuf_ptr ; i++ ) {
	write( dev, midibuf+i, 1 );
      }
    }
    midibuf_ptr = 0;
  }

  return 0;
}

int change_midi_port( int port ) {

  if (!multiport) {
    put_midi( 0xf5 );
    put_midi( port+1 );
  } else {
    if (current_port!=port)
      flush_midi();
    current_port = port;
  }

  return 0;
}

int send_midi_reset( void ) {

  const unsigned char *p;
  int port;

  if ( reset_mode >= sizeof(reset_exclusives) ) {
    return 1;
  }

  flush_midi();
  for ( port=0 ; port<1 ; port++ ) {

    /* reset all notes */
    p = all_reset;
    while ( *p != 0xff ) {
      put_midi( (int)*p++ );
    }
    flush_midi();
    myusleep(60000);

    /* send reset message */
    p = reset_exclusives[reset_mode];
    while ( *p != 0xff ) {
      put_midi( (int)*p++ );
    }
    flush_midi();
    myusleep(60000);
  }

  return 0;
}

/* ------------------------------------------------------------------- */

int send_rtm_start( void ) {
  put_midi( 0xfa );
  return 0;
}

int send_rtm_stop( void ) {
  put_midi( 0xfc );
  return 0;
}

int send_rtm_continue( void ) {
  put_midi( 0xfb );
  return 0;
}

int send_rtm_timingclock( void ) {
  put_midi( 0xf8 );
  return 0;
}

/* TIMER */


#ifdef HAVE_DEV_RTC
static int devrtc_fd=-1;
static unsigned long lasthz;
#define SAYLEEN_RTC_CLOCK 1024
void start_rtc_timer( void ) {
  devrtc_fd = open("/dev/rtc", O_RDONLY);
  if (devrtc_fd<0) return;

  ioctl(devrtc_fd, RTC_IRQP_READ, &lasthz);
  if (ioctl(devrtc_fd, RTC_IRQP_SET, SAYLEEN_RTC_CLOCK)<0) {
    goto error_end;
  }
  if (ioctl(devrtc_fd, RTC_PIE_ON, 0)<0) {
    goto error_end;
  }
  return;

 error_end:
  devrtc_fd=-1;
  lasthz=1024;
  return;
}

void stop_rtc_timer( void ) {
  if (devrtc_fd<0) return;

  ioctl(devrtc_fd, RTC_PIE_OFF, 0);
  ioctl(devrtc_fd, RTC_IRQP_SET, lasthz);
  devrtc_fd=-1;
}
#endif

void myusleep( long usec ) {

#ifdef HAVE_DEV_RTC
  unsigned long data;
  int ret;
  unsigned long hz;
  static int watchdog=0;
  long hz_usec;

  if (devrtc_fd < 0) {
    start_rtc_timer();
    if (devrtc_fd<0) {
      goto error_end;
    }
  }

  if (watchdog--<=0) {
    ioctl(devrtc_fd, RTC_PIE_ON, 0);
    ioctl(devrtc_fd, RTC_IRQP_READ, &hz);
    if (hz!=SAYLEEN_RTC_CLOCK) {
      ioctl(devrtc_fd, RTC_IRQP_SET, SAYLEEN_RTC_CLOCK);
    }
    watchdog=100;
  }
  hz_usec=(1000*1000)/SAYLEEN_RTC_CLOCK;
  while(usec>hz_usec) {
    ret=read(devrtc_fd, &data, sizeof(unsigned long));
    if (ret==-1) {
      goto error_end;
    }
    usec-=hz_usec;
  }

  return;


 error_end:
# ifdef HAVE_USLEEP
  usleep(usec);
# else
  {
    struct timeval st;
    st.tv_sec = 0;
    st.tv_usec = usec;
    select(0, NULL, NULL, NULL, &st);
  }
# endif
  return;
#else

# ifdef HAVE_NANOSLEEP
  /* nanosleep(2) */
  struct timespec t;
  t.tv_sec  = (time_t)usec/1000000;
  t.tv_nsec = (long)(usec%1000000)*1000;
  nanosleep( &t, NULL );
  return;
# else

#  ifdef HAVE_USLEEP
  /* usleep(2) */
  usleep(usec);
  return;

#  else

  /* select(2) */
  struct timeval st;
  st.tv_sec = 0;
  st.tv_usec = usec;
  select(0, NULL, NULL, NULL, &st);
  return;
#  endif
# endif
#endif
}
