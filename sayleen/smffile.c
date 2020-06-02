/*
  SMF file loader & checker

  Copyright 1999 by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
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

#include <sys/stat.h>

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

#include "smfplay.h"

SMF_DATA *smf_read_file( char *name ) {

  FILE *fp;
  struct stat stt;
  SMF_DATA *smf;
  unsigned char *n;
  int s;

  /* data read */

  smf = (SMF_DATA *)malloc(sizeof(SMF_DATA));
  if ( smf == NULL ) return NULL;

  if ( stat( name, &stt ) ) return NULL;
  fp = fopen( name, "r" );
  if ( fp == NULL ) return NULL;

  smf->length = stt.st_size;
  smf->data = (unsigned char *)malloc(sizeof(unsigned char)*smf->length);
  fread( smf->data, 1, smf->length, fp );

  n = ctime(&(stt.st_mtime));
  s = strlen(n)+4;
  smf->date = (unsigned char *)malloc(sizeof(unsigned char)*s);
  strcpy( smf->date, n );
  n = strrchr( smf->date, '\n' );
  if ( n != NULL ) *n = '\0';

  fclose(fp);

  n = strrchr( name, '/' );
  if ( n == NULL ) n=name-1;
  n++;
  s = strlen(n)+4;
  smf->file_name = (unsigned char *)malloc(sizeof(unsigned char)*s);
  strcpy( smf->file_name, n );

  return smf;
}

int smf_close( SMF_DATA *smf ) {

  if ( smf == NULL ) return 0;
  if ( smf->date != NULL ) free(smf->date);    /* time stamp */
  if ( smf->data != NULL ) free(smf->data);    /* SMF data */
  if ( smf->file_name != NULL ) free(smf->file_name);

  free(smf);
  smf=NULL;

  return 0;
}
