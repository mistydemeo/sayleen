/*
  RCP converter / player

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

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif

#ifndef USE_NETBSDGETOPT
# include <getopt.h>
#else
# include "netbsdgetopt.h"
#endif

#include "rcp.h"
#include "gettext_wrapper.h"
#include "version.h"

/* ------------------------------------------------------------------- */

#define PATH_BUF_SIZE 1024
#define PLAYER_NAME "rcpplay"

/* ------------------------------------------------------------------- */

void error_end( char * );
static int option_gets( int, char ** );
static void usage( void );
static void display_version( void );
static char *command_name=NULL;
static int verbose=FLAG_FALSE;
static unsigned char *copyright=NULL;
static unsigned char *outfile=NULL;
static unsigned char *outdev=NULL;
static unsigned char *devs[2];
static int is_player    = FLAG_FALSE;
static int is_send_rtm  = FLAG_FALSE;
static int is_buffered  = FLAG_TRUE;

int reset_mode=0;

/* ------------------------------------------------------------------- */

extern int rcpplay( RCP_DATA *);

/* ------------------------------------------------------------------- */
int main( int argc, char **argv ) {

  int num;
  char *a, buf[PATH_BUF_SIZE];
  int isonefile;

  RCP_DATA *rcp;
  unsigned char *smf;

#ifdef ENABLE_NLS
  setlocale( LC_ALL, "" );
  bindtextdomain( PACKAGE, LOCALEDIR );
  textdomain( PACKAGE );
#endif /* ENABLE_NLS */

  num = option_gets( argc, argv );
  if ( num == argc-1 ) isonefile=FLAG_TRUE;
  else isonefile=FLAG_FALSE;

  /* main loop */

  while ( num < argc ) {
    char *name;
    name = argv[num++];

    if ( isonefile==FLAG_FALSE ) {
      if ( (a=strrchr( name, '.' ))==NULL ) continue; /* no extension */

      if ( strcasecmp( a, ".rcp" )!=0 ) {
	if ( strcasecmp( a, ".r36" )!=0 ) continue; /* extension not RCP/R36 */
      }
    }

    rcp = rcp_read_file( argv[num-1] );
    if ( rcp==NULL ) {
      snprintf( buf, PATH_BUF_SIZE, _("Cannot open file %s.\n"), argv[num-1] );
      error_end(buf);
    }

    rcp->command_name   = command_name;
    rcp->enable_verbose = verbose;
    rcp->is_player      = is_player;
    rcp->is_send_rtm    = is_send_rtm;
    rcp->is_buffered    = is_buffered;

    if ( is_player == FLAG_FALSE ) {
      rcp->copyright               = copyright;
      rcp->enable_converter_notice = FLAG_TRUE;

      if ( verbose == FLAG_TRUE ) {
	fprintf( stderr, _("Filename = %s\n"), argv[num-1] );
	if ( outfile != NULL )
	  fprintf(stderr, _("Output filename = %s\n"), outfile );
	if ( rcp->copyright != NULL )
	  fprintf( stderr, _("Copyright message = %s\n"), rcp->copyright );
      }

      smf=rcptomid( rcp );

      if ( smf == NULL ) {
	fprintf(stderr,_("%s: Cannot convert %s\n"), command_name, argv[num-1]);
      }
      else {
	if ( outfile == NULL )
	  fwrite( smf, 1, rcp->smf_size, stdout );
	else {
	  FILE *fp;
	  fp = fopen(outfile,"w");
	  if ( fp != NULL ) {
	    fwrite( smf, 1, rcp->smf_size, fp );
	    fclose(fp);
	  } else
	    fprintf(stderr,_("%s: Cannot open file %s\n"), command_name, outfile);
	}
	free(smf);
      }
    }
    else {

      if ( verbose == FLAG_TRUE ) {
	fprintf( stderr, _("Filename = %s\n"), argv[num-1] );
      }

      rcp->output_devices[0] = devs[0];
      rcp->output_devices[1] = devs[1];
      
      rcp->is_multiport = FLAG_FALSE;
      if (devs[0]&&devs[1]) {
	rcp->is_multiport   = FLAG_TRUE;
      } else if (!outdev && (devs[0] || devs[1])) {
	if (devs[0]) outdev = devs[0];
	else outdev = devs[1];
      }

      if ( outdev != NULL )
	rcp->output_device = outdev;
      else
	rcp->output_device = DEFAULT_OUTPUT_DEVICE;

      rcpplay( rcp );
      rcp_close( rcp );
    }
  }

  /* finished */

  exit(0);
}

/* ------------------------------------------------------------------- */

void error_end( char *msg ) {

  fprintf( stderr, "%s: %s\n", command_name, msg );
  exit(1);
}

/* ------------------------------------------------------------------- */

static int option_gets( int argc, char **argv ) {

  extern char *optarg;
  extern int optind;

  int c;
  int option_index=0;

  command_name =
    (strrchr(argv[0],'/')==NULL)?argv[0]:(strrchr(argv[0],'/')+1);

  if ( strcasecmp( command_name, PLAYER_NAME ) == 0 ) {
    is_player = FLAG_TRUE;
  }

  verbose = FLAG_FALSE;
  is_send_rtm = FLAG_FALSE;
  is_buffered = FLAG_TRUE;
  devs[0] = NULL;
  devs[1] = NULL;

  while(1) {
    static struct option long_options[] = {
      {"no-buffered",   0, 0, 'b'},
      {"send-seq",      0, 0, 300},
      {"outdev",        1, 0, 'm'},
      {"dev0",          1, 0, 301},
      {"dev1",          1, 0, 302},
      {"reset-mode",    1, 0, 'r'},
      {"outfile",       1, 0, 'o'},
      {"version",       0, 0, 'V'},
      {"verbose",       0, 0, 'v'},
      {"help",          0, 0, 'h'},
      {"copyright",     1, 0, 'c'},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "Vvbo:m:r:h", long_options, &option_index );
    if ( c == EOF ) break;

    switch(c) {

    case 300:
      is_send_rtm = FLAG_TRUE;
      break;

    case 'b':
      is_buffered = FLAG_FALSE;
      break;

    case 'm':
      if ( outdev != NULL ) free(outdev);
      outdev =  (unsigned char *)malloc(sizeof(unsigned char)*strlen(optarg)+16);
      strcpy( outdev, optarg );
      break;

    case 301:
      if (devs[0]) free(devs[0]);
      devs[0] =  (unsigned char *)malloc(sizeof(unsigned char)*strlen(optarg)+16);
      strcpy( devs[0], optarg );
      break;

    case 302:
      if (devs[1]) free(devs[1]);
      devs[1] =  (unsigned char *)malloc(sizeof(unsigned char)*strlen(optarg)+16);
      strcpy( devs[1], optarg );
      break;

    case 'r':
      reset_mode = atoi(optarg);
      if ( reset_mode < 0 ) reset_mode = 0;
      if ( reset_mode > 3 ) reset_mode = 0;
      break;


    case 'h': /* help */
      usage();
      break;

    case 'V':
      display_version(); /* version */
      break;

    case 'v':
      verbose = FLAG_TRUE;
      break;

    case 'c':
      if ( copyright != NULL ) free(copyright);
      copyright = (unsigned char *)malloc(sizeof(unsigned char)*strlen(optarg)+16);
      strcpy( copyright, optarg );
      break;

    case 'o':
      if ( outfile != NULL ) free(outfile);
      outfile =  (unsigned char *)malloc(sizeof(unsigned char)*strlen(optarg)+16);
      strcpy( outfile, optarg );
      break;

    case '?':
      break;
    default:
      break;
    }
  }

  if ( optind >= argc ) {
    fprintf(stderr, _("%s: No input filename is specified.\n"), command_name);
    exit(1);
  } 

  return optind;
}

static void usage( void ) {

  fprintf(stderr, "usage: %s [options] [rcp-filename]\n", command_name );
  fprintf(stderr, _("Options:\n"));

  if ( is_player == FLAG_TRUE ) {
    fprintf(stderr, " -m,     --outdev <devname>   ");
    fprintf(stderr, _("Output device name.\n"));
    fprintf(stderr, "         --dev0 <devname>     ");
    fprintf(stderr, _("Output device name 0 (multi-port).\n"));
    fprintf(stderr, "         --dev1 <devname>     ");
    fprintf(stderr, _("Output device name 1 (multi-port).\n"));
    fprintf(stderr, " -r,     --reset-mode <val>   ");
    fprintf(stderr, _("Send specified reset message after playing. \n"));
    fprintf(stderr, "                               0:GM 1:GS 2:SC88 3:XG\n");
  }

  if ( is_player == FLAG_FALSE ) {
    fprintf(stderr, " -o,     --outfile <filename> ");
    fprintf(stderr, _("Output to file as <filename>.\n"));
    fprintf(stderr, "         --copyright <msg>    ");
    fprintf(stderr, _("Set copyright meta event as <msg>.\n"));
  }

  fprintf(stderr, " -v,     --verbose            ");
  fprintf(stderr, _("Be verbose.\n"));
  fprintf(stderr, " -V,     --version            ");
  fprintf(stderr, _("Show version information.\n"));
  fprintf(stderr, " -h,     --help               ");
  fprintf(stderr, _("Show this help message.\n"));

  exit(0);
}

static void display_version( void ) {

  fprintf(stderr, "%s version ", command_name );
  fprintf(stderr, VERSION_ID "\n");
  if ( is_player == FLAG_TRUE ) {
    fprintf(stderr, "tiny RCP player");
#ifdef HAVE_STED2_SUPPORT
    fprintf(stderr, " <STed2 support>");
#endif
    fprintf(stderr,"\n");
  }
  else
    fprintf(stderr, "RCP to SMF converter\n");
  fprintf(stderr, "Copyright 1999 by NAGANO Daisuke <breeze.nagano@nifty.ne.jp>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "This is free software; see the source for copying conditions.\n");
  fprintf(stderr, "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n");
  fprintf(stderr, "PARTICULAR PURPOSE.\n\n");

  exit(0);
}
