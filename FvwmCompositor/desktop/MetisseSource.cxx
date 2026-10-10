/*
 *
 * desktop/MetisseSource.cxx --
 *
 * Copyright (C) Nicolas Roussel, Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include "MetisseSource.H"

#include <nucleo/core/ReactiveEngine.H>
#include <nucleo/image/encoding/Conversion.H>
#include <nucleo/gl/window/keysym.H>
#include <nucleo/utils/ByteOrder.H>
#include <nucleo/utils/FileUtils.H>

#include "MetisseDesktop.H"
#include "MetisseWindow.H"

#include "libmetisse/proto.h"
#include "libmetisse/auth.h"

#include <X11/Xmd.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <stdio.h>
#include <math.h>

#include <stdexcept>

/*
 * NOTE: METISSE supports bitsPerPixel of 8, 16 and 32 only (not 24).
 * at 32 (24) RGB is in fact raw RGBA and at 16 it is RGB565
 *
 * The alpha is broken in any case. 
 */

/*
 * When leaving a window, should we generate keyRelease events for
 * pressed modifiers (VNC client does so) ?
 *
 * Yes we do that
 *
 */

#define DEBUG_LEVEL 1

#if DEBUG_LEVEL>=1
static void
PrintPixelFormat(rfbPixelFormat *format) {
  if (format->bitsPerPixel == 1) {
    fprintf(stderr,"  Single bit per pixel.\n");
    fprintf(stderr,
		  "  %s significant bit in each byte is leftmost on the screen.\n",
		  (format->bigEndian ? "Most" : "Least"));
  } else {
    fprintf(stderr,"  %d bits per pixel.\n",format->bitsPerPixel);
    if (format->bitsPerPixel != 8) {
      fprintf(stderr,"  %s significant byte first in each pixel.\n",
		    (format->bigEndian ? "Most" : "Least"));
    }
    if (format->trueColour) {
      fprintf(stderr,"  True colour: max red %d green %d blue %d",
		    format->redMax, format->greenMax, format->blueMax);
      fprintf(stderr,", shift red %d green %d blue %d\n",
		    format->redShift, format->greenShift, format->blueShift);
    } else {
      fprintf(stderr,"  Colour map (not true colour).\n");
    }
  }
}
#endif


#define METISSE_TIME_DEBUG 0

#define METISSE_TIME_DEBUG_FRAME_ONLY 0

#define METISSE_TIME_DEBUG_START      0
#define METISSE_TIME_DEBUG_FBU_START  1
#define METISSE_TIME_DEBUG_FBU_STOP   2
#define METISSE_TIME_DEBUG_FRAME_START  3
#define METISSE_TIME_DEBUG_FRAME_STOP   4
#define METISSE_TIME_DEBUG_BITS   5

#if METISSE_TIME_DEBUG
#include <sys/time.h>

static 
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
       /* Perform the carry for the later subtraction by updating Y. */
       if (x->tv_usec < y->tv_usec) {
         int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
         y->tv_usec -= 1000000 * nsec;
         y->tv_sec += nsec;
       }
       if (x->tv_usec - y->tv_usec > 1000000) {
         int nsec = (x->tv_usec - y->tv_usec) / 1000000;
         y->tv_usec += 1000000 * nsec;
         y->tv_sec -= nsec;
       }
     
       /* Compute the time remaining to wait.
          `tv_usec' is certainly positive. */
       result->tv_sec = x->tv_sec - y->tv_sec;
       result->tv_usec = x->tv_usec - y->tv_usec;
     
       /* Return 1 if result is negative. */
       return x->tv_sec < y->tv_sec;
}
#endif


static void
bitsPerSeconds(int bits, int type, bool shm)
{
#if METISSE_TIME_DEBUG
	static unsigned long totalBits = 0;
	static unsigned long shmBits = 0;
	static unsigned long rectangles = 0;
	static unsigned long updates = 0;
	static unsigned long frames = 0;
	static struct timeval time;
	struct timeval currentTime;

	if (type == METISSE_TIME_DEBUG_START)
	{
		gettimeofday(&time, NULL);
		return;
	}
	else if (type == METISSE_TIME_DEBUG_FBU_STOP)
	{
		updates++;
		return;
	}
	else if (type == METISSE_TIME_DEBUG_BITS)
	{
		rectangles++;
		totalBits += bits;
		if (shm) shmBits += bits;
	}

	if (type != METISSE_TIME_DEBUG_FRAME_STOP)
	{
		return;
	}

	frames++;
	gettimeofday(&currentTime, NULL);

	if (currentTime.tv_sec - time.tv_sec >= 5)
	{
		timeval diff;
		float tmicro, tsec, tmilli;
		
		timeval_subtract(&diff, &currentTime, &time);

		tmicro = diff.tv_sec * 1000000 + diff.tv_usec;
		tmilli = tmicro / 1000.0;
		tsec = tmicro / 1000000.0;

#if METISSE_TIME_DEBUG_FRAME_ONLY
		std::cerr
			<< "--- " << "time: " << tsec << " seconds  "
			<< " real updates/sec: " << (float)(frames)/tsec
			<< std::endl;
#else
		std::cerr
			<< "--- " << "time: " << tsec << " seconds" << std::endl
			<< "bits: " << totalBits
			<< " (shm: " << ((shmBits)?1:0)
			<< ") rectangles: " << rectangles
			<< " updates: " << updates 
			<< " real updates: " << frames 
			  << std::endl
			  << "bits/millisecond: "
			  << (float)totalBits/tmilli
			  << " real updates/sec: "
			  << (float)(frames)/tsec
			  << " rectangles/frame: "
			  << (float)rectangles/(float)frames
			  << std::endl;
#endif		
		gettimeofday(&time, NULL);
		frames = totalBits = rectangles = shmBits = updates = 0;
		
	}
#endif

}

// ----------------------------------------------------------------------
#define BUF_SIZE 8192
static char buf[BUF_SIZE];
static char *bufoutptr = buf;
static int buffered = 0;

bool
MetisseSource::_ReadFromRFBServer(char *out, unsigned int n)
{
	int rfbsock = _conn->getFd() ;

	if (n <= buffered)
	{
		memcpy(out, bufoutptr, n);
		bufoutptr += n;
		buffered -= n;
		return True;
	}

	memcpy(out, bufoutptr, buffered);

	out += buffered;
	n -= buffered;

	bufoutptr = buf;
	buffered = 0;

	if (n <= BUF_SIZE)
	{

		while (buffered < n)
		{
			int i = read(
				rfbsock, buf + buffered, BUF_SIZE - buffered);
			if (i <= 0)
			{
				if (i < 0)
				{
					if (errno == EWOULDBLOCK ||
					    errno == EAGAIN) {
						std::cerr <<
							"Should process event?"
							  << std::endl;
						i = 0;
					}
					else
					{
						perror(": read");
						return False;
					}
				}
				else
				{
					if (1)
					{
						std::cerr <<
							"METISSE server closed " <<
							"connection"
							  << std::endl;
					}
					return False;
				}
			}
			buffered += i;
		}

		memcpy(out, bufoutptr, n);
		bufoutptr += n;
		buffered -= n;
		return True;
	}
	else
	{
		
		while (n > 0)
		{
			int i = read(rfbsock, out, n);
			if (i <= 0)
			{
				if (i < 0)
				{
					if (errno == EWOULDBLOCK ||
					    errno == EAGAIN) {
						std::cerr <<
							"Should process event?"
							  << std::endl;
						i = 0;
					}
					else
					{
						perror(": read");
						return False;
					}
				}
				else
				{
					if (1)
					{
						std::cerr << "METISSE server closed "
							  << "connection"
							  << std::endl;
					}
					return False;
				}
			}
			out += i;
			n -= i;
		}

		return True;
	}
}

void
MetisseSource::_receive(char *data, unsigned int length) {
	int bytestoread = length-_buffer.length() ;
	if (bytestoread>0)
	{
		int fd = _conn->getFd() ;
		int buffersize = bytestoread ;
		if (buffersize<64000) buffersize = 64000 ;
		char *tmpbuffer = new char [buffersize] ;
		int bytesread = 0 ;
		while (bytesread<bytestoread)
		{
			int newbytes = read(
				fd, (char *)(tmpbuffer+bytesread),
				buffersize-bytesread) ;
			if (newbytes==-1)
			{
				throw std::runtime_error(
					"MetisseSource: Connection closed");
			}
			bytesread += newbytes ;

#if 0
			std::cerr << newbytes << " new bytes (" << bytestoread
				  << ", " << bytestoread-bytesread << " to go)"
				  << std::endl ;
#endif

		}
		_buffer.append((char *)tmpbuffer, bytesread) ;
		delete [] tmpbuffer ;
	}

#if 0
	std::cerr << "Reading " << length << " bytes from buffer" << std::endl ;
#endif

	memcpy(data, _buffer.c_str(), length) ;
	_buffer.erase(0, length) ;
}

#define METISSE_BUFFERED_READS 1

#if METISSE_BUFFERED_READS
//#define RECEIVE(a,b) _ReadFromRFBServer(a,b); VERY OLD STUFF
#define RECEIVE(a,b) _receive(a,b);
#else
#define RECEIVE(buffer,size) _conn->receive(buffer, size,true)
#endif

// ------------------------------------------------------------------

MetisseSource::MetisseSource(MetisseDesktop *MetisseDesktop,  const char *encodingname,
					    const URI& uri, bool use_shm)
{
	_metisseDesktop = MetisseDesktop;
	_hostname = uri.host ;
	_port = 5900 + uri.port;
	std::string query = uri.query ;
	if (!URI::getQueryArg(query, "password", &_password)) _password = "" ;
	_conn = NULL ;
	_useShm = use_shm;
	_depth = 0;
	_encoding = Image::getEncodingByName(encodingname) ;
}

bool
MetisseSource::start(void)
{
	try {
		_conn = new TcpConnection(_hostname.c_str(), _port) ;

		int one=1, socket=_conn->getFd() ;
		setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (void *)&one, sizeof(int)) ;
#ifdef TCP_FASTACK
		setsockopt(socket, IPPROTO_TCP, TCP_FASTACK, (void *)&one, sizeof(int)) ;
#endif

		// --------------------------------------------------------
		// Greetings

		rfbProtocolVersionMsg pv;
		RECEIVE(pv, sz_rfbProtocolVersionMsg) ;
		pv[sz_rfbProtocolVersionMsg] = 0 ;

		int major,minor;
		if (sscanf(pv,rfbProtocolVersionFormat,&major,&minor) != 2)
		{
			throw std::runtime_error(
				"MetisseSource: not a valid Metisse server") ;
		}

#if DEBUG_LEVEL>=1
		std::cerr << "MetisseSource: METISSE server supports protocol "
			"version " << major << "." << minor ;
		std::cerr << " (viewer " << rfbProtocolMajorVersion << "."
			  << rfbProtocolMinorVersion << ")" << std::endl ;
#endif

		major = rfbProtocolMajorVersion ;
		minor = rfbProtocolMinorVersion ;
		sprintf(pv,rfbProtocolVersionFormat,major,minor) ;
		_conn->send(pv, sz_rfbProtocolVersionMsg) ;

		// -----------------------------------------------------------
		// Authentication

		CARD32 authScheme ;
		RECEIVE((char *)&authScheme, 4) ;
		authScheme = ByteOrder::swap32ifle(authScheme);

		switch (authScheme)
		{
		case rfbConnFailed:
		{
			CARD32 reasonLen ;
			RECEIVE((char *)&reasonLen, 4) ;
			reasonLen = ByteOrder::swap32ifle(reasonLen) ;
			char *reason = new char [reasonLen] ;
			RECEIVE(reason, reasonLen) ;
			std::string msg = "MetisseSource: METISSE connection "
				"failed (" ;
			msg.append(reason, reasonLen) ;
			msg.append(")") ;
			throw std::runtime_error(msg) ;
		}
		break ;

		case rfbNoAuth:
#if DEBUG_LEVEL>=1
			std::cerr << "MetisseSource: No authentication needed"
				  << std::endl ;
#endif
			break;

		case rfbMetisseAuth:
		{
			CARD8 challenge[CHALLENGESIZE];
			RECEIVE((char *)challenge, CHALLENGESIZE) ;
			if (_password!="")
			{
				metisseEncryptBytes(challenge, (char *)_password.c_str()) ;
			}
			else
			{
				char *passwd ;
				do
				{
					passwd = getpass("METISSE Password: ");
				}
				while ((!passwd) || (strlen(passwd) == 0)) ;
				if (strlen(passwd) > 8)
				{
					passwd[8] = '\0';
				}
				metisseEncryptBytes(challenge, passwd);
				// "Forget" the clear-text password
				for (int i = strlen(passwd); i >= 0; i--)
				{
					passwd[i] = '\0';
				}
			}

			_conn->send((char *)challenge, CHALLENGESIZE) ;

			CARD32 authResult;
			RECEIVE((char *)&authResult, 4) ;
			authResult = ByteOrder::swap32ifle(authResult);

			switch (authResult)
			{
			case rfbMetisseAuthOK:
#if DEBUG_LEVEL>=1
				std::cerr << "MetisseSource: METISSE "
					  << "authentication "
					  << "succeeded" << std::endl ;
#endif
				break;
			case rfbMetisseAuthFailed:
				throw std::runtime_error(
					"MetisseSource: METISSE authentication "
					"failed") ;
			case rfbMetisseAuthTooMany:
				throw std::runtime_error(
					"MetisseSource: METISSE authentication "
					"failed - too many tries");
			default:
				throw std::runtime_error(
					"MetisseSource: Unknown METISSE "
					"authentication result") ;
			}
		}
		break;

		default:
			throw std::runtime_error(
				"Unknown authentication scheme from METISSE server");
		} /* switch (authScheme) */

		// -----------------------------------------------------------

		rfbClientInitMsg ci;
		ci.shared = 1 ;
		_conn->send((char *)&ci, sz_rfbClientInitMsg) ;

		rfbServerInitMsg si;
		RECEIVE((char *)&si, sz_rfbServerInitMsg) ;

		si.framebufferWidth = ByteOrder::swap16ifle(si.framebufferWidth);
		si.framebufferHeight = ByteOrder::swap16ifle(si.framebufferHeight);
		si.format.redMax = ByteOrder::swap16ifle(si.format.redMax);
		si.format.greenMax = ByteOrder::swap16ifle(si.format.greenMax);
		si.format.blueMax = ByteOrder::swap16ifle(si.format.blueMax);
		si.nameLength = ByteOrder::swap32ifle(si.nameLength);

		_width = si.framebufferWidth;
		_height = si.framebufferHeight;

		char *desktopName = new char [si.nameLength + 1] ;
		RECEIVE(desktopName, si.nameLength) ;

		desktopName[si.nameLength] = 0;
		_metisseDesktop->desktopName = desktopName ;

#if DEBUG_LEVEL>=1
		fprintf(stderr,"Desktop name \"%s\" (%ix%i)\n",desktopName,
			_width, _height);
		fprintf(stderr,"Connected to METISSE server, using protocol "
			"version %d.%d\n",
			rfbProtocolMajorVersion, rfbProtocolMinorVersion);
		fprintf(stderr,"METISSE server default format:\n");
		PrintPixelFormat(&si.format);
#endif

		delete [] desktopName;

		if (_encoding==Image::PREFERRED) {
		  switch (si.format.bitsPerPixel) {
		  case 16: _encoding = Image::RGB565 ; _depth = 16 ; break ;
		  default: _encoding = Image::RGBA ; _depth = 24 ; break ;
		  }
		}

		if (_encoding!=Image::RGB565 && _encoding!=Image::RGBA && _encoding!=Image::ARGB) {
		  std::cerr << "Unsupported encoding (" << Image::getEncodingName(_encoding) << "), switching to RGBA" << std::endl ;
		  _encoding = Image::RGBA ; 
		  _depth = 24 ;
		}
	 
		// --- Pixel format ------------------------------------------

  rfbSetPixelFormatMsg spf;
  spf.type = rfbSetPixelFormat;
  spf.format.bigEndian = ByteOrder::isLittleEndian() ? 0 : 1 ;
  spf.format.trueColour = 1 ;
  if (_encoding==Image::RGB565) {
    spf.format.bitsPerPixel =  16;
    spf.format.depth = 16;
    spf.format.redMax = ByteOrder::swap16ifle(31) ;
    spf.format.greenMax = ByteOrder::swap16ifle(63) ;
    spf.format.blueMax = ByteOrder::swap16ifle(31) ;
    if (ByteOrder::isLittleEndian()) {
	 spf.format.redShift = 11;
	 spf.format.greenShift = 5 ;
	 spf.format.blueShift = 0 ;
    } else {
	 spf.format.redShift = 0;
	 spf.format.greenShift = 5;
	 spf.format.blueShift = 11;
    }
  } else {
    spf.format.bitsPerPixel = 32 ;
    spf.format.depth = 24 ;
    spf.format.redMax = spf.format.greenMax = spf.format.blueMax = ByteOrder::swap16ifle(255) ;
    if (_encoding==Image::ARGB) {
	 if (ByteOrder::isLittleEndian()) {
	   spf.format.redShift = 8 ;
	   spf.format.greenShift = 16 ;
	   spf.format.blueShift = 24 ;
	 } else {
	   spf.format.blueShift = 0 ;
	 spf.format.greenShift = 8 ;
	 spf.format.redShift = 16 ;
	 }
    } else { // RGBA
	 if (ByteOrder::isLittleEndian()) {
	   spf.format.redShift = 0 ;
	   spf.format.greenShift = 8 ;
	   spf.format.blueShift = 16 ;
	 } else {
	   spf.format.redShift = 24 ;
	   spf.format.greenShift = 16 ;
	   spf.format.blueShift = 8 ;
	 }
    }
  }

		_conn->send((char *)&spf, sz_rfbSetPixelFormatMsg) ;

		// --- Encoding ----------------------------------------------

		char buf[sz_rfbSetEncodingsMsg + 5 * 4]; // 5 = MAX_ENCODINGS
		rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
		CARD32 *encs = (CARD32 *)(&buf[sz_rfbSetEncodingsMsg]);
		se->type = rfbSetEncodings;
#if !defined(NO_RFB_PADDING_IN_METISSE)
		se->pad = 0;
#endif
		se->nEncodings = 0;

		// Not supported by Metisse
		// encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingCopyRect);
		// encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingHextile);
		// encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingRRE);
		
		
		// broken here or in Metisse when resizing a window ?
		//encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingCoRRE);

		// any way raw encoding is the best encoding in the local
		// case ??

		if (_useShm)
		{
			encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingRawShm);
#if DEBUG_LEVEL>=1
			std::cerr << "Use shared memory for framebuffer update" << std::endl ;
#endif
		}
		else
		{
			encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingRaw);
		}


		encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingPointerPos);
		encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingXCursor);
#ifdef HAVE_XCURSOR
		encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingARGBCursor);
#endif
		encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingWindowShape);
		encs[se->nEncodings++] = ByteOrder::swap32ifle(rfbEncodingFacades);

		int len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;
		se->nEncodings = ByteOrder::swap16ifle(se->nEncodings);
		_conn->send(buf, len) ;

		// --- Here we go ! ------------------------------------------

		subscribeTo(_conn) ; // ready to receive data from the Metisse server

		updateRequest(true) ;
		bitsPerSeconds(0, METISSE_TIME_DEBUG_START, 0);

		return true ;
	} catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl ;
		stop() ;
		return false ;
	}
}

void
MetisseSource::react(Observable *obs)
{
  // std::cerr << "MetisseSource::react this=" << this << " obs=" << obs << " conn=" << _conn << std::endl ;
  if (obs==_conn && !_readMETISSEServer()) stop() ;
  notifyObservers() ;
}

bool MetisseSource::_framebufferUpdate(rfbServerToClientMsg msg)
{
	RECEIVE(((char *)&msg.fu) + 1, sz_rfbFramebufferUpdateMsg - 1) ;
	unsigned long nRects = ByteOrder::swap16ifle(msg.fu.nRects);
	unsigned char *shm = NULL, *s = NULL;
	CARD16 nshmRects = 0;
	unsigned long shmid = ByteOrder::swap32ifle(msg.fu.shmid);
	unsigned long xWindow = ByteOrder::swap32ifle(msg.fu.window);
	unsigned long isRoot = ByteOrder::swap32ifle(msg.fu.topWindow);

	bitsPerSeconds(
		0, METISSE_TIME_DEBUG_FBU_START, 0);

	if (isRoot == 0)
	{
		isRoot = 1;
	}
	else
	{
		isRoot = 0;
	}

#if DEBUG_LEVEL>=2
	fprintf(stderr, "MetisseSource[FramebufferUpdate]: window: 0x%lx (%i,%i)\n",
		xWindow, msg.fu.nRects, isRoot);
#endif

	MetisseWindow *MetisseWin = 0;

	MetisseWin = _metisseDesktop->findWindow(xWindow);
	float wWidth = 0, wHeight = 0;

	if (MetisseWin)
	{
		MetisseWin->getCurrentSize(&wWidth, &wHeight);
	}

	if (shmid)
	{
		shm = (unsigned char *)shmat(shmid, NULL, 0);
		if ((char *)shm == (char *)-1) perror("shmat (MetisseSource)");
		if (shmctl(shmid, IPC_RMID, 0)==-1) perror("shmctl IPC_RMID (MetisseSource)");
		if ((char *)shm == (char *)-1)  
		  shmid = 0;
		else
		{
			s = shm;
			memcpy(&nshmRects, s, sizeof(CARD16));
			s += sizeof(CARD16);
			nshmRects = ByteOrder::swap16ifle(nshmRects);
		}
		// std::cerr << "shmid: " << shmid << " nshmRects: " << nshmRects << std::endl ;
	}


	int i;
	for (i=0; i< nRects+nshmRects; ++i)
	{
	  	unsigned char *conv = NULL;

		rfbFramebufferUpdateRectHeader rect;
		if (i < nRects)
		{
			RECEIVE((char *)&rect,
				sz_rfbFramebufferUpdateRectHeader) ;
		}
		else
		{
			memcpy(&rect, s, sz_rfbFramebufferUpdateRectHeader);
			conv = s + sz_rfbFramebufferUpdateRectHeader;
			s += sz_rfbFramebufferUpdateRectHeader;
		}

		rect.r.x = ByteOrder::swap16ifle(rect.r.x);
		rect.r.y = ByteOrder::swap16ifle(rect.r.y);
		rect.r.w = ByteOrder::swap16ifle(rect.r.w);
		rect.r.h = ByteOrder::swap16ifle(rect.r.h);
		rect.encoding = ByteOrder::swap32ifle(rect.encoding);

#if 0
		std::cerr << "MetisseSource: rfbFramebufferUpdate " << rect.r.x
			  << "," << rect.r.y ;
		std::cerr << " " << rect.r.w << "x" << rect.r.h <<
			" encoding: " << rect.encoding << std::endl ;
#endif

		if (rect.encoding == rfbEncodingXCursor)
		{
			int bpr = (rect.r.w + 7) / 8;
			int bd = bpr * rect.r.h;
			char *buf = NULL;
			rfbXCursorColors colors;

			if (rect.r.w * rect.r.h)
			{
				RECEIVE((char *)&colors, sz_rfbXCursorColors);

				buf = new char[bd * 2];
				if (buf == NULL)
				{
					std::cerr << "MetisseSource: allocation failed" << std::endl;
					if (shmid!=0 && shmdt(shm)==-1) perror("shmdt (MetisseSource)") ;
					return False;
				}

				RECEIVE(buf, bd * 2);
			}
			_metisseDesktop->HandleXCursor(
				rect.r.x, rect.r.y, rect.r.w, rect.r.h,
				buf, colors);
			continue;
		}

		if (rect.encoding == rfbEncodingARGBCursor)
		{
#ifdef HAVE_XCURSOR
			char *buf = NULL;
			unsigned int bufSize = rect.r.w * rect.r.h * 4;
			rfbXCursorColors colors;

			if (bufSize)
			{
				buf = new char[bufSize];
				if (buf == NULL)
				{
					std::cerr << "MetisseSource: allocation failed" << std::endl;
					if (shmid!=0 && shmdt(shm)==-1) perror("shmdt (MetisseSource)") ;
					return False;
				}

				RECEIVE((char *)buf, bufSize);
			}
			else
			{
				_metisseDesktop->HandleXCursor(
					rect.r.x, rect.r.y, 0, 0,
					buf, colors);
			}
			_metisseDesktop->HandleARGBCursor(
				rect.r.x, rect.r.y,
				rect.r.w, rect.r.h,
				buf);
#else // HAVE_XCURSOR
			// should not happen !
			unsigned	buffer_size = rect.r.w * rect.r.h * 4;
			char	buffer[buffer_size];
			RECEIVE(buffer, buffer_size);
#endif // HAVE_XCURSOR
			continue;
		}

		if (rect.encoding == rfbEncodingPointerPos)
		{
			_metisseDesktop->handleCursorPosition(rect.r.x, rect.r.y);
			continue;
		}

		if (rect.encoding == rfbEncodingWindowShape)
		{
			/* rect.r contains the size of the window */
			rfbWindowShapeHeader h;
			RECEIVE((char *)&h, sz_rfbWindowShapeHeader) ;
			h.nrects = ByteOrder::swap32ifle(h.nrects);
			int size =  h.nrects * 4 * sizeof(CARD16) ;
			CARD16 *buf = NULL;
			if (size)
			{
				buf = new CARD16[size];
				RECEIVE((char *)buf, size) ;
			}
			_metisseDesktop->handleWindowShape(MetisseWin, buf, h.nrects);
			continue;
		}
		
		if (rect.encoding == rfbEncodingFacades)
		{
			rfbFacadesHeader h;
			RECEIVE((char *)&h, sz_rfbFacadesHeader) ;
			h.ncuts = ByteOrder::swap32ifle(h.ncuts);
			int size =  h.ncuts * 7 * sizeof(CARD32) ;
			// fprintf(stderr,"rfbEncodingFacades %i %i\n",
			//	h.ncuts, size);
			CARD32 *buf = NULL;
			if (size)
			{
				buf = new CARD32[size];
				RECEIVE((char *)buf, size) ;
			}
			_metisseDesktop->handleWindowFacades(MetisseWin, buf, h.ncuts);
			//fprintf(stderr,"rfbEncodingFacades Done\n");
			continue;
		}
		

		if ((rect.r.h * rect.r.w) == 0)
		{
			continue ;
		}
		
		if (MetisseWin &&
		    (rect.r.x+rect.r.w > (int)wWidth ||
		     rect.r.y+rect.r.h > (int)wHeight))
		{
		  std::cerr << "MetisseSource [rfbFramebufferUpdate]: "
				  << "rectangle too big ("
				  << rect.r.x << "," << rect.r.y
				  << " " << rect.r.w << "x" << rect.r.h
				  << " - " << (int)wWidth << "x" << (int)wHeight 
				  << ")" << std::endl ;
		  continue ;
		}

		switch (rect.encoding)
		{
		case rfbEncodingRawShm:
		case rfbEncodingRaw:
		{
			int rectImgSize;

			rectImgSize = rect.r.w * rect.r.h *
				Image::getBytesPerPixel(_encoding);

			bool isShm = false;

			Image tmpImg;
			if (shmid != 0)
			{
				isShm = true;
				tmpImg.setData(
					(unsigned char *)s, rectImgSize,
					Image::NONE);
				tmpImg.setDims(rect.r.w, rect.r.h);
				tmpImg.setEncoding(_encoding);
				s += rectImgSize;
			}
			else
			{
			  tmpImg.prepareFor(rect.r.w, rect.r.h, _encoding) ;
			  RECEIVE((char *)tmpImg.getData(),rectImgSize) ;
			}
#if 0
			std::cerr << "raw: " << rect.r.w << "x" << rect.r.h <<
				" " << rect.r.x << "," << rect.r.y << std::endl ;
#endif
			_metisseDesktop->handleImageFramebufferUpdate(
				MetisseWin, (bool)isRoot, &tmpImg,
				rect.r.x, rect.r.y, rect.r.w, rect.r.h);

			bitsPerSeconds(
				rectImgSize, METISSE_TIME_DEBUG_BITS, isShm);
		}
		break; 

		default:
			std::cerr << "MetisseSource: unknown rect encoding:" << std::hex << rect.encoding << std::endl ;
			if (shmid!=0 && shmdt(shm)==-1) perror("shmdt (MetisseSource)") ;
			return false;
		}
	}

	if (shmid!=0 && shmdt(shm)==-1) perror("shmdt (MetisseSource)") ;

	bitsPerSeconds(0, METISSE_TIME_DEBUG_FBU_STOP, 0) ;

	return true;
}

bool
MetisseSource::_readMETISSEServer(void)
{
	if (!_conn) return false;

	bool getAfbu = false;
	int fd = _conn->getFd() ;
	unsigned int i ;
	int count = 0;
	int fbcount = 0;
	int confCount = 0;
	int restackCount = 0;
	Chronometer _chrono ;

#if METISSE_BUFFERED_READS
	while (_chrono.read() < 1000 && (getavail(fd) || _buffer.length()))
#else
     for (bool loop=true; loop; loop=false)
#endif
	{
		rfbServerToClientMsg msg;
#if METISSE_BUFFERED_READS
		RECEIVE((char *)&msg, 1) ;
#else
		if (RECEIVE((char *)&msg, 1)!=1) break ;
#endif

		count++;

		switch (msg.type)
		{

		case rfbBell:
			// FIXME: do something fun with the window ??
			//std::cerr << "MetisseSource: ring my bell..." << std::endl;
			break ;

		case rfbSetColourMapEntries:
		{
		  // std::cerr << "MetisseSource: rfbSetColourMapEntries" << std::endl ;
			RECEIVE(
				((char *)&msg.scme) + 1,
				sz_rfbSetColourMapEntriesMsg - 1) ;
			int size = msg.scme.nColours * 3 /* * CARD16 */ ;
			char *tmp = new char [size] ;
			RECEIVE(tmp, size) ;
			delete tmp ;
		}
		break ;

		case rfbServerCutText:
		{
			RECEIVE(((char *)&msg.sct) + 1,
				sz_rfbServerCutTextMsg - 1) ;
			msg.sct.length = ByteOrder::swap16ifle(msg.sct.length);
			char *tmp = new char [msg.sct.length] ;
			RECEIVE(tmp, msg.sct.length) ;
			std::cerr << "MetisseSource: new text in cut buffer '"
				  << tmp << "'" << std::endl ;
			delete tmp;
		}
		break ;

		case rfbConfigureWindow:
		{
		  // std::cerr << "MetisseSource: rfbConfigureWindow" << std::endl ;
			RECEIVE(((char *)&msg.cw) + 1,
				sz_rfbConfigureWindowMsg - 1) ;
			msg.cw.window = ByteOrder::swap32ifle(msg.cw.window);
			msg.cw.xsgn =  ByteOrder::swap16ifle(msg.cw.xsgn);
			int x = ByteOrder::swap16ifle(msg.cw.x);
			if (msg.cw.xsgn == 0)
			{
				x = -x;
			}
			msg.cw.ysgn =  ByteOrder::swap16ifle(msg.cw.ysgn);
			int y = ByteOrder::swap16ifle(msg.cw.y);
			if (msg.cw.ysgn == 0)
			{
				y = -y;
			}
			msg.cw.width = ByteOrder::swap32ifle(msg.cw.width);
			msg.cw.height =	ByteOrder::swap32ifle(msg.cw.height);
			_metisseDesktop->handleConfigureWindow(
				msg.cw.window, x, y,
				(int)msg.cw.width, (int)msg.cw.height,
				ByteOrder::swap16ifle(msg.cw.isroot));
			confCount++;
		}
		break;

		case rfbUnmapWindow:
		{
		  // std::cerr << "MetisseSource: rfbUnmapWindow" << std::endl ;
			RECEIVE(((char *)&msg.uw) + 1,
				sz_rfbUnmapWindowMsg - 1) ;
			msg.uw.window = ByteOrder::swap32ifle(msg.uw.window);
			_metisseDesktop->handleUnmapWindow(msg.uw.window);
		}
		break;

		case rfbDestroyWindow:
		{
		  // std::cerr << "MetisseSource: rfbDestroyWindow" << std::endl ;
			RECEIVE(((char *)&msg.dw) + 1,
				sz_rfbDestroyWindowMsg - 1) ;
			msg.dw.window = ByteOrder::swap32ifle(msg.dw.window);
			_metisseDesktop->handleDestroyWindow(msg.dw.window);
		}
		break;
    
		case rfbRestackWindow:
		{
			#if 0
			std::cerr << "MetisseSource: rfbRestackWindow "
				  << sz_rfbRestackWindowMsg - 1
				  << " " << _buffer.length() << std::endl;
			#endif
			RECEIVE(((char *)&msg.rw) + 1,
				sz_rfbRestackWindowMsg - 1) ;
			msg.rw.window = ByteOrder::swap32ifle(msg.rw.window);
			msg.rw.nextWindow = ByteOrder::swap32ifle(msg.rw.nextWindow);
			msg.rw.transientFor = ByteOrder::swap32ifle(msg.rw.transientFor);
			msg.rw.unmanagedFor = ByteOrder::swap32ifle(msg.rw.unmanagedFor);
			msg.rw.grabWindow = ByteOrder::swap32ifle(msg.rw.grabWindow);
			msg.rw.duplicateFor = ByteOrder::swap32ifle(msg.rw.duplicateFor);
			msg.rw.facadeReal = ByteOrder::swap32ifle(msg.rw.facadeReal);
			msg.rw.flags = ByteOrder::swap32ifle(msg.rw.flags);
			_metisseDesktop->handleRestackWindow(
				msg.rw.window, msg.rw.nextWindow,
				msg.rw.transientFor, msg.rw.unmanagedFor,
				msg.rw.grabWindow, msg.rw.duplicateFor,
				msg.rw.facadeReal, msg.rw.flags);
			restackCount++;
		}
		break;

		case rfbFramebufferUpdate:
		{
		  // std::cerr << "MetisseSource: rfbFramebufferUpdate" << std::endl ;
			bool ret = _framebufferUpdate(msg);
			if (!ret)
			{
				return false;
			}
			fbcount++;
			getAfbu = true;
		}
		break;

		case rfbNoOp:
		{
			RECEIVE(((char *)&msg.no) + 1,
				sz_rfbNoOpMsg - 1) ;
			msg.no.counter = ByteOrder::swap32ifle(msg.no.counter);
			static int counter = 0;
			//std::cerr << "MetisseSource: rfbNoOp 0x"
			//	<< std::hex << i << std::dec << std::endl;
			if (counter != msg.no.counter)
				std::cerr << "MetisseSource: skipped rfbNoOp!!!!!" << std::endl;
			counter++;
		}
		break;

		default:
			std::cerr << "MetisseSource: unknown message type "
				  << std::hex << (int)msg.type << std::dec
				  << " from METISSE server" << std::endl ;
			return false;
			break ;
		} /* switch */

	}

	if (getAfbu)
	  bitsPerSeconds(0, METISSE_TIME_DEBUG_FRAME_STOP, 0);

#if 0
	if (count >= 11 || _chrono.read() >= 100)
	{
	    std::cerr << "Chrono: " <<  _chrono.read() << " Count: " << count
		      << " FB: " << fbcount
		      << " Conf: " << confCount
		      << " Restack: " << restackCount << std::endl;
	}
#endif

	if (count)
		updateRequest(true);

	return true;
}

bool
MetisseSource::stop(void) {
  if (_conn) {
    unsubscribeFrom(_conn) ;
    delete _conn ;
    _conn=NULL ;
    return true ;
  }
  return false ;
}

// ------------------------------------------------------------------

void
MetisseSource::updateRequest(bool incremental)
{
	updateRequest(0,0,10*_width,10*_height,incremental) ;
}

void
MetisseSource::updateRequest(int x, int y, int w, int h, bool incremental) {
  rfbFramebufferUpdateRequestMsg fur;

  fur.type = rfbFramebufferUpdateRequest;
  fur.incremental = incremental ? 1 : 0;
  fur.x = ByteOrder::swap16ifle(x);
  fur.y = ByteOrder::swap16ifle(y);
  fur.w = ByteOrder::swap16ifle(w);
  fur.h = ByteOrder::swap16ifle(h);

  _conn->send((char *)&fur, sz_rfbFramebufferUpdateRequestMsg) ;
}

void
MetisseSource::windowUpdateRequest(
	uint32_t win, int x, int y, int w, int h, bool incremental)
{
	rfbWindowUpdateRequestMsg wur;
#if 0
	fprintf(stderr,"MetisseSource::windowUpdateRequest:\n") ;
	fprintf(stderr,"  sizeof(rfbWindowUpdateRequest)=%d, sizeof(wur)=%d, sz_rfbWindowUpdateRequestMsg=%d\n", sizeof(rfbWindowUpdateRequestMsg), sizeof(wur), sz_rfbWindowUpdateRequestMsg) ;
	fprintf(stderr,"  d1=%d, d2=%d, d3=%d, d4=%d, d5=%d, d6=%d\n",
		   ((int)&wur.incremental-(int)&wur.type),
		   ((int)&wur.window-(int)&wur.type),
		   ((int)&wur.x-(int)&wur.type),
		   ((int)&wur.y-(int)&wur.type),
		   ((int)&wur.w-(int)&wur.type),
		   ((int)&wur.h-(int)&wur.type)) ;
#endif
	wur.type = rfbWindowUpdateRequest;
	wur.incremental = incremental ? 1 : 0;
	wur.window = ByteOrder::swap32ifle(win);
	wur.x = ByteOrder::swap16ifle(x);
	wur.y = ByteOrder::swap16ifle(y);
	wur.w = ByteOrder::swap16ifle(w);
	wur.h = ByteOrder::swap16ifle(h);

	_conn->send((char *)&wur, sz_rfbWindowUpdateRequestMsg) ;
}

void
MetisseSource::windowUpdateRequest(uint32_t win)
{
	windowUpdateRequest(win, 0, 0, 0, 0, false);
}

void
MetisseSource::keyEvent(unsigned long key, bool down_flag) {
	rfbKeyEventMsg ke;

	ke.type = rfbKeyEvent ;
	ke.down = down_flag ? 1 : 0 ;

	ke.key = ByteOrder::swap32ifle(key) ;
	_conn->send((char *)&ke, sz_rfbKeyEventMsg) ;
}

void
MetisseSource::pointerEvent(
	uint32_t win, int x, int y, unsigned long button_mask)
{
	rfbPointerEventMsg pe;

#if 0
	std::cerr << "MetisseSource::pointerEvent: " << std::hex << id << std::dec
		  <<" " << x << " " << y << " "
		  << button_mask << " " << sizeof(rfbPointerEventMsg) << std::endl;
#endif
      
#if 0
	if (x < 0 || y < 0)
	{
		return;
	}
#endif

	pe.type = rfbPointerEvent;
#if !defined(NO_RFB_PADDING_IN_METISSE)
	pe.pad1 = 0;
#endif
	pe.buttonMask = ByteOrder::swap32ifle(button_mask);
	int xsgn = 1, ysgn = 1;

	if (x < 0)
	{
		xsgn = 0;
		x = -x;
	}
	if (y < 0) {
		ysgn = 0;
		y = -y;
	}
	pe.xsgn = xsgn;
	pe.x = ByteOrder::swap16ifle((unsigned int)x) ;
	pe.ysgn = ysgn;
	pe.y = ByteOrder::swap16ifle((unsigned int)y) ;
	pe.window = ByteOrder::swap32ifle(win);
	_conn->send((char *)&pe, sz_rfbPointerEventMsg) ;
}

void
MetisseSource::pointerEvent(int x, int y, unsigned long button_mask)
{
	pointerEvent(0, x, y, button_mask);
}

void MetisseSource::getSize(unsigned int *width, unsigned int *height)
{
	*width = _width;
	*height = _height;
}
