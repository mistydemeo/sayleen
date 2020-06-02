/*
  tiny SMF player

  Copyright 2000 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Feb.03.2000
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

#ifndef _SMFPLAY_H_
#define _SMFPLAY_H_

#define FLAG_TRUE  1
#define FLAG_FALSE 0

#define SMF_MAX_TRACKS            128
#define SMF_MAX_NOTES             128

#define SMF_MAX_RESULT_SMF_SIZE  1024
#define SMF_MAX_MESSAGE_SIZE     1024

#define ENABLE_PORT_CHANGE

/* structs */

typedef struct _SMF_TRACK {
  int           top;                        /* base pointer */
  int           size;                       /* data size */
  int           midi_ch;                    /* midi channel */
  int           port;                       /* midi port (0 or 1 ) */

  /* track work */

  int           enabled;
  int           finished;

  int           current_ptr;
  long          delta_step;
  long          total_step;

  int           event;
  long          step;
  int           vel;

  int           last_event;

  int           notes[SMF_MAX_NOTES];
  int           all_notes_expired;

  unsigned char message[SMF_MAX_MESSAGE_SIZE+10];

} SMF_TRACK;

typedef struct _SMF_DATA {

  unsigned char *data;        /* data */
  size_t         length;      /* data length */
  unsigned char *file_name;   /* original SMF/G36 filename */
  unsigned char *date;        /* original SMF/R36 timestamp */
  unsigned char *command_name;/* command name ( typically "SMFtomid" ) */

  unsigned char  title[65];   /* data title ( perhaps SJIS ) */

  int format;                 /* SMF format */
  int timebase;               /* timebase */
  long tempo;                 /* tempo */
  long tempo_delta;           /* tempo / timebase */
  long rtm_delta;             /* tempo / 24 */
  int beat_h;                 /* beat */
  int beat_l;                 /* beat */

  int key;                    /* key */
  int play_bias;              /* play bias */

  int tracks;                 /* track number (SMF:18 R36:26) */

  long step;                  /* total step */

                              /* user exclusive */

                              /* track work area */

  SMF_TRACK track[SMF_MAX_TRACKS];

  int result[SMF_MAX_RESULT_SMF_SIZE+10];

  int            enable_converter_notice;
  int            enable_verbose;

  /* player's informations */

  unsigned char *output_device;
  unsigned char *output_devices[2];
  int            is_player;
  int            is_send_rtm;
  int            is_buffered;
  int            is_multiport;

} SMF_DATA;

extern void           error_end( char *);
extern SMF_DATA      *smf_read_file( char * );
extern int            smf_close( SMF_DATA * );

#endif /* _SMFPLAY_H_ */
