/*
 *
 * main/compositor.cxx --
 *
 * Copyright (C) Nicolas Roussel
 * Copyright (C) Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config-not-xserver.h"
#endif

#include <nucleo/nucleo.H>
#include <nucleo/core/ReactiveEngine.H>
#include <nucleo/utils/AppUtils.H>
#include <nucleo/utils/FileUtils.H>
#include <nucleo/gl/window/glWindow.H>
#include <nucleo/image/sink/ImageSink.H>

#include <sstream> // here because WinRegions.H, included by
			    // WindowRenderer.H, defines max and min...

#include "renderer/WindowRenderer.H"
#include "desktop/MetisseWindow.H"
#include "desktop/MetisseDesktop.H"
#include "fvwmmodule/FvwmModule.H"
#include "AUtils.H"
#include "AScreen.H"
#include "Persistence.H"

#include <sys/time.h>

using namespace nucleo ;

#define VERBOSE 0

void
quitFvwmAndExit(int fds[2]) {
  SendText(fds, "Quit", 0) ;
  FvwmPacket* packet = ReadFvwmPacket(fds[1]) ;
  if (packet!=NULL) std::cerr << "quitFvwmAndExit: received a non-NULL packet" << std::endl ;
  exit(0) ;
}

static void
showUsage(void) {
  std::cerr 
    << "[-C (software cursor)] "
    << "[-D(isplay fps)] "
    << "[-s (disable window shadow)] " 
    << "[-o (rthogonal projection)] "
    << "[-d display] "
    << "[-w metisse-server-uri] "
    << "[-n (use npot texture extension)] "
    << "[-m (do not use shared memory)] "
    << "[-t (do not use the stencil buffer)] "
    << "[-p (use mipmaps)] "
    << "[-e image-encoding] "
    << "[-P (portrait mode)] "
    << "[-F (force fullscreen)] "
    << "[-c capture-uri] "
    << std::endl << std::endl
    << "Warning: metisse-server-uri and display MUST be specified" 
    << std::endl << std::endl ;
}

int
main(int argc, char **argv)
{
#if VERBOSE
  std::cerr << "argc=" << argc << " argv=[" << argv[0] ;
  for (int ia=1; ia<argc; ia++) 
    std::cerr << " " << argv[ia] ;
  std::cerr << "]" << std::endl ;
#endif

  /*
    From http://www.fvwm.org/documentation/dev_modules.php
    
    argv[0] is the full pathname of the module
    argv[1] and argv[2] are file descriptors that can be used to communicate with fvwm
    argv[3] is the full path name of the configuration file from which the module was started
    argv[4] is the application window in whose context the module was launched
    argv[5] is the context of the window decoration in which the module was launched

    argv[6] and above are user specified command line arguments
  */

  char *myName = GetFileNameFromPath(argv[0]);

  if (argc<6) { // FIXME: No better way of detecting this?
    std::cerr << "This program can only run as an FVWM module" 
		    << std::endl << std::endl
		    << "Usage: " << myName << " " ;
    showUsage() ;
    exit(1) ;
  }

  int fvwm_fds[2] ;
  fvwm_fds[0] = atoi(argv[1]) ;
  fvwm_fds[1] = atoi(argv[2]) ;

  std::stringstream rsBuffer ;
  rsBuffer << "AFuncRestartModule " << myName ;
  for (int ia=6; ia<argc; ++ia) rsBuffer << " " << argv[ia] ;
  std::string restartString=rsBuffer.str() ;
#if VERBOSE
  std::cerr << "restartString = [" << restartString << "]" << std::endl ;
#endif

  int user_argc = argc-5 ;
  char **user_argv = new char * [user_argc] ;
  user_argv[0] = strdup(argv[0]) ;
  for (int uia=1; uia<user_argc; ++uia) user_argv[uia] = strdup(argv[5+uia]) ;
#if VERBOSE
  std::cerr << "uargc=" << user_argc << " uargv=[" << user_argv[0] ;
  for (int uia=1; uia<user_argc; uia++) 
    std::cerr << " " << user_argv[uia] ;
  std::cerr << "]" << std::endl ;
#endif

  bool USE_SOFTWARE_CURSOR = false ;
  bool SHOW_FPS = false ;
  bool SHADOW = false ;
  bool ORTHO = false ;
  bool NOSTENCIL = false;
  char *DISPLAY = 0 ;
  char *METISSE_URI = 0 ;
  bool USE_MIPMAPS = false;
  char *ENCODING = "PREFERRED" ;
  bool PORTRAIT = false;
  bool FORCE_FS = false;
  char *CAPTURE = 0 ;

  // NPOT_EXT is disabled by default as XFree-4.4.0 (rc1) has this
  // extension but it is totally broken with the radeon driver
  bool USE_NPOT_EXT = false ;
  bool NO_SHM = false;

  if (parseCommandLine(
	      user_argc, user_argv, "CDsod:w:nmtpe:PFc:", "bbbbssbbbbsbbs",
	      &USE_SOFTWARE_CURSOR, &SHOW_FPS, &SHADOW, &ORTHO,
	      &DISPLAY, &METISSE_URI, &USE_NPOT_EXT, &NO_SHM, &NOSTENCIL,
	      &USE_MIPMAPS, &ENCODING, &PORTRAIT, &FORCE_FS,
	      &CAPTURE) < 0)
  {	
    std::cerr << std::endl << myName << " " ;
    showUsage() ;
    quitFvwmAndExit(fvwm_fds) ;
  }

  if (!METISSE_URI || !DISPLAY)
  {
    std::cerr << "w=" << METISSE_URI << " d=" << DISPLAY << std::endl ;
    std::cerr << std::endl << myName << ": " ;
    std::cerr << "METISSE URI and display MUST be specified (using -w and -d)" ;
    std::cerr << std::endl << std::endl ;
    quitFvwmAndExit(fvwm_fds) ;
  }

  // ------------------------------------------------------
  // Connect to the Metisse server via a MetisseDesktop

  MetisseDesktop *metisseDesktop = new MetisseDesktop(
	  METISSE_URI, "foldable", (NO_SHM)? false:true, ENCODING) ;

  // ------------------------------------------------------

  setenv("DISPLAY",DISPLAY,1) ;
  
  if (ORTHO)
  {
	  ORTHO = true;
  }

  if (!USE_NPOT_EXT)
  {
	  glTiledTexturedImage::use_npot_ext = false;
  }
  if (USE_MIPMAPS)
  {
	  glTiledTexturedImage::do_generate_mipmaps = true;
  }

  // ------------------------------------------------------

  AScreen *AScr = new AScreen(metisseDesktop, ORTHO, NOSTENCIL, PORTRAIT, FORCE_FS);

  //AScr->setTitle(metisseDesktop->desktopName.c_str()) ;
  AScr->setTitle("FvwmCompositor") ;
  metisseDesktop->setAScreen(AScr);

  {
    unsigned int w,h;
    metisseDesktop->getSize(&w, &h);
    AScr->setGeometry(w, h);
    ReactiveEngine::step() ;
    if (USE_SOFTWARE_CURSOR) AScr->useSoftwareCursor();
  }

  AScr->makeCurrent() ;

  // ------------------------------------------------------

  FvwmModule fvwmModule(
	  AScr, metisseDesktop, fvwm_fds[0], fvwm_fds[1], myName, restartString);

  glClearColor(0.7,0.71,0.7,1) ;
  glEnable(GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (SHOW_FPS) AScr->showFPS();

  if (SHADOW) WindowRenderer::shadowByDefault = false;

  // ------------------------------------------------------

  fvwmModule.sendWindowList();
  persistence_init();

  if (CAPTURE) AScr->startVideoCapture(CAPTURE) ;

  // sgNode::debugMode = true ;

  std::cerr << __FILE__ << ": hi!" << std::endl ;
  ReactiveEngine::run() ;
  std::cerr << __FILE__ << ": bye..." << std::endl ;

  delete AScr;

  return 0 ;
}
