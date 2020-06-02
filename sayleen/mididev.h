/*
  MIDI device access routines

  Copyright 2000 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Dec.23.1999
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

#ifndef _MIDIDEV_H_
#define _MIDIDEV_H_

struct _MIDI_DEV {
  int is_buffered;
  int is_multiport;

  unsigned char *output_device;
  unsigned char *output_devices[2];
};
typedef struct _MIDI_DEV MIDI_DEV;

extern int open_midi_device( MIDI_DEV *in_dev );
extern int close_midi_device( void );
extern int change_midi_port ( int );
extern int put_midi( int );
extern int flush_midi ( void );
extern int send_midi_reset( void );
extern int send_rtm_start( void );
extern int send_rtm_stop( void );
extern int send_rtm_continue( void );
extern int send_rtm_timingclock( void );
extern void myusleep( long );

#endif /* _MIDIDEV_H_ */
