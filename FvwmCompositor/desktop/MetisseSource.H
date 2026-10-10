#ifndef MetisseSource_H_
#define MetisseSource_H_

/*
 *
 * desktop/MetisseSource.H --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <nucleo/core/ReactiveObject.H>
#include <nucleo/image/source/ImageSource.H>
#include <nucleo/network/tcp/TcpConnection.H>
#include <nucleo/core/URI.H>

#include <stdint.h>

#include "libmetisse/proto.h"

using namespace nucleo;

class MetisseDesktop;



class MetisseSource : public ReactiveObject {

 private:

	std::string _hostname ;
	int _port ;
	std::string _password ;

	Image::Encoding _encoding ;
	int _depth;

	TcpConnection *_conn ;
	std::string _buffer ;

	MetisseDesktop *_metisseDesktop;

	unsigned int _width;
	unsigned int _height;

	bool _useShm;

	void _receive(char *data, unsigned int length);
	bool _ReadFromRFBServer(char *out, unsigned int n);
	bool _framebufferUpdate(rfbServerToClientMsg msg);
	bool _readMETISSEServer(void) ;

 protected:

	void react(Observable*);

 public:

  MetisseSource(MetisseDesktop *MetisseDesktop, const char *encoding, const URI& uri,
			bool use_shm);
	~MetisseSource(void) { stop() ; }

 	bool start(void) ;
	bool isActive(void) { return (_conn!=0) ; }
	bool stop(void) ;

	void updateRequest(bool include=true) ;
	void updateRequest(int x, int y, int w, int h, bool incremental=true);
	void windowUpdateRequest(uint32_t win, int x, int y, int w, int h, bool incremental);
	void windowUpdateRequest(uint32_t win);
	void keyEvent(unsigned long keysym, bool down_flag=true) ;
	void pointerEvent(uint32_t win, int x, int y, unsigned long button_mask=0) ;
	void pointerEvent(int x, int y, unsigned long button_mask=0) ;
	void getSize(unsigned int *width, unsigned int *height);

} ;

#endif
