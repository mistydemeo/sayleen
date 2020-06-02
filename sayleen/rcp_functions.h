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

#ifndef _RCP_FUNCTIONS_H_
#define _RCP_FUNCTIONS_H_

extern int rcp_note_on( RCP_DATA *, int );
extern int rcp_note_off( RCP_DATA *, int, int );
extern int rcp_user_exclusive( RCP_DATA *, int );
extern int rcp_ch_exclusive( RCP_DATA *, int );
extern int rcp_exec_extern_prog( RCP_DATA *, int );
extern int rcp_bank_and_prog( RCP_DATA *, int );
extern int rcp_key_scan( RCP_DATA *, int );
extern int rcp_midi_ch_change( RCP_DATA *, int );
extern int rcp_tempo_change( RCP_DATA *, int );
extern int rcp_after_touch( RCP_DATA *, int );
extern int rcp_control_change( RCP_DATA *, int );
extern int rcp_program_change( RCP_DATA *, int );
extern int rcp_after_touch_poly( RCP_DATA *, int );
extern int rcp_pitch_bend( RCP_DATA *, int );
extern int rcp_yamaha_base( RCP_DATA *, int );
extern int rcp_yamaha_dev_name( RCP_DATA *, int );
extern int rcp_yamaha_addr( RCP_DATA *, int );
extern int rcp_yamaha_xg_ad( RCP_DATA *, int );
extern int rcp_roland_base( RCP_DATA *, int );
extern int rcp_roland_para( RCP_DATA *, int );
extern int rcp_roland_dev( RCP_DATA *, int );
extern int rcp_key_change( RCP_DATA *, int );
extern int rcp_comment_start( RCP_DATA *, int );
extern int rcp_loop_end( RCP_DATA *, int );
extern int rcp_loop_start( RCP_DATA *, int );
extern int rcp_same_measure( RCP_DATA *, int );
extern int rcp_measure_end( RCP_DATA *, int );
extern int rcp_end_of_track( RCP_DATA *, int );
extern int rcp_dx7_function( RCP_DATA *, int );
extern int rcp_dx_parameter( RCP_DATA *, int );
extern int rcp_dx_rerf( RCP_DATA *, int );
extern int rcp_tx_function( RCP_DATA *, int );
extern int rcp_fb01_parameter( RCP_DATA *, int );
extern int rcp_fb01_system( RCP_DATA *, int );
extern int rcp_tx81z_vced( RCP_DATA *, int );
extern int rcp_tx81z_aced( RCP_DATA *, int );
extern int rcp_tx81z_pced( RCP_DATA *, int );
extern int rcp_tx81z_system( RCP_DATA *, int );
extern int rcp_tx81z_effect( RCP_DATA *, int );
extern int rcp_dx7_2_remote_sw( RCP_DATA *, int );
extern int rcp_dx7_2_aced( RCP_DATA *, int );
extern int rcp_dx7_2_pced( RCP_DATA *, int );
extern int rcp_tx802_pced( RCP_DATA *, int );
extern int rcp_mks_7( RCP_DATA *, int );

#endif /* _RCP_FUNCTIONS_H_ */
