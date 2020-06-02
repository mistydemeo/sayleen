/*
  RCP converter

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  May.28.1999
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

#include <stdio.h>
#include <stdlib.h>

#include "rcp.h"
#include "rcpconv.h"

/* ------------------------------------------------------------------- */

long rcpconv( unsigned char *data, long length, unsigned char **smf, 
	      unsigned char *copyright ) {

  RCP_DATA rcp;
  long smf_length;

  rcp.length       = length;
  rcp.data         = data;
  rcp.date         = NULL;  /* timestamp text (ctime) of original RCP file */
  rcp.file_name    = NULL;  /* filename of original RCP file */
#ifdef RCPCONV_COMMAND_NAME
  rcp.command_name = RCPCONV_COMMAND_NAME;
#else
  rcp.command_name = NULL;
#endif
  rcp.copyright    = copyright;

  rcp.enable_converter_notice = FLAG_FALSE;
  rcp.enable_verbose          = FLAG_FALSE;

  *smf = rcptomid( &rcp );
  if ( *smf == NULL ) smf_length = -10;
  else smf_length = rcp.smf_size;

  return smf_length;
}

long rcpconv_with_notice( unsigned char *data, long length, unsigned char **smf, 
			  unsigned char *copyright,
			  unsigned char *command_name,
			  unsigned char *file_name,
			  unsigned char *date ) {

  RCP_DATA rcp;
  long smf_length;

  rcp.length       = length;
  rcp.data         = data;
  rcp.date         = date;
  rcp.file_name    = file_name;
  rcp.command_name = command_name;
  rcp.copyright    = copyright;

  rcp.enable_converter_notice = FLAG_TRUE;
  rcp.enable_verbose          = FLAG_FALSE;

  *smf = rcptomid( &rcp );
  if ( *smf == NULL ) smf_length = -10;
  else smf_length = rcp.smf_size;

  return smf_length;
}
