/*
  SMF utility functions

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Mar.14.1999
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

#include "smf.h"

int *smf_number_conversion( long num ) {

  /* Max number of result data sequents are limitted to 
     4 bytes. */

  static int smf_number[4];
  int i,j;
  int is_exist_upper_bytes;

  if ( num < 0 || num >= 128*128*128*128) {
    /* invalid smf number */
    return NULL;
  }

  for ( i=0 ; i<4 ; i++ ) {
    smf_number[i]=0;
  }
  i=0;
  is_exist_upper_bytes = 0;

  j = (num / (128*128*128)) % 128;
  if ( j > 0 ) {
    smf_number[i++] = j + 0x80;
    is_exist_upper_bytes = 1;
  }
  j = (num / (128*128)) % 128;
  if ( j > 0 || is_exist_upper_bytes == 1 ) {
    smf_number[i++] = j + 0x80;
    is_exist_upper_bytes = 1;
  }
  j = (num / 128) % 128;
  if ( j > 0 || is_exist_upper_bytes == 1 ) {
    smf_number[i++] = j + 0x80;
  }
  j = num % 128;
  smf_number[i] = j;

  return smf_number;
}

