/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/* 
 * metisseauth.h - describes the functions provided by the wncauth library.
 */

#ifndef METISSEAUTH_H
#define METISSEAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPWLEN 8
#define CHALLENGESIZE 16

int metisseEncryptAndStorePasswd(char *passwd, char *fname);
char *metisseDecryptPasswdFromFile(char *fname);
int metisseDecryptPasswdFromFile2(char *fname, char *passwdFullControl, char *passwdViewOnly);
void metisseRandomBytes(unsigned char *bytes);
void metisseEncryptBytes(unsigned char *bytes, char *passwd);

#ifdef __cplusplus
}
#endif

#endif
