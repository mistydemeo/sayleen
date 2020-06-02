/*
  gettext_wrapper.h  gettext wrapper

  Made by Daisuke Nagano <breeze_nagano@nifty.ne.jp>
  Mar.29.1998 

 */

#ifndef _GETTEXT_WRAPPER_H_
#define _GETTEXT_WRAPPER_H_

#ifdef ENABLE_NLS
# include <locale.h>
# include <libintl.h>
# undef _
# define _(string) gettext(string)
# define N_(String) (String)

#else /* ENABLE_NLS */
# define _(string) string
# define N_(String) (String)
#endif /* ENABLE_NLS */

#endif /* _GETTEXT_WRAPPER_H_ */

