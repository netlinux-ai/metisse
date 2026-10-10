/* -*-c-*- */
/* Copyright (C) 2005  Olivier Chapuis */

#ifndef _METISSE_
#define _METISSE_

void metisse_handle_module_input(void);
void metisse_check_pointer_pos(exec_context_t *pexc, int *x, int *y);
Bool metisse_is_unmap_forbided(FvwmWindow *fw);

#endif
