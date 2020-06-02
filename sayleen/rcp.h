/*
  RCP converter

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

#ifndef _RCP_H_
#define _RCP_H_

#define FLAG_TRUE  1
#define FLAG_FALSE 0

#define RCP_HEADER_STRING        "RCM-PC98V2.0(C)COME ON MUSIC"
#define STEDDATA_HEADER_STRING   "STEDDATAx.xxDedicatedToTURBO"

#define RCP_MAX_TRACKS           36
#define RCP_MAX_USER_EXCLUSIVE   8
#define RCP_MAX_COMMENT_SIZE     36
#define RCP_MAX_NOTES            128
#define RCP_MAX_LOOPS            128

#define RCP_MAX_RESULT_SMF_SIZE  256
#define RCP_USER_EXCLUSIVE_SIZE  24

#define RCP_DEFAULT_SMF_SIZE     65536

#define ENABLE_PORT_CHANGE

/* structs */

typedef struct _LOOP_DATA {
  int top_ptr;
  int loop_count;
  int same_measure_flag;
  int same_measure_ptr;

} LOOP_DATA;

typedef struct _RCP_TRACK {
  int           top;                        /* base pointer */
  int           size;                       /* data size */
  int           midi_ch;                    /* midi channel */
  int           port;                       /* midi port (0 to 15 ) */
  int           key;                        /* key offset */
  int           st;                         /* step offset */
  int           mode;                       /* mode */
                                            /* comment */
  unsigned char comment[RCP_MAX_COMMENT_SIZE];

  /* track work */

  int           enabled;
  int           finished;

  int           current_ptr;
  long          delta_step;
  long          total_step;

  int           event;
  int           step;
  int           gate;
  int           vel;

  int           notes[RCP_MAX_NOTES];
  int           notes_min;
  int           notes_max;
  int           all_notes_expired;

  int           yamaha_base[2];
  int           roland_base[2];

  int           loop_depth;
  LOOP_DATA     loop[RCP_MAX_LOOPS];

  int           same_measure_flag;
  int           same_measure_ptr;

  unsigned char *smf;
  int           smf_ptr;

} RCP_TRACK;

typedef struct _RCP_DATA {

  unsigned char *data;        /* data */
  size_t         length;      /* data length */
  unsigned char *file_name;   /* original RCP/G36 filename */
  unsigned char *date;        /* original RCP/R36 timestamp */
  unsigned char *command_name;/* command name ( typically "rcptomid" ) */
  unsigned char *copyright;   /* copyright notice */

  int g36;                    /* flag for G18/G36 format */
  int rcp;                    /* flag for RCP/R36 format */
  int steddata;               /* flag for STED3 format */

  unsigned char  title[65];   /* data title ( perhaps SJIS ) */
  unsigned char  memo[337];   /* memo */

  int timebase;               /* timebase */
  int tempo;                  /* tempo */
  int realtempo;              /* tempo (Beat per minute) */
  int rtm_delta;              /* timebase / 24 */
  int beat_h;                 /* beat */
  int beat_l;                 /* beat */

  int key;                    /* key */
  int play_bias;              /* play bias */

  int tracks;                 /* track number (RCP:18 R36:26) */

  long step;                  /* total step */

                              /* user exclusive */
  int user_exclusive[RCP_MAX_USER_EXCLUSIVE][24];
  int yamaha_dev_id;
  int yamaha_model_id;
  int roland_dev_id;
  int roland_model_id;

                              /* track work area */

  RCP_TRACK track[RCP_MAX_TRACKS];

  int result_smf[RCP_MAX_RESULT_SMF_SIZE];

  unsigned char *smf_tempo_track;
  int            smf_tempo_track_ptr;

  unsigned char *smf_data;
  int            smf_size;

  int            enable_converter_notice;
  int            enable_verbose;

  /* player's informations */

  char          *output_device;
  int            is_player;
  int            is_send_rtm;
  int            is_buffered;
  int            is_multiport;
  char          *output_devices[2];

} RCP_DATA;

extern void           error_end( char *);
extern RCP_DATA      *rcp_read_file( char * );
extern int            rcp_close( RCP_DATA * );

extern unsigned char *rcptomid( RCP_DATA * );


/* RCP data structure definition */

#define RCP_USER_EXCLUSIVE_1         0x90
#define RCP_USER_EXCLUSIVE_2         0x91
#define RCP_USER_EXCLUSIVE_3         0x92
#define RCP_USER_EXCLUSIVE_4         0x93
#define RCP_USER_EXCLUSIVE_5         0x94
#define RCP_USER_EXCLUSIVE_6         0x95
#define RCP_USER_EXCLUSIVE_7         0x96
#define RCP_USER_EXCLUSIVE_8         0x97
#define RCP_CH_EXCLUSIVE             0x98
#define RCP_EXEC_EXTERN_PROG         0x99
#define RCP_BANK_AND_PROG            0xe2
#define RCP_KEY_SCAN                 0xe5
#define RCP_MIDI_CH_CHANGE           0xe6
#define RCP_TEMPO_CHANGE             0xe7
#define RCP_AFTER_TOUCH              0xea
#define RCP_CONTROL_CHANGE           0xeb
#define RCP_PROGRAM_CHANGE           0xec
#define RCP_AFTER_TOUCH_POLY         0xed
#define RCP_PITCH_BEND               0xee
#define RCP_YAMAHA_BASE              0xd0
#define RCP_YAMAHA_DEV_NUM           0xd1
#define RCP_YAMAHA_ADDR              0xd2
#define RCP_YAMAHA_XG_AD             0xd3
#define RCP_ROLAND_BASE              0xdd
#define RCP_ROLAND_PARA              0xde
#define RCP_ROLAND_DEV               0xdf
#define RCP_KEY_CHANGE               0xf5
#define RCP_COMMENT_START            0xf6
#define RCP_LOOP_END                 0xf8
#define RCP_LOOP_START               0xf9
#define RCP_SAME_MEASURE             0xfc
#define RCP_MEASURE_END              0xfd
#define RCP_END_OF_TRACK             0xfe
#define RCP_DX7_FUNCTION             0xc0
#define RCP_DX_PARAMETER             0xc1
#define RCP_DX_RERF                  0xc2
#define RCP_TX_FUNCTION              0xc3
#define RCP_FB01_PARAMETER           0xc5
#define RCP_FB01_SYSTEM              0xc6
#define RCP_TX81Z_VCED               0xc7
#define RCP_TX81Z_ACED               0xc8
#define RCP_TX81Z_PCED               0xc9
#define RCP_TX81Z_SYSTEM             0xca
#define RCP_TX81Z_EFFECT             0xcb
#define RCP_DX7_2_REMOTE_SW          0xcc
#define RCP_DX7_2_ACED               0xcd
#define RCP_DX7_2_PCED               0xce
#define RCP_TX802_PCED               0xcf
#define RCP_MKS_7                    0xdc

#define RCP_2ND_EVENT                0xf7

#endif /* _RCP_H_ */
