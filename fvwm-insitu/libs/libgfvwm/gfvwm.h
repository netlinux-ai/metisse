#ifndef __GFVWM_H__
#define __GFVWM_H__

#include <glib.h>

#include "../Module.h"

typedef void (*GfvwmNewPacketCallback) (FvwmPacket *packet);

void gfvwm_init (int *argc, char ***argv, const char *name);
void gfvwm_send_command (const gchar *command);
void gfvwm_register_callback (GfvwmNewPacketCallback cb);


#endif
