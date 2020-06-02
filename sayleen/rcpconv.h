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

#ifndef _RCPCONV_H_
#define _RCPCONV_H_

extern long rcpconv( unsigned char *,  /* Pointer to RCP data */
		     long,             /* Length (bytes) of RCP data */
		     unsigned char **, /* Destination of conversion */
		     unsigned char *   /* Copyright notice */
		     );

extern long rcpconv_with_notice(
		     unsigned char *,  /* Pointer to RCP data */
		     long,             /* Length (bytes) of RCP data */
		     unsigned char **, /* Destination of conversion */
		     unsigned char *,  /* Copyright notice */
		     unsigned char *,  /* Converter's command name */
		     unsigned char *,  /* Filename of original RCP file */
		     unsigned char *   /* Timestamp of original RCP file */
		     );

#endif / _RCPCONV_H_ */
