/* -*-c-*- */

#ifndef F_RECORD_H
#define F_RECORD_H

/* ---------------------------- included header files ---------------------- */

#ifdef HAVE_XRECORD
#define XRecordSupport 1
#else
#define XRecordSupport 0
#endif

#include <X11/Xproto.h>

#if XRecordSupport
#include <X11/extensions/record.h>
#endif

/* ---------------------------- warping lib ------------------------- */

#if XRecordSupport

#define FRecordBadContext XRecordBadContext

#define	FRecordFromServerTime XRecordFromServerTime
#define	FRecordFromClientTime XRecordFromClientTime
#define	FRecordFromClientSequence XRecordFromClientSequence 

#define FRecordCurrentClients XRecordCurrentClients 
#define FRecordFutureClients XRecordFutureClients
#define FRecordAllClients XRecordAllClients

#define FRecordFromServer XRecordFromServer
#define FRecordFromClient XRecordFromClient
#define FRecordClientStarted XRecordClientStarted
#define FRecordClientDied XRecordClientDied
#define FRecordStartOfData XRecordStartOfData
#define FRecordEndOfData XRecordEndOfData

typedef XRecordClientSpec FRecordClientSpec;
typedef XRecordContext FRecordContext;
typedef XRecordRange8 FRecordRange8;
typedef XRecordRange16 FRecordRange16;
typedef XRecordExtRange FRecordExtRange;
typedef XRecordRange FRecordRange;
typedef XRecordClientInfo FRecordClientInfo;
typedef XRecordState FRecordState;
typedef XRecordInterceptData FRecordInterceptData;

/* *
 * this typedef should, according to XRecord,  
 * specification be defined in <X11/extensions/record.h>
 * ... can't find .... errrh?
 *
 */
typedef union { 
  unsigned char    type ;
  xEvent           event ;
  xResourceReq     req   ;
  xGenericReply    reply ;
  xError           error ;
  xConnSetupPrefix setup;
} FRecordDatum ;

#define FRecordIdBaseMask(a) XRecordIdBaseMask(a)
#define FRecordQueryVersion(a,b,c) XRecordQueryVersion(a,b,c)
#define FRecordCreateContext(a,b,c,d,e,f) XRecordCreateContext(a,b,c,d,e,f)
#define FRecordAllocRange() XRecordAllocRange()
#define FRecordRegisterClients(a,b,c,d,e,f,g) XRecordRegisterClients(a,b,c,d,e,f,g)
#define FRecordUnregisterClients(a,b,c,d) XRecordUnregisterClients(a,b,c,d)
#define FRecordGetContext(a,b,c) XRecordGetContext(a,b,c)
#define FRecordFreeState(a) FRecordFreeState(a)

#if 1
typedef void (*FRecordInterceptProc) (
#ifdef __STDC__
    XPointer , FRecordInterceptData*
#endif
);
#endif

#define FRecordEnableContext(a,b,c,d) XRecordEnableContext(a,b,c,d)
#define FRecordEnableContextAsync(a,b,c,d) XRecordEnableContextAsync(a,b,c,d)
#define FRecordProcessReplies(a) XRecordProcessReplies(a)
#define FRecordFreeData(a) XRecordFreeData(a)
#define FRecordDisableContext(a,b) XRecordDisableContext(a,b)
#define FRecordFreeContext(a,b) XRecordFreeContext(a,b)

#else

#define FRecordBadContext       0

#define	FRecordFromServerTime		0x01
#define	FRecordFromClientTime		0x02
#define	FRecordFromClientSequence	0x04

#define FRecordCurrentClients		1
#define FRecordFutureClients		2
#define FRecordAllClients		3

#define FRecordFromServer           	0
#define FRecordFromClient               1
#define FRecordClientStarted           	2
#define FRecordClientDied               3
#define FRecordStartOfData		4
#define FRecordEndOfData		5

typedef unsigned long   FRecordClientSpec;
typedef unsigned long 	FRecordContext;

typedef struct
{
    unsigned char 	first;
    unsigned char 	last;
} FRecordRange8;

typedef struct
{
    unsigned short 	first;
    unsigned short 	last;
} FRecordRange16; 

typedef struct
{
    FRecordRange8	ext_major;
    FRecordRange16 	ext_minor; 
} FRecordExtRange; 

typedef struct
{
    FRecordRange8     core_requests;	/* core X requests */
    FRecordRange8     core_replies;	/* core X replies */
    FRecordExtRange   ext_requests;	/* extension requests */
    FRecordExtRange   ext_replies;	/* extension replies */
    FRecordRange8     delivered_events;	/* delivered core and ext events */
    FRecordRange8     device_events; 	/* all core and ext device events */
    FRecordRange8     errors;		/* core X and ext errors */
    Bool	      client_started;	/* connection setup reply */
    Bool              client_died; 	/* notice of client disconnect */    
} FRecordRange;

typedef struct
{
    FRecordClientSpec 	client;
    unsigned long 	nranges; 
    FRecordRange 	**ranges;
} FRecordClientInfo;

typedef struct
{
    Bool 		enabled;
    int			datum_flags; 
    unsigned long 	nclients; 
    FRecordClientInfo 	**client_info;
} FRecordState;

typedef struct
{
    XID 		id_base;
    Time		server_time; 
    unsigned long 	client_seq;
    int			category; 
    Bool 		client_swapped;
    unsigned char 	*data;
    unsigned long 	data_len;	/* in 4-byte units */
} FRecordInterceptData;

/*
 *
 */
typedef union { 
  unsigned char    type ;
  xEvent           event ;
  xResourceReq     req   ;
  xGenericReply    reply ;
  xError           error ;
  xConnSetupPrefix setup;
} FRecordDatum ;

#define FRecordIdBaseMask(a) 0
#define FRecordQueryVersion(a,b,c) 0
#define FRecordCreateContext(a,b,c,d,e,f) None
#define FRecordAllocRange() NULL
#define FRecordRegisterClients(a,b,c,d,e,f,g) 0
#define FRecordUnregisterClients(a,b,c,d) 0
#define FRecordGetContext(a,b,c) 0
#define FRecordFreeState(a)

typedef void (*FRecordInterceptProc) (
#ifdef __STDC__
    XPointer, FRecordInterceptData*
#endif
);

#define FRecordEnableContext(a,b,c,d) 0
#define FRecordEnableContextAsync(a,b,c,d) 0
#define FRecordProcessReplies(a)
#define FRecordFreeData(a)
#define FRecordDisableContext(a,b) 0
#define FRecordFreeContext(a,b) 0

#endif

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

typedef enum
{
	FRECORD_EVENT  = 0,
	FRECORD_REQUEST,
	FRECORD_REPLY,
	FRECORD_ERROR,
	FRECORD_DELIVERED_EVENT,
	FRECORD_DEVICE_EVENT,
	FRECORD_EXT_REPLY_MAJOR,
	FRECORD_EXT_REPLY_MINOR,
	FRECORD_EXT_REQUEST_MAJOR,
	FRECORD_EXT_REQUEST_MINOR,
	FRECORD_NR_OF_TYPES,
	FRECORD_META_DATA,
	FRECORD_NO_DATA          
}  FRecordDataType;

typedef struct
{
	Display *dpy_control;
	Display *dpy_data;
	FRecordClientSpec *xids; 
	int nclients;
	FRecordContext Context;
	FRecordState *rState;
	FRecordRange **range_array ;
	int data_ranges[FRECORD_NR_OF_TYPES];
	int data_flags;
	FRecordInterceptProc dispatch;
	XPointer closure;
	Bool has_dispatched;
	Bool do_record;
} FRecordRecordDescription;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- interface functions ------------------------ */

Bool FRecordInit(Display *dpy);
int FRecordGetErrorCodeBase(void);
int FRecordGetMajorOpCode(void);
Bool FRecordGetExtensionSupported(Display *dpy);

int FRecordAddRange (
	FRecordRecordDescription *frd, FRecordDataType type, int start,
	int stop);
FRecordRecordDescription *FRecordCreateDescription(Display *dpy);
void FRecordStartRecording(FRecordRecordDescription *frd);
void FRecordStopRecording(FRecordRecordDescription *frd);
void FRecordFreeRecordDescription(FRecordRecordDescription *frd);
void FRecordMyProcessReplies(FRecordRecordDescription *frd);

void FRecordProcessRepliesAll(void);
Bool FRecordShouldProcessReplies(void);
void FRecordPrintListInfo(void);

FRecordRecordDescription *FRecordCreateDescriptionS(
	Display *dpy, unsigned long mask);
#endif
