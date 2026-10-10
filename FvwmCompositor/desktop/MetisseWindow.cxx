/*
 *
 * desktop/MetisseWindow.cxx --
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

#include "MetisseWindow.H"

#include <nucleo/image/sink/ImageSink.H>
#include <nucleo/gl/glUtils.H>
#include <nucleo/image/processing/basic/Paint.H>
#include <nucleo/image/encoding/Conversion.H>
#include <algorithm>

#include "main/AScreen.H"
#include "main/Persistence.H"
#include "main/AUtils.H"

#include "renderer/WindowRenderer.H"
#ifdef HAVE_SPI
#include "main/WidgetManager.H"
#endif

#include "MetisseSource.H"
#include "MetisseDesktop.H"

/*
 * this #include and the three following functions are still needed
 * because of REGION_*, which are used later on to handle facades
 */
extern "C" {
#include "xserver/include/region.h"
}

void Xfree(pointer a)
{
	if (a) 
		free((char *)a);
}

void *Xalloc(unsigned long amount)
{
    register pointer  ptr;
	
    if ((long)amount <= 0) {
	return (unsigned long *)NULL;
    }
    /* aligned extra on long word boundary */
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
    if ((ptr = (pointer)malloc(amount))) {
	return (unsigned long *)ptr;
    }
    return (unsigned long *)NULL;
}

void *Xrealloc(pointer ptr, unsigned long amount)
{
    if ((long)amount <= 0)
    {
	if (ptr && !amount)
	    free(ptr);
	return (unsigned long *)NULL;
    }
    amount = (amount + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
    if (ptr)
        ptr = (pointer)realloc((char *)ptr, amount);
    else
	ptr = (pointer)malloc(amount);
    if (ptr)
        return (unsigned long *)ptr;
    return (unsigned long *)NULL;
}
/* end REGION_* helper code */


using namespace std;


float MetisseWindow::rootTransY = 0;
float MetisseWindow::rootTransX = 0;

MetisseWindow::MetisseWindow(
	std::string title, unsigned long id, MetisseSource *metisseSource,
	AScreen *ascr, int x, int y, unsigned int width, unsigned int height)
{
  
	_title = title;
	_name = title;
	_metisseSource = metisseSource;
	_ascr = ascr;
	_surfaceScaleSaveX = _real_x = _rrx = x;
	_surfaceScaleSaveY = _real_y = _rry = y;
	x = y = 0;
	_interTransX = _interTransY = 0;
	_attachedTransX = _attachedTransY = 0;

	_width = _rwidth = width;
	_height = _rheight = height;
	_frameID = id;
	_clientID = 0;
	_stackingPos = 0;

	_texture = (glTiledTexturedImage*)0;

	_resetTexture = false;
	_trx = 0;
	_try = 0;

	_mapped = false;
	_neverMapped = true;
	_changed = true;

	_red = _green = _blue = 1.0;
	_alpha = _alpha_saved = 1.0;
 
	_texCoordsSize = 0 ;
	_texCoords = (float*)0 ;
	_renderer = (WindowRenderer*)0 ;

	_unmanaged = false;
	_isRootWindow = false;
	_isEwmhDesktop = false;
	_isToolTips = false;
	_transientFor = 0;
	_unmanagedFor = 0;
	_transientForOrig = 0;
	_unmanagedForOrig = 0;
	_cutOrig = 0;

	_duplicateFor = 0;
	
	_pseudoSwallowNode = 0;
	_pseudoSwallowedBy = 0;

	_isAFacadeWindow = false;
	_facade.clear();
	_facadeChanged = false;
	_numOfMatchingFacades = 0;
	_facadeHideCount = 0;
	_isAToolGlass = 0;

	_facadeDesc = 0;
	_facadeReal = 0;

#ifdef HAVE_SPI
	_widgetValuators.clear();
	_accessible = 0;
#endif
	_sb_cross = false;
	_sb_relaction = 0;
	_prev_button_mask = _ascr->getButtonMask();

	_flags = 0;
	_titleHeight = 0;
	_borderWidth = 0;
	_decorLeft = _decorTop = _decorRight = _decorBottom = 0;
	_fclient_width = _fclient_height = 0;
	_windowName = 0;
	_resClass = 0;
	_resName = 0;

	_transientForCorrectionDirty = true;
	_desktopWidth = 0;
	_desktopHeight = 0;

	_rootTransX = 0;
	_rootTransY = 0;

	_surfaceRotationY = 0;
	_surfaceRotationX = 0;
	_surfaceRotationZ = 0;
	_surfaceScale = 1;
	_surfaceRotationAllowed = true;

	_absoluteScaleX = 1;
	_absoluteScaleY = 1;
	_absoluteScaleZ = 1;

	_absoluteRotateAngle = 0;
	_absoluteRotateX = 0;
	_absoluteRotateY = 0;
	_absoluteRotateZ = 0;

	_inMovePage = false;
	_currentTranslationForced = false;

	#if 0
	std::cerr << "MetisseWindow " << _frameID << "\n  "
		  << _real_x << " " << _real_y << " " << _width << " "
		  << _height << "\n";
	#endif
}

MetisseWindow::~MetisseWindow()
{
	if (_texCoords)
	{
		delete [] _texCoords;
	}
	if (_texture)
	{
		delete _texture;
	}
	if (_renderer)
	{
		delete _renderer;
	}
	if (_cutOrig)
	{
		delete _cutOrig;
	}
	if (_flags)
	{
		free(_flags);
	}

	setWindowName(0);
	setResClass(0);
	setResName(0);

	FacadeMatchingMap::iterator i;

	for (i = _matchingFacadesOver.begin(); i != _matchingFacadesOver.end();
	     i++)
	{
		delete (*i).second;
	}
	for (i = _matchingFacades.begin(); i != _matchingFacades.end();
	     i++)
	{
		delete (*i).second;
	}
	if (_facadeReal != 0)
	{
		// ? let leaks for now ... design pbs!
		// delete _facadeReal;
	}
#ifdef HAVE_SPI
	vallist::iterator iter;
	for (iter =  _widgetValuators.begin(); iter != _widgetValuators.end();
	     iter++)
	{
		delete (*iter);
	}
	if (_accessible)
	{
		//Accessible_unref(_accessible);
	}
#endif
	
}

// --------------------------------------------------------------

std::string
MetisseWindow::getTitle(void)
{
	return _title ;
}

void
MetisseWindow::setRenderer(WindowRenderer *renderer)
{
	if (_renderer)
	{
		removeDependency(_renderer) ;
		delete _renderer ;
	}
	_renderer = renderer ;
	addDependency(renderer);
	_changed = true;
}

WindowRenderer *MetisseWindow::getRenderer(void)
{
	return _renderer;
}

// --------------------------------------------------------------

void MetisseWindow::setSize(float width, float height)
{
  // never called ?
	_width = _rwidth = width ;
	_height = _rheight = height ;
	_changed = true;
}

void MetisseWindow::getCurrentSize(float *width, float *height)
{
	*width = (float)_rwidth ;
	*height = (float)_rheight ;
}

void MetisseWindow::getSize(float *width, float *height)
{
	*width = (float)_width ;
	*height = (float)_height ;
}

void MetisseWindow::getSize(double *width, double *height)
{
	*width = (float)_width ;
	*height = (float)_height ;
}

void MetisseWindow::getSize(int *width, int *height)
{
	*width = (int)_width ;
	*height = (int)_height ;
}

void MetisseWindow::getSize(unsigned int *width, unsigned int *height)
{
	*width = (unsigned int)_width ;
	*height = (unsigned int)_height ;
}
void MetisseWindow::getClientSize(int *width, int *height)
{
	if (_fclient_width != 0 && _fclient_height != 0)
	{
		*width = (int)_fclient_width;
		*height = (int)_fclient_height;
	}
	else
	{
		*width = (int)_width - (_decorLeft + _decorRight);
		*height = (int)_height - (_decorTop + _decorBottom);
	}
}

void MetisseWindow::setRealPosition(float x, float y, bool saveScalePos)
{
	_real_x = _rrx = x;
	_real_y = _rry = y;
	if (isUnmanaged())
	{
		_transientForCorrectionDirty = true;
	}
	if (saveScalePos)
	{
		saveSurfaceScaleRealPosition();
	}
	if (_pseudoSwallowedBy != 0)
	{
		MetisseWindow *win =
			dynamic_cast<MetisseWindow *>(_pseudoSwallowedBy);
		if (win != 0)
		{
			win->getRenderer()->simpleReset();
		}
	}
}

void MetisseWindow::getCurrentRealPosition(float *x, float *y)
{
	*x = _rrx;
	*y = _rry;
}

void MetisseWindow::getRealPosition(float *x, float *y)
{
	*x = _real_x;
	*y = _real_y;
}

void MetisseWindow::getRealPosition(int *x, int *y)
{
	*x = (int)_real_x;
	*y = (int)_real_y;
}

int MetisseWindow::getStackingPos(void)
{
	return _stackingPos;
}

void MetisseWindow::setStackingPos(int s)
{
	_stackingPos = s;
}

bool MetisseWindow::inMovePage(void)
{
	return _inMovePage;
}

void MetisseWindow::setInMovePage(bool v)
{
	_inMovePage = v;
}

void MetisseWindow::setMapped(bool mapped)
{
	_mapped = mapped;
	
	//std::cerr << "set Mapped: " << _frameID << " " << _mapped << "\n";
	if (_mapped)
	{
		_neverMapped = false;
	}
	_transientForCorrectionDirty = true;
	_changed = true;
}


bool MetisseWindow::isMapped(void)
{
	return _mapped;
}

bool MetisseWindow::neverMapped(void)
{
	return _neverMapped;
}

void MetisseWindow::setUnmanaged(bool u)
{
	_unmanaged = u;
}

void MetisseWindow::setUnmanaged(void)
{
	setUnmanaged(true);
}

bool MetisseWindow::isUnmanaged(void)
{
	return _unmanaged;
}

void MetisseWindow::setRootWindow(void)
{
	_isRootWindow = true;
}

bool MetisseWindow::isRootWindow(void)
{
	return _isRootWindow;
}

void MetisseWindow::setEwmhDesktop(void)
{
	_isEwmhDesktop = true;
}

bool MetisseWindow::isEwmhDesktop(void)
{
	return _isEwmhDesktop;
}

void MetisseWindow::setToolTips(void)
{
	_alpha = 0.8; // FIXME: should be configurable
	_isToolTips = true;
}

bool MetisseWindow::isToolTips(void)
{
	return _isToolTips;
}

void MetisseWindow::_applyTransientTransform(void)
{
	if (_transientFor == 0)
	{
		return;
	}

	MetisseWindow *w = dynamic_cast<MetisseWindow *>(_transientFor);

	if (w == 0)
	{
		return;
	}

	GLfloat trans[16];

	#if 0
	std::cerr << "_applyTransientTransform "
		  << this << " " << w << std::endl;
	#endif

	w->getRendererTransformation(trans);
	getRenderer()->setTransformation(trans);

}

void MetisseWindow::_attachTransient(void)
{
	if (_transientFor == 0)
	{
		return;
	}

	MetisseWindow *w = dynamic_cast<MetisseWindow *>(_transientFor);

	if (w == 0)
	{
		return;
	}

	_ascr->root->attachWindow(w,this);
	#if 0
	std::cerr << "attachTransient " << this << " " << w << " "
		  << w->getDuplicateFor() << std::endl;
	#endif
}

void MetisseWindow::setTransientFor(sgNode *n, sgNode *on, cut *c)
{
	_transientFor = n;
	_transientForOrig = on;
	if (_cutOrig)
	{
		delete _cutOrig;
	}
	_cutOrig = c;

	if (n == 0)
	{
		return;
	}

	MetisseWindow *win = dynamic_cast<MetisseWindow *>(n);
	if (win == 0 || !win->isEwmhDesktop())
	{
		return;
	}
	win->pseudoSwallow(this);
	_pseudoSwallowedBy = win;
	win->getRenderer()->simpleReset();
}

sgNode *MetisseWindow::getTransientFor(void)
{
	return _transientFor;
}

sgNode *MetisseWindow::getTransientForOrig(void)
{
	return _transientForOrig;
}

void MetisseWindow::setUnmanagedFor(sgNode *n, sgNode *on, cut *c)
{
	_unmanagedFor = n;
	_transientFor = n;
	_unmanagedForOrig = on;
	_transientForOrig = on;
	if (_cutOrig)
	{
		delete _cutOrig;
	}
	_transientForCorrectionDirty = true;
	_cutOrig = c;
}

sgNode *MetisseWindow::getUnmanagedFor(void)
{
	return _unmanagedFor;
}

sgNode *MetisseWindow::getUnmanagedForOrig(void)
{
	return _unmanagedForOrig;
}

cut *MetisseWindow::getCutOrig(void)
{
	return _cutOrig;
}

bool MetisseWindow::isSticky(void)
{
	if (_flags && (_flags->common.is_sticky_across_pages))
	{
		return true;
	}
	return false;
}

void MetisseWindow::setToolGlass(bool v)
{
	_isAToolGlass = v;
}

bool MetisseWindow::isAToolGlass()
{
	return _isAToolGlass;
}

void MetisseWindow::pseudoSwallow(sgNode *n)
{
	_pseudoSwallowNode = n;
}

sgNode *MetisseWindow::getPseudoSwallowedNode(void)
{
	return _pseudoSwallowNode;
}

void MetisseWindow::setPseudoSwallowedBy(sgNode *n)
{
	_pseudoSwallowedBy = n;
}

sgNode *MetisseWindow::getPseudoSwallowedBy(void)
{
	return _pseudoSwallowedBy;
}

void MetisseWindow::setDuplicateFor(sgNode *n)
{
	_duplicateFor = n;
}

sgNode *MetisseWindow::getDuplicateFor(void)
{
	return _duplicateFor;
}

bool MetisseWindow::setFacade(FacadeMapping wcm, bool add)
{
	bool changed = false;
	bool hadFacade = (_facade.size() > 0)? true:false;
	int oldFacadeSize = _facade.size();

	if (wcm.size() != _facade.size())
	{
		changed = true;
	}
	else if (wcm.size() > 0)
	{
		int ns = 0, os = 0;

		FacadeMapping::iterator iter;
		for (iter = wcm.begin(); iter != wcm.end(); iter++)
		{
			ns += ((*iter).second).size();
		}
		for (iter = _facade.begin(); iter != _facade.end(); iter++)
		{
			os += ((*iter).second).size();
		}
		std::cerr << "os " << os << " ns " << ns << "\n";
		if (ns != os)
		{
			changed = true;
		}
		for (iter = wcm.begin(); iter != wcm.end(); iter++)
		{
			_facade.erase((*iter).first);
		}
		if (_facade.size() > 0)
		{
			changed = true;
		}
	}
	_facade = wcm;
	_facadeChanged = true;

	// std::cerr << "Set Facdes " << hadFacade << " " << changed << " "
	//	  << oldFacadeSize << "\n";

	if (!hadFacade || (hadFacade && _facade.size() == 0))
	{
		if (hadFacade && _facade.size() == 0 && !_isAFacadeWindow)
		{
			FacadeMatchingMap::iterator i;

			for (i = _matchingFacadesOver.begin();
			     i != _matchingFacadesOver.end(); i++)
			{
				FacadeMatchStatus *fms = (*i).second;
				FacadeDescription *fd = (*i).first;
				FacadeRealisation *fr_found = NULL;

				std::set<FacadeRealisation *>::iterator j;
				for (j = fd->realisations.begin();
				     j != fd->realisations.end();
				     j++)
				{
					FacadeRealisation *fr = (*j);
					if (fr->overRealWin != this)
					{
						continue;
					}
					fr_found = fr;
					break;
				}

				if (fr_found == NULL)
				{
					continue;
				}

				if (!(fms->realized && fd->hideSources))
				{
					fd->realisations.erase(fr_found);
					delete fr_found;
					fms->realized = false;
					continue;
				}

				WindowMapping wins =
					_ascr->getMetisseDesktop()->getWindowMapping();
				std::set<MetisseWindow *>::iterator l; 
				for (l = fr_found->WindowCutList.begin();
				     l != fr_found->WindowCutList.end();
				     l++)
				{
					MetisseWindow *win = (*l);
					WindowMapping::iterator k =
						wins.find(win->getFrameID()) ;
					if (k == wins.end())
					{
						continue;
					}
					win->hideAsFacadeSource(false);
				}
				fd->realisations.erase(fr_found);
				delete fr_found;
				fms->realized = false;
			}
		}
		buildSpecialMenu();
	}
	else if (hadFacade && add && !_isAFacadeWindow)
	{
		FacadeMatchingMap::iterator i;

		// std::cerr << "over facade changed!\n";
		for (i = _matchingFacadesOver.begin();
		     i != _matchingFacadesOver.end(); i++)
		{
			FacadeMatchStatus *fms = (*i).second;
			FacadeDescription *fd = (*i).first;
			FacadeRealisation *fr_found = NULL;

			std::set<FacadeRealisation *>::iterator j;
			for (j = fd->realisations.begin();
			     j != fd->realisations.end();
			     j++)
			{
				FacadeRealisation *fr = (*j);
				if (fr->overRealWin != this)
				{
					continue;
				}
				fr_found = fr;
				break;
			}

			if (fr_found)
			{

				// std::cerr << "fr_found\n";
				fd->realisations.erase(fr_found);
				delete fr_found;
				fms->realized = false;
				break;
			}
			// FIXME: hideAsFacadeSource(false); for removed
			// no it is ok for now ... we can all remove all
			// is over facade element
		}
		buildSpecialMenu();
	}
	else if (changed && _isAFacadeWindow)
	{
		if (_facadeReal)
		{
			// lets leak!
			//delete _facadeReal;
			_facadeReal = 0;
			// should rm _facadeDesc ?
			buildSpecialMenu();
		}
		// FIXME: maybe removed an element just added to a
		// saved facade!
	}
	

#if 0
	std::cerr << "Set Win Cuts " << std::endl;
	FacadeMapping::iterator iter;
	for (iter = _facade.begin(); iter != _facade.end(); iter++)
	{
		std::cerr << (*iter).first << ": ";
		cutlist cl = (*iter).second;
		cutlist::iterator jter;
		for (jter = cl.begin(); jter != cl.end(); jter++)
		{
			std::cerr << (*jter).sx << " " << (*jter).sy << " " 
				  << (*jter).sw << " " << (*jter).sh << " " 
				  << (*jter).dx << " " << (*jter).dy
				  << std::endl;
		}
	}
#endif
	return changed;
}

FacadeMapping  MetisseWindow::getFacade(void)
{
	return _facade;
}

void MetisseWindow::rmFacade(sgNode *n)
{
	FacadeMapping::iterator iter = _facade.find(n);
	if (iter == _facade.end())
	{
		return;
	}

	if (isAFacadeWindow())
	{
		MetisseWindow *w = dynamic_cast<MetisseWindow *>((*iter).first);
		cutlist cl = (*iter).second;
		cutlist::iterator jter;
		for (jter = cl.begin(); jter != cl.end(); jter++)
		{
			_ascr->getMetisseDesktop()->rmFacade((*jter), w, this);
		}
	}
	
	_facade.erase(n);

	if (!hasAFacade())
	{
		buildSpecialMenu();
	}

	_facadeChanged = true;
}

void MetisseWindow::rmFacade(sgNode *n, cut *c)
{
	FacadeMapping::iterator iter = _facade.find(n);
	if (iter == _facade.end())
	{
		return;
	}

	cutlist cl = (*iter).second;
	cutlist::iterator jter;
	for (jter = cl.begin(); jter != cl.end(); jter++)
	{
		if ((*jter).sx == c->sx && (*jter).sy == c->sy &&
		    (*jter).sw == c->sw && (*jter).sh == c->sh &&
		    (*jter).dx == c->dx && (*jter).dy == c->dy)
		{
			break;
		}
	}

	if (jter == cl.end())
	{
		return;
	}

	if (isAFacadeWindow())
	{
		MetisseWindow *w = dynamic_cast<MetisseWindow *>((*iter).first);
		_ascr->getMetisseDesktop()->rmFacade((*jter), w, this);
	}

	cl.erase(jter);
	if (cl.size() == 0)
	{
		_facade.erase(n);

		if (!hasAFacade())
		{
			buildSpecialMenu();
		}
	}

	_facadeChanged = true;	
}

bool MetisseWindow::isOneOfOurFacade(sgNode *n)
{
	FacadeMapping::iterator iter = _facade.find(n);
	if (iter != _facade.end())
	{
		return true;
	}
	return false;
}

Bool MetisseWindow::isAFacadeWindow(void)
{
	return _isAFacadeWindow;
}

Bool MetisseWindow::setIsAFacadeWindow(void)
{
	_isAFacadeWindow = true;
	buildSpecialMenu();
}

bool MetisseWindow::hasAFacade(void)
{
	if (_facade.size() > 0)
	{
		return true;
	}
	return false;
}

// ---------------------------------------------------------------
// All this stuff is for facade persitance ...

// ----
bool MetisseWindow::addMatchingFacade(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacades.find(fd);

	if (i != _matchingFacades.end())
	{
		if ((*i).second->to_be_removed)
		{
			(*i).second->to_be_removed = false;
			return true;
		}
		return false;
	}

	//std::cerr << "addMatchingFacade\n";
	FacadeMatchStatus *fms = new FacadeMatchStatus();
	_matchingFacades[fd] = fms;

	return true;
}

bool MetisseWindow::removeMatchingFacade(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacades.find(fd);

	if (i == _matchingFacades.end())
	{
		return false;
	}
	//std::cerr << "removeMatchingFacade\n";
	FacadeMatchStatus *fms = (*i).second;
	fms->to_be_removed = true;

	return true;
}

void MetisseWindow::setFacadeRealisation(
	FacadeDescription *fd, FacadeRealisation *fr)
{
	// here we are the facade window ...
	_facadeDesc = fd;
	_facadeReal = fr;
	// should send to the source?
	buildSpecialMenu();
}

FacadeDescription *MetisseWindow::deleteRealisedDescription(void)
{
	if (!_facadeReal)
	{
		return NULL;
	}
	_facadeDesc->realisations.erase(_facadeReal);
	// lets leak!
	delete _facadeReal;
	_facadeReal = NULL;
	// should send to the source?
	return _facadeDesc;
}

// ---

bool MetisseWindow::addMatchingFacadeOver(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacadesOver.find(fd);

	if (i != _matchingFacadesOver.end())
	{
		FacadeMatchStatus *fms = (*i).second;
		if (fms->to_be_removed)
		{
			fms->to_be_removed = false;
			return true;
		}
		return false;
	}

	//std::cerr << "addMatchingFacadeOver\n";
	FacadeMatchStatus *fms = new FacadeMatchStatus();
	_matchingFacadesOver[fd] = fms;

	return true;
}

bool MetisseWindow::removeMatchingFacadeOver(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacadesOver.find(fd);

	if (i == _matchingFacadesOver.end())
	{
		return false;
	}

	//std::cerr << "removeMatchingFacadeOver\n";
	FacadeMatchStatus *fms = (*i).second;
	fms->to_be_removed = true;

	return true;
}

void MetisseWindow::setFacadeOverWin(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacadesOver.find(fd);

	if (i != _matchingFacadesOver.end())
	{
		FacadeMatchStatus *fms = (*i).second;
		fms->over_win = true;
	}
}

void MetisseWindow::setOverFacadeRealisation(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacadesOver.find(fd);

	if (i == _matchingFacadesOver.end())
	{
		std::cerr << "MetisseWindow::setOverFacadeRealisation "
			  << "Should not happen!\n"; 
		return;
	}
	FacadeMatchStatus *fms = (*i).second;
	fms->realized = true;
	buildSpecialMenu();
}

FacadeDescription *MetisseWindow::deleteOverRealisedDescription(void)
{
	FacadeDescription *r_fd = NULL;
	FacadeMatchingMap::iterator i;

	for (i = _matchingFacadesOver.begin();
	     i != _matchingFacadesOver.end(); i++)
	{
		FacadeMatchStatus *fms = (*i).second;
		FacadeDescription *fd = (*i).first;
		FacadeRealisation *fr_found = NULL;

		std::set<FacadeRealisation *>::iterator j;
		for (j = fd->realisations.begin();
		     j != fd->realisations.end();
		     j++)
		{
			FacadeRealisation *fr = (*j);
			if (fr->overRealWin != this)
			{
				continue;
			}
			fr_found = fr;
			break;
		}
		if (fr_found)
		{
			// std::cerr << "fr_found\n";
			fd->realisations.erase(fr_found);
			delete fr_found;
			_matchingFacadesOver.erase(fd);
			_numOfMatchingFacades--;
			delete fms;
			r_fd = fd;
			break;
		}
	}

	return r_fd;
}

// --

bool MetisseWindow::deleteFacadeDescription(FacadeDescription *fd)
{
	FacadeMatchingMap::iterator i = _matchingFacadesOver.find(fd);

	if (i != _matchingFacadesOver.end())
	{
		FacadeMatchStatus *fms = (*i).second;
		if (fd->matched)
		{
			_numOfMatchingFacades--;
		}
		_matchingFacadesOver.erase(fd);
		delete fms;
		return true;
	}

	i = _matchingFacades.find(fd);

	if (i != _matchingFacades.end())
	{
		FacadeMatchStatus *fms = (*i).second;
		if (fd->matched)
		{
			_numOfMatchingFacades--;
		}
		_matchingFacades.erase(fd);
		delete fms;
		return true;
	}
}

void MetisseWindow::hideAsFacadeSource(bool on)
{
	static int prev_x=0,prev_y=0;

	if (on)
	{
		//std::cerr << "hideAsFacadeSource on " << _facadeHideCount
		//	  << " " << this << "\n";

		if (_facadeHideCount == 0)
		{
			prev_x = (int)_real_x;
			prev_y = (int)_real_y;
			_ascr->sendFvwmCmd("Move 5000p 5000p", this);	
		}
		_facadeHideCount++;
	}
	else
	{
		char cmd[356];

		//std::cerr << "hideAsFacadeSource off " << _facadeHideCount
		//	  << " " << this << "\n";
		if (_facadeHideCount == 1)
		{
			sprintf(cmd,"Move %ip %ip",prev_x,prev_y); 
			// std::cerr << cmd << "\n";
			_ascr->sendFvwmCmd(cmd, this);	
			_facadeHideCount = 0;
		}
		else
		{
			_facadeHideCount--;
			if (_facadeHideCount < 0)
			{
				_facadeHideCount = 0;
			}
		}
	}
}

//--
void MetisseWindow::checkFacadeMenu(void)
{
	bool rebuild = false;
	int count = 0;
	std::list<FacadeDescription *> rmlist;
	std::list<FacadeDescription *> over_rmlist;
	FacadeMatchingMap::iterator i;

	//std::cerr << "checkFacadeMenu\n";
	for (i = _matchingFacadesOver.begin();
	     i != _matchingFacadesOver.end(); i++)
	{
		bool m = (!(*i).second->to_be_removed && (*i).first->matched);
		if (m != (*i).second->in_menu)
		{
			rebuild = true;
		}
		if (m)
		{
			count++;
		}
		if ((*i).second->to_be_removed)
		{
			//delete (*i).second;
			over_rmlist.push_front((*i).first);
		}
	}
	for (i = _matchingFacades.begin(); i != _matchingFacades.end(); i++)
	{
		bool m = (!(*i).second->to_be_removed && (*i).first->matched);
		if (m != (*i).second->in_menu)
		{
			rebuild = true;
		}
		if (m)
		{
			count++;
		}
		if ((*i).second->to_be_removed)
		{
			//delete (*i).second;
			rmlist.push_front((*i).first);
		}
	}
	std::list<FacadeDescription *>::iterator j;
	for (j = over_rmlist.begin(); j != over_rmlist.end(); j++)
	{
		_matchingFacadesOver.erase((*j));
	}
	for (j = rmlist.begin(); j != rmlist.end(); j++)
	{
		_matchingFacades.erase((*j));
	}
	_numOfMatchingFacades = count;
	if (rebuild)
	{
		buildSpecialMenu();
	}
}

// ---------------------------------------------------------------
#ifdef HAVE_SPI
Accessible *MetisseWindow::getTopAccessible(void)
{
	if (_accessible == 0)
	  _accessible = _ascr->registerToWidgetManager(this);
	return _accessible; 
}

AWidgetValuator *MetisseWindow::_findMatchingValuator(AWidgetValuator *val)
{
	long x, y, w, h;
	if (!val->getExtents(&x, &y, &w, &h))
	{
		// should not happen
		return 0;
	}
	vallist::iterator vter;
	for (vter =  _widgetValuators.begin(); vter != _widgetValuators.end();
	     vter++)
	{
		long xx, yy, ww, hh;
		if ((*vter)->getExtents(&xx, &yy, &ww, &hh) &&
		    x == xx && y == yy && w == ww & h == hh)
		{
			return (*vter);
		}
	}
	return 0;
}

void MetisseWindow::setWidgetValuator(AWidgetValuator *sb)
{
	//std::cerr << "setWidgetValuator\n";
	AWidgetValuator *my_val = _findMatchingValuator(sb);
	
	if (sb->getSpeed() == -1)
	{
		if (my_val)
		{
			//std::cerr << "Remove\n";
			_widgetValuators.remove(my_val);
			delete my_val;
		}
		delete sb;
		return;
	}

	if (my_val)
	{
		my_val->setSpeed(sb->getSpeed());
		//std::cerr << "Update\n";
		delete sb;
	}
	else
	{
		//std::cerr << "Add\n";
		_widgetValuators.push_front(sb);
	}
}
#endif

// ---------------------------------------------------------------
void MetisseWindow::setWindowName(char *name)
{
	if (_windowName != 0)
	{
		free(_windowName);
		_windowName = 0;
	}
	if (name == 0)
	{
		return;
	}
	
	_windowName = strdup(name);
}

void MetisseWindow::setResClass(char *name)
{
	if (_resClass != 0)
	{
		free(_resClass);
		_resClass = 0;
	}
	if (name == 0)
	{
		return;
	}
	
	_resClass = strdup(name);
}

void MetisseWindow::setResName(char *name)
{
	if (_resName != 0)
	{
		free(_resName);
		_resName = 0;
	}
	if (name == 0)
	{
		return;
	}
	
	_resName = strdup(name);
}

char *MetisseWindow::getWindowName(void)
{
	return _windowName;
}

char *MetisseWindow::getResClass(void)
{
	return _resClass;	
}
char *MetisseWindow::getResName(void)
{
	return _resName ;
}


unsigned long MetisseWindow::getFrameID(void)
{
	return _frameID;
}

unsigned long MetisseWindow::getClientID(void)
{
	return _clientID;
}

bool MetisseWindow::isIconified(void)
{
	if (_flags)
	{
		return (_flags->is_iconified)? true:false;
	}

	return false;
}

void MetisseWindow::setIconified(bool v)
{
	if (_flags)
	{
		_flags->is_iconified = v;
	}
}

int MetisseWindow::getDesk(void)
{
	return _desk;
}

bool MetisseWindow::isManaged(void)
{
	if (_flags)
	{
		return true;
	}

	return false;
}
void MetisseWindow::setFvwmParameters(
	window_flags *flags, int th, int bw, unsigned long clientID,
	unsigned long int frame_w, unsigned long int frame_h, int desk,
	bool instartup)
{
	bool first_time = false;

	if (_flags)
	{
		free(_flags);
	}
	else
	{
		first_time = true;
	}

	_desk = desk;
	unsigned long int old_cw, old_ch;
	old_cw = _fclient_width;
	old_ch = _fclient_height;

	int oldleft = _decorLeft;
	int oldtop = _decorTop;
	int oldright = _decorRight;
	int oldbottom = _decorBottom;
	int tbw = bw;
	_flags = flags;
	_titleHeight = th;
	_borderWidth = bw;
	_clientID = clientID;
	_decorLeft = _decorTop = _decorRight = _decorBottom = bw;
	_fclient_width = frame_w - 2*bw;
	_fclient_height = frame_h - 2*bw;
	//std::cerr << "th: " << th << " bw: " << bw << " has title: " <<
	// flags->has_title << std::endl;
	if (true || flags->has_title)
	{
		switch(flags->common.title_dir)
		{
		case DIR_E:
			_decorRight += th;
			_fclient_width -= th;
			break;
		case DIR_W:
			_decorLeft += th;
			_fclient_width -= th;
			break;
		case DIR_S:
			_decorBottom += th;
			_fclient_height -= th;
			break;
		default:
		case DIR_N:
			_decorTop += th;
			_fclient_height -= th;
			break;	
		}
	}

	bool decoChanged = false;
	if (oldleft != _decorLeft ||
	    oldtop != _decorTop ||
	    oldright != _decorRight ||
	    oldbottom != _decorBottom)
	{
		decoChanged = true;
	}

	if (decoChanged || _facadeChanged)
	{
		if (decoChanged)
		{
			if (_texture)
			{
				Image tmpImg;
				// encoding!
				tmpImg.prepareFor(
					(int)_rwidth, (int)_rheight,
					_texture->getTexture().getEncoding());
				paintImage(&tmpImg, 195, 195, 195, 255);
				_texture->update(&tmpImg);
			}
			_metisseSource->windowUpdateRequest(getFrameID());
			Image tmpImg;
		}
		else
		{
			_facadeChanged = false;
		}
		FacadeMapping::iterator iter;
		for (iter = _facade.begin(); iter != _facade.end(); iter++)
		{
			MetisseWindow *w =
				dynamic_cast<MetisseWindow *>((*iter).first);
			if (w == 0)
			{
				continue;
			}
			_metisseSource->windowUpdateRequest(w->getFrameID());
		}
	}
	
	if (first_time)
	{
		buildSpecialMenu();
	}
	else if (!instartup && !_ascr->inInteractiveResize())
	{
		if (_fclient_width != old_cw || _fclient_height != old_ch)
		{
			//std::cerr << "FVWMPAR:  persistence_window_check\n";
			persistence_window_check(this, false, _ascr);
		} 
	}
}


int MetisseWindow::getBorderWidth(void)
{
	return _borderWidth;
}

void MetisseWindow::getDecorStrut(int *left, int *top, int *right, int *bottom)
{
	*left = _decorLeft;
	*top = _decorTop;
	*right = _decorRight;
	*bottom = _decorBottom;
}

int MetisseWindow::getMaxDecorStrut(void)
{
	return max(max(_decorLeft,_decorTop), max(_decorRight,_decorBottom));
}

bool MetisseWindow::_buildFacadeMenu(void)
{
	//std::cerr << "_buildFacadeMenu " << _numOfMatchingFacades << "\n";
	if (_numOfMatchingFacades == 0)
	{
		return false;
	}

	_ascr->sendFvwmCmd("+ I + \"\" Nop", 0);
	char cmd[1024];
	FacadeMatchingMap::iterator i;

	for (i = _matchingFacadesOver.begin();
	     i != _matchingFacadesOver.end(); i++)
	{
		FacadeDescription *fd = (*i).first;
		FacadeMatchStatus *fms = (*i).second;
		if (!fd->matched)
		{
			fms->in_menu = false;
			continue;
		}
		if (!fms->over_win)
		{
			continue;
		}
 
		fms->in_menu = true;
		if (fd->automatic && fms->realized)
		{
			sprintf(cmd,
				"+ I + \"Over Facade %s (auto/ok)\" Nop", fd->name);
		}
		else if (fms->realized)
		{
			sprintf(cmd,
				"+ I + \"Over Facade %s (done)\" Nop", fd->name);
		}
		else if (fd->automatic)
		{
			sprintf(cmd,
				"+ I + \"Over Facade %s (auto/rm)\" Pick sma "
				"RecallFacade \"%s\"", fd->name,
				fd->name);
		}
		else
		{
			sprintf(cmd,
				"+ I + \"Over Facade %s\" Pick sma "
				"RecallFacade \"%s\"", fd->name,
				fd->name);
		}
		_ascr->sendFvwmCmd(cmd, 0);
	}
	for (i = _matchingFacades.begin(); i != _matchingFacades.end(); i++)
	{
		FacadeDescription *fd = (*i).first;
		FacadeMatchStatus *fms = (*i).second;
		if (!fd->matched)
		{
			fms->in_menu = false;
			continue;
		}
		fms->in_menu = true;
		sprintf(cmd,
			"+ I + \"Facade %s\" Pick sma "
			"RecallFacade \"%s\"", fd->name,
			fd->name);
		_ascr->sendFvwmCmd(cmd, 0);
	}

	return true;
}

void MetisseWindow::buildSpecialMenu(void)
{
	char cmd[1024];
	char funcname[256];

	if (_unmanaged)
	{
		return;
	}

	sprintf(funcname, "FuncInterMakeMenuInterWindow0x%lx", getClientID());
	sprintf(cmd,"DestroyFunc %s", funcname);
	//std::cerr << "funcname: " << funcname << "\n";
	_ascr->sendFvwmCmd(cmd, 0);
	sprintf(cmd,"AddToFunc  %s", funcname);
	_ascr->sendFvwmCmd(cmd, 0);

	_ascr->sendFvwmCmd("+ I DestroyMenu recreate MenuInterWindow", 0);
	_ascr->sendFvwmCmd("+ I AddToMenu MenuInterWindow", 0);

	if (hasAFacade())
	{
		bool real = false;
		// FIXME: or unsave facade in case it is realized
		// fprintf(stderr,"buildSpecialMenu: hasAFacade %s\n",
		//	_windowName);
		if (!isAFacadeWindow())
		{
			FacadeMatchingMap::iterator i;
			for (i = _matchingFacadesOver.begin();
			     i != _matchingFacadesOver.end(); i++)
			{
				if (((*i).second)->realized)
				{
					real = true;
				}
			}
			// fprintf(stderr,"Not a facade window %i\n", real);
		}
		else
		{
			// I am a facade window ... ok
			
		}
		if (real || _facadeReal)
		{
			_ascr->sendFvwmCmd("+ I + \"UNSave Facade\" "
					   "Pick sma UnSaveFacade", 0);	
		}
		else
		{
			_ascr->sendFvwmCmd("+ I + \"Save Facade\" "
					   "Pick sma SaveFacadeDialogue", 0);
		}
		if (!isAFacadeWindow())
		{
			_ascr->sendFvwmCmd(
				"+ I + \"Remove Facade\" sma rmFacade", 0);
		}
	}
	if (getRenderer() != 0 && getRenderer()->hasCutShape())
	{
		_ascr->sendFvwmCmd("+ I + \"Remove Holes\" sma UnCutWindow", 0);
	}
	_buildFacadeMenu();
	_ascr->generateExtInputWindowMenu(this);

	sprintf(cmd, "+ I ChangeMenuStyle AMenuStyleWinOps MenuInterWindow");
	_ascr->sendFvwmCmd(cmd, 0);
}

// --------------------------------------------------------------

void MetisseWindow::setBgcolor(float red, float green, float blue, float alpha)
{
	_red = red ;
	_green = green ;
	_blue = blue ;
	_alpha = alpha ;
	_changed = true ;
}

void MetisseWindow::setAlpha(float alpha)
{
	_alpha = alpha ;
	_changed = true ;
}

void MetisseWindow::setTmpAlpha(float alpha)
{
	_alpha_saved = _alpha;
	_alpha = alpha ;
	_changed = true;
}

void MetisseWindow::resetAlpha(void)
{
	_alpha = _alpha_saved;
	_changed = true;
}

void MetisseWindow::getBgcolor(float *red, float *green, float *blue, float *alpha)
{
	*red = _red ;
	*green = _green ;
	*blue = _blue ;
	*alpha = _alpha ;
}

float MetisseWindow::getAlpha(void)
{
	return _alpha ;
}

bool MetisseWindow::doPseudoSelection(void)
{
	return _ascr->doPseudoSelection();
}

// --------------------------------------------------------------
// Drawing stuff

bool MetisseWindow::inTextureReset(void)
{
	return _resetTexture;
}

void MetisseWindow::textureResetSetRealPosition(float x, float y)
{
	_trx = _trx - _rrx + x;
	_try = _try + _rry - y;
	_rrx = x;
	_rry = y;
}

void MetisseWindow::resetTexture(float width, float height)
{
	_resetTexture = true;

	_trx = _trx + (-_rwidth + width)/2;
	_try = _try + (_rheight - height)/2;

	_rwidth = width;
	_rheight = height;
	#if 0
	std::cerr << "resetTexture " << _frameID << " " << _mapped << "\n"
		  << _real_x << " " << _real_y << " " << _width << " "
		  << _height << "\n"
		  << _rrx << " " << _rry << " " << _rwidth << " "
		  << _rheight << "\n";
	#endif
	if (_pseudoSwallowedBy != 0)
	{
		MetisseWindow *win =
			dynamic_cast<MetisseWindow *>(_pseudoSwallowedBy);
		if (win != 0)
		{
			win->getRenderer()->simpleReset();
		}
	}
}

void MetisseWindow::deleteTexture(void)
{
	delete _texture;
	//std::cerr << "Delete Texture " << _frameID << "\n";
	_texture = 0;
}

bool MetisseWindow::doDraw(void)
{
	if (true || _ascr->inInteractiveResize())
	{
		return !(_texture == 0);
	}
	
	return !(_texture == 0 || _resetTexture);
}

void MetisseWindow::myUpdateTexture(
	Image *img, int x, int y, unsigned int w, unsigned int h)
{
	if (_texture == 0 || _resetTexture)
	{
		Image tmpImg;
		bool badSize = false;

		if (x == 0 && y == 0 && w == _rwidth && h == _rheight)
		{
			// ok
			//std::cerr << "Update ResetTexture\n";
		}
		else
		{
#if 0
		  std::cerr << "BS: " << w << " " << h << ", "  << (int)_rwidth
			    << " " << (int)_rheight << std::endl;
#endif
			tmpImg.prepareFor(
				(int)_rwidth, (int)_rheight, img->getEncoding());
			paintImage(&tmpImg, 195, 195, 195, 255);
			badSize = true;
		}

		if (w == 0 || h == 0)
		{
			return;
		}

		if (_resetTexture)
		{
			#if 0
			std::cerr << "myUpdateTexture: Reset Texture "
				  << _frameID << "\n";
			#endif
			translate_rel(_trx, _try, 0);
		}
		#if 0
		else
		{
			std::cerr << "myUpdateTexture: Create Texture "
				  << _frameID << "\n";
		}
		#endif
		_width = _rwidth;
		_height = _rheight;
		_real_x = _rrx;
		_real_y = _rry;
		if (_resetTexture && isUnmanaged())
		{
			//std::cerr << "-- resetTexture call correction \n";
			_transientForCorrectionDirty = true;
		}
		saveSurfaceScaleRealPosition();

		_trx = 0;
		_try = 0;
		_resetTexture = false;
		
		if (_renderer)
		{
			_renderer->simpleReset();
		}

		if (_texture == 0)
		{
			_texture = new glTiledTexturedImage(0);
		}

		if (badSize)
		{
			_texture->update(&tmpImg);
			_texture->subUpdate(img, x, y, w, h);
		}
		else
		{
			_texture->update(img);
		}

		float left = 0 ;
		float top = 0 ;
		float right = _width /(float)_texture->getWidth();
		float bottom = _height /(float)_texture->getHeight();
		setTextureCoordinates(left, top, right, bottom);

		postRedisplay();
		return;
	}

	_texture->subUpdate(img, x, y, w, h);
	postRedisplay() ;

	return;
}

void MetisseWindow::updateTexture(
	Image *img, int x, int y, unsigned int w, unsigned int h,
	bool redirected)
{

	if (!redirected && (getDuplicateFor() != 0 || isAFacadeWindow()))
	{
		// we are redirected and we get an unpdate for us
		int ix,iy,sx,sy;
		int iw,ih;
		Image::Encoding enc = img->getEncoding();

		sx = sy = 0;

		int top, left, right, bottom;
		if (getDuplicateFor() != 0)
		{
			top = left = right = bottom = _borderWidth;
		}
		else
		{
			top = _decorTop;
			left = _decorLeft;
			right = _decorRight;
			bottom = _decorBottom;
		}
		// top
		if (y < top)
		{
			iy = y;
			ih = top - y;
			ix = x;
			iw = w;

			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, sx, sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
		}
		// left 
		if (x < left)
		{
			iy = y;
			ih = h;
			ix = x;
			iw = left - x;

			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, sx, sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
		}
		// bottom
		if (y+h > _height - bottom)
		{
			iy = ((int)_height - bottom> y)?
				(int)_height - bottom:y;
			ih = (y+h) - ((int)_height - bottom);
			ix = x;
			iw = w;
			sx = 0;
			sy = h - ih;

			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, sx, -sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
		}
		// right
		if (x+w > _width - right)
		{
			ix = ((int)_width - right> x)?
				(int)_width - right:x;
			iw = (x+w) - ((int)_width - right);
			iy = y;
			ih = h;
			sx = w - iw;
			sy = 0;

			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, -sx, sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
		}
		return;
	}
	else if (!redirected && hasAFacade())
	{
		Image::Encoding enc = img->getEncoding();
		RegionRec updateRegion;
		RegionRec tmpRegion;
		BoxRec box;

		box.x1 = x;
		box.y1 = y;
		box.x2 = x+w;
		box.y2 = y+h;

		REGION_INIT(0,&updateRegion,&box,0);
		FacadeMapping::iterator iter;
		for (iter = _facade.begin(); iter != _facade.end(); iter++)
		{
			 
			cutlist cl = (*iter).second;
			cutlist::iterator jter;
			for (jter = cl.begin(); jter != cl.end(); jter++)
			{
				box.x1 = (*jter).dx;
				box.y1 = (*jter).dy;
				box.x2 = (*jter).dx + (*jter).sw;
				box.y2 = (*jter).dy + (*jter).sh;
				REGION_INIT(0,&tmpRegion,&box,0);
				REGION_SUBTRACT(
					0,&updateRegion, &updateRegion,
					&tmpRegion);
				REGION_UNINIT(0,&tmpRegion);
			}
		}
		for (int i = 0; i < REGION_NUM_RECTS(&updateRegion); i++)
		{
			int ix = REGION_RECTS(&updateRegion)[i].x1;
			int iy = REGION_RECTS(&updateRegion)[i].y1;
			int iw = REGION_RECTS(&updateRegion)[i].x2 - ix;
			int ih = REGION_RECTS(&updateRegion)[i].y2 - iy;
			int sx = x - ix;
			int sy = y - iy;
			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, sx, sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
		}
		REGION_UNINIT(0,&updateRegion);
		return;
	}
	else if (redirected && getDuplicateFor() != 0)
	{
		// intersect with the borders
		int ix,iy,sx,sy;
		int iw,ih;
		Image::Encoding enc = img->getEncoding();
		
		iw = w; ih = h; ix = x; iy = y;
		sx = 0;
		sy = 0;

		int top, left, right, bottom;
		top = left = right = bottom = _borderWidth;

		if (x <= _borderWidth)
		{
			ix = _borderWidth;
			if ((int)w - (ix - x) <= 0)
			{
				return;
			}
			iw = w - (ix - x);
			sx = w - iw;
		}
		if (y <= _borderWidth)
		{
			iy = _borderWidth;
			if ((int)h - (iy - y) <= 0)
			{
				return;
			}
			ih = h - (iy - y);
			sy = h - ih;
		}
		if (ix + iw > (int)_width - _borderWidth)
		{
			iw = - ix + (int)_width - _borderWidth;
		}
		if (iy + ih > (int)_height - _borderWidth)
		{
			ih = - iy + (int)_height - _borderWidth;
		}
		if (iw <= 0 || ih <= 0)
		{
			return;
		}

		if (ix != x || iy != y || iw != w || ih != h)
		{
			Image iImg;
			iImg.prepareFor(
				(int)iw, (int)ih, img->getEncoding());
			drawImageInImage(img, &iImg, -sx, -sy);
			convertImage(&iImg, enc);
			myUpdateTexture(&iImg, ix, iy, iw, ih);
			return;
		}
	}

	myUpdateTexture(img, x, y, w, h);

}

void MetisseWindow::facadeUpdateTexture(
	sgNode *winc, Image *img, int x, int y, unsigned int w, unsigned int h)
{
	if (!isOneOfOurFacade(winc))
	{
		return;
	}

	MetisseWindow *wc = dynamic_cast<MetisseWindow *>(winc);

	if (wc == 0)
	{
		return;
	}

	// intersect with the borders
	int ix,iy,sx,sy;
	int iw,ih;
	Image::Encoding enc = img->getEncoding();
		
	iw = w; ih = h; ix = x; iy = y;
	sx = 0;
	sy = 0;

	int top = 0, left = 0, right = 0, bottom = 0;
	if (isAFacadeWindow())
	{
		top = _decorTop;
		left = _decorLeft;
		right = _decorRight;
		bottom = _decorBottom;
	}
	// loop over the cuts

	FacadeMapping::iterator iter = _facade.find(wc);
	if (iter == _facade.end())
	{
		return;
	}

	cutlist cl = (*iter).second;
	cutlist::iterator jter;

	for (jter=cl.begin(); jter!= cl.end(); ++jter)
	{
		int x1 = max(x, (*jter).sx);
		int y1 = max(y, (*jter).sy);
		int x2 = min(x + w, (*jter).sx + (*jter).sw);
		int y2 = min(y + h, (*jter).sy + (*jter).sh);

		ix = x1;
		iy = y1;
		iw = x2 - x1;
		ih = y2 - y1;

#if 0
		std::cerr << "  I: " << x1 << " " << x2
			  << " " << y1 << " " << y2 << std::endl;
#endif
		
		if (x2 > x1 && y2 > y1)
		{
			// rectangle intersect
		}
		else
		{
			continue;

		}
#if 0
		std::cerr << "  Inter: " << ix << " " << iy
			  << " " << iw << " " << ih << std::endl;
#endif
		sy = (iy - y);
		sx = (ix - x);
		ix = ix -  (*jter).sx + left;
		iy = iy - (*jter).sy + top;

		//std::cerr << "WC FB: " << std::endl;
		Image iImg;
		iImg.prepareFor(
			(int)iw, (int)ih, img->getEncoding());
		drawImageInImage(img, &iImg, -sx, -sy);
		convertImage(&iImg, enc);
		myUpdateTexture(&iImg, ix + (*jter).dx, iy + (*jter).dy, iw, ih);
	}
}

void MetisseWindow::setTextureCoordinates(
	float left, float top, float right, float bottom)
{
	_texCoordsSize = 8;
	if (_texCoords == 0)
	{
		_texCoords = new float [_texCoordsSize] ;
	}
	_texCoords[0] = left ; _texCoords[1] = bottom ;  // bottom left
	_texCoords[2] = right ; _texCoords[3] = bottom ; // bottom right
	_texCoords[4] = right ; _texCoords[5] = top ;    // top right
	_texCoords[6] = left ; _texCoords[7] = top ;     // top left
	_changed = true;
}

void MetisseWindow::getTextureMapping(
	glTiledTexturedImage **texture, int *texCoordsSize, float **texCoords)
{
	*texture = _texture ;
	*texCoordsSize = _texCoordsSize ;
	*texCoords = new float [_texCoordsSize] ;
	memcpy(*texCoords, _texCoords, _texCoordsSize*sizeof(float)) ;
}

// --------------------------------------------------------------

void MetisseWindow::getCurrentTranslation(float *tx, float *ty)
{
	*tx = - (_desktopWidth - _width)/2 + _real_x + rootTransX
		+ _interTransX + _attachedTransX;
	*ty = (_desktopHeight - _height)/2 - _real_y + rootTransY
		+ _interTransY + _attachedTransY;
}

void MetisseWindow::forceCurrentTranslation(float tx, float ty)
{
	_forceX = _real_x;
	_forceY = _real_y;
	translate_rel(- _real_x + tx, - ty +_real_y , 0);
	//setRealPosition(tx, ty);
	_currentTranslationForced = true;
	_changed = true;
}

void MetisseWindow::unforceCurrentTranslation(void)
{
	if (_currentTranslationForced)
	{
		translate_rel(_real_x - _forceX, _forceY -_real_y , 0);
		//setRealPosition(_forceX, _forceY);
		_changed = true;
	}
	_currentTranslationForced = false;
	_changed = true;
}

// --------------------------------------------------------------
#define ROTATE_REL(a,x,y,z) getRenderer()->rotate_rel(a, x, y, z)
//#define ROTATE_REL(a,x,y,z) _ascr->root->rotateWindow(this, a, x, y, z)

void MetisseWindow::saveSurfaceScaleRealPosition(void)
{
	if (_surfaceScale == 1)
	{
		_surfaceScaleSaveX = _real_x;
		_surfaceScaleSaveY = _real_y;
	}
}

bool MetisseWindow::getSurfaceScaleSavedRealPosition(float *x, float *y)
{
	*x = _surfaceScaleSaveX;
	*y = _surfaceScaleSaveY;

	return True;
}

void MetisseWindow::surfaceRotation(bool cancel)
{
	_surfaceRotationAllowed = !cancel;
	if (cancel)
	{
		ROTATE_REL(-_surfaceRotationX, 0, -1, 0);
		ROTATE_REL(-_surfaceRotationY, 1, 0, 0);
		ROTATE_REL(-_surfaceRotationZ, 0, 0, 1);
		if (_surfaceScale != 1)
			getRenderer()->scale(1/_surfaceScale, 1/_surfaceScale,
				1/_surfaceScale);
		_surfaceRotationY = 0;
		_surfaceRotationX = 0;
		_surfaceRotationZ = 0;
		_surfaceScale = 1;
	}
	else
	{
		float tx,ty;
		getCurrentTranslation(&tx, &ty);
		_surfaceCorrection(tx, ty);
	}
}

void MetisseWindow::_resetSurfaceCorrection(void)
{
	if (_surfaceRotationX)
		ROTATE_REL(-_surfaceRotationX, 0, -1, 0);
	if (_surfaceRotationY)
		ROTATE_REL(-_surfaceRotationY, 1, 0, 0);
	if (_surfaceRotationZ)
		ROTATE_REL(-_surfaceRotationZ, 0, 0, 1);
	if (_surfaceScale != 1)
		getRenderer()->scale(
			1/_surfaceScale, 1/_surfaceScale, 1/_surfaceScale);
	_surfaceRotationY = 0;
	_surfaceRotationX = 0;
	_surfaceRotationZ = 0;
	_surfaceScale = 1;
}


void MetisseWindow::surfaceCorrection(void)
{
	float tx,ty;
	getCurrentTranslation(&tx, &ty);
	_surfaceCorrection(tx, ty);
}

void MetisseWindow::_surfaceCorrection(float tx, float ty)
{
	if (isUnmanaged() || isRootWindow() || !_surfaceRotationAllowed)
	{
		return;
	}

	double rx,ry,rz,s;
	_ascr->root->getSurfaceRotation(
		tx, ty, _width, _height, _surfaceScale, &rx, &ry, &rz, &s);

	if (_surfaceRotationX)
		ROTATE_REL(_surfaceRotationX, 0, 1, 0);
	if (_surfaceRotationY)
		ROTATE_REL(_surfaceRotationY, -1, 0, 0);
	if (_surfaceRotationZ)
		ROTATE_REL(_surfaceRotationZ, 0, 0, -1);
	if (_surfaceScale != 1)
		getRenderer()->scale(
			1/_surfaceScale, 1/_surfaceScale, 1/_surfaceScale);

	if (ry)
		ROTATE_REL(ry, 1, 0, 0);
	if (rx)
		ROTATE_REL(rx, 0, -1, 0);
	if (rz)
		ROTATE_REL(rz, 0, 0, 1);
	if (s != 1)
		getRenderer()->scale(s, s, s);

	_surfaceRotationY = ry;
	_surfaceRotationX = rx;
	_surfaceRotationZ = rz;
	_surfaceScale = s;
}

// --------------------------------------------------------------

void MetisseWindow::preDisplay(
	float desktopWidth, float desktopHeight, float eyeDist, float depth)
{
	_desktopWidth = desktopWidth;
	_desktopHeight = desktopHeight;
	_eyeDist = eyeDist;

	
	if (_transientForCorrectionDirty)
	{
		_transientForCorrectionDirty = false;
		//std::cerr << "_transientForCorrection() "  << this <<"\n";
		_transientForCorrection();
	}
	//else
	{
		/* current translation */
		float tx,ty;
		getCurrentTranslation(&tx, &ty);
		_surfaceCorrection(tx, ty);
	}
}

void MetisseWindow::restoreTranslation(void)
{
	_interTransX = 0;
	_interTransY = 0;
	_attachedTransX = 0;
	_attachedTransY = 0;
	_surfaceRotationX = 0;
	_surfaceRotationY = 0;
	_surfaceRotationZ = 0;
	_surfaceScale = 1;
	_absoluteScaleX = 1;
	_absoluteScaleY = 1;
	_absoluteScaleZ = 1;

	float tx,ty;
	getCurrentTranslation(&tx, &ty);

	getRenderer()->translate_rel(tx, ty, 0);
	_surfaceCorrection(tx, ty);
	_attachTransient();
	//_surfaceCorrection(tx, ty);
}

void MetisseWindow::translate_rel(float x, float y, float z)
{
	//std::cerr << "translate_rel" << std::endl;
	_resetSurfaceCorrection();

	float tx,ty;
	getCurrentTranslation(&tx, &ty);

	getRenderer()->translate_rel(-tx, -ty, 0);

	getRenderer()->translate_rel(x, y, 0);

	getRenderer()->translate_rel(tx, ty, 0);

	float ntx = - (_desktopWidth - _width)/2 +  _real_x + x + _interTransX;
	float nty = (_desktopHeight - _height)/2 - (_real_y + y) + _interTransY;
	
	_surfaceCorrection(ntx, nty);
}

void MetisseWindow::_translate_rel2(float x, float y, float z)
{
	float tx,ty;
	getCurrentTranslation(&tx, &ty);

	getRenderer()->translate_rel(-tx, -ty, 0);

	getRenderer()->translate_rel(x, y, 0);

	getRenderer()->translate_rel(tx, ty, 0);
	/*
	float ntx = - (_desktopWidth - _width)/2 +  _real_x + x + _interTransX;
	float nty = (_desktopHeight - _height)/2 - (_real_y + y) + _interTransY;
	*/
}

void MetisseWindow::internalTranslation(float x, float y, float z)
{
	//return;
	_resetSurfaceCorrection();
	_translate_rel2(-_interTransX, -_interTransY, 0);
	x = _interTransX + x;
	y = _interTransY + y;
	_interTransX = _interTransY = 0;
	_translate_rel2(x, y, z);
	_interTransX = x;
	_interTransY = y;
	float ntx = - (_desktopWidth - _width)/2 +  _real_x + _interTransX;
	float nty = (_desktopHeight - _height)/2 - _real_y + _interTransY;
	_surfaceCorrection(ntx, nty);
}

void MetisseWindow::getInternalTranslation(float *x, float *y)
{
	*x = _interTransX;
	*y = _interTransY;
}

void MetisseWindow::mapCorrection(void)
{
	translate_rel(
		rootTransX - _rootTransX, rootTransY - _rootTransY, 0);
}

void MetisseWindow::unmapCorrection(void)
{
	_rootTransX = rootTransX;
	_rootTransY = rootTransY;
}

void MetisseWindow::_transientForCorrection(void)
{

	if (_transientFor != 0)
	{
		#if 0
		std::cerr << "-- Map correction " << _frameID << " " 
			  << _interTransX << " " <<  _interTransY << "\n"
			  << _rrx << " " << _rry << " " << _rwidth << " "
			  << _rheight << "\n"
			  << _real_x << " " << _real_y << " " << _width << " "
			  << _height << "\n"
			  << _texture << std::endl;
		#endif
		internalTranslation(-_interTransX, -_interTransY, 0);
		float tx,ty;
		getCurrentTranslation(&tx, &ty);

		_resetSurfaceCorrection();
		getRenderer()->translate_rel(-tx, -ty, 0);

		_applyTransientTransform();

		getRenderer()->translate_rel(tx, ty, 0);
		_surfaceCorrection(tx, ty);
		_attachTransient();
		#if 0
		std::cerr << "Map correction End " 
			  << _interTransX << " " <<  _interTransY << std::endl;
		#endif
	}
}



void MetisseWindow::absoluteScale(float x, float y, float z)
{
	_resetSurfaceCorrection();
	getRenderer()->scale(
		1/_absoluteScaleX,1/_absoluteScaleY,1/_absoluteScaleZ);
	if (_absoluteScaleX == 0 || _absoluteScaleY == 0 || _absoluteScaleZ == 0)
	{
		_absoluteScaleX = 1;
		_absoluteScaleY = 1;
		_absoluteScaleZ = 1;
	}
	else
	{
		_absoluteScaleX = x;
		_absoluteScaleY = y;
		_absoluteScaleZ = 1;
	}
	getRenderer()->scale(_absoluteScaleX,_absoluteScaleY,_absoluteScaleZ);
	float ntx = - (_desktopWidth - _width)/2 +  _real_x + _interTransX;
	float nty = (_desktopHeight - _height)/2 - _real_y + _interTransY;
	_surfaceCorrection(ntx, nty);
}

void MetisseWindow::getAbsoluteScale(float *x, float *y, float *z)
{
	*x = _absoluteScaleX;
	*y = _absoluteScaleY;
	*z = _absoluteScaleZ;
}

void MetisseWindow::absoluteRotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	getRenderer()->rotate_rel(
		-_absoluteRotateAngle, _absoluteRotateX, _absoluteRotateY,
		_absoluteRotateZ);
	_absoluteRotateAngle = angle;
	_absoluteRotateX = x;
	_absoluteRotateY = y;
	_absoluteRotateZ = z;
	getRenderer()->rotate_rel(
		_absoluteRotateAngle, _absoluteRotateX, _absoluteRotateY,
		_absoluteRotateZ);
}

void  MetisseWindow::resetAbsoluteScale(void)
{
	// we use absolute scale in interactive scale
	_absoluteScaleX = 1;
	_absoluteScaleY = 1;
	_absoluteScaleZ = 1;
}

// --------------------------------------------------------------

void  MetisseWindow::getRendererTransformation(
	GLfloat *trans, bool ignoreInterTrans)
{
	/* current translation */
	float tx,ty;
	if (ignoreInterTrans)
	{
		tx = - (_desktopWidth - _width)/2 + _real_x + rootTransX;
		ty = (_desktopHeight - _height)/2 - _real_y + rootTransY;
	}
	else
	{
		getCurrentTranslation(&tx, &ty);
	}

	//_resetSurfaceCorrection();
	getRenderer()->translate_rel(-tx, -ty, 0);

	//_surfaceCorrection(tx, ty);
	getRenderer()->getTransformation(trans);
	//_resetSurfaceCorrection();

	getRenderer()->translate_rel(tx, ty, 0);

	//_surfaceCorrection(tx, ty);
}

void  MetisseWindow::getRendererFullTransformation(GLfloat *trans)
{
	getRendererTransformation(trans, false);
	trans[16] = _interTransX;
	trans[17] = _interTransY;
}

void  MetisseWindow::setRendererTransformation(
	GLfloat *trans, bool ignoreInterTrans)
{
	/* current translation */
	//internalTranslation(
	//	-_interTransX+_prevInterTransX,
	//	-_interTransY+_prevInterTransY, 0);
	float tx,ty;
	if (ignoreInterTrans)
	{
		tx = - (_desktopWidth - _width)/2 + _real_x + rootTransX;
		ty = (_desktopHeight - _height)/2 - _real_y + rootTransY;
	}
	else
	{
		getCurrentTranslation(&tx, &ty);
	}

	//_resetSurfaceCorrection();
	getRenderer()->translate_rel(-tx, -ty, 0);

	//_surfaceCorrection(tx, ty);
	getRenderer()->setTransformation(trans);
	//_resetSurfaceCorrection();

	if (!ignoreInterTrans)
	{
		_interTransX = trans[16];
		_interTransY = trans[17];
		getCurrentTranslation(&tx, &ty);
	}

	getRenderer()->translate_rel(tx, ty, 0);

	//_surfaceCorrection(tx, ty);
}

void  MetisseWindow::setRendererFullTransformation(GLfloat *trans)
{
	setRendererTransformation(trans, false);
}

// --------------------------------------------------------------
// forward to fvwmmodule via ascr

void MetisseWindow::sendMetisseTransform(bool mark)
{
	_ascr->sendMetisseTransform(this, mark);
}

// --------------------------------------------------------------

std::list<WinRegions::region > MetisseWindow::getSelectedRegions(void)
{
	std::list<WinRegions::region > regions =_renderer->getSelectedRegions();
	std::list<WinRegions::region >::iterator i;
	//std::cerr << "getSelectedRegions " <<  regions.size() << std::endl;
	for (i=regions.begin(); i!= regions.end(); ++i)
	{
		(*i).x = (*i).x + _width/2.0;
		(*i).y = - (*i).y + _height/2.0;
	}

	return regions;
}

// --------------------------------------------------------------
bool MetisseWindow::getCrossPosition(double *x, double *y)
{
	if (!_sb_cross)
	{
		return false;
	}
	*x = 0;
	*y = 0;
	return true;
}

sgNode *MetisseWindow::getFacadeAtPoint(int x, int y, cut *fcut)
{
	FacadeMapping::iterator iter;
	for (iter = _facade.begin(); iter != _facade.end(); iter++)
	{
		MetisseWindow *dw = dynamic_cast<MetisseWindow *>((*iter).first);
		float drx,dry;
		dw->getRealPosition(&drx, &dry);
		cutlist cl = (*iter).second;
		cutlist::iterator jter;
		int decorX = (int)(x + _width/2.0);
		int decorY = (int)(-y + _height/2.0);
		int top = 0, left = 0, right = 0, bottom = 0;
		if (isAFacadeWindow())
		{
			top = _decorTop;
			left = _decorLeft;
			right = _decorRight;
			bottom = _decorBottom;
		}
		for (jter = cl.begin(); jter != cl.end(); jter++)
		{
			if (decorX > (*jter).dx + left &&
			    decorX <= (*jter).dx + (*jter).sw + left &&
			    decorY > (*jter).dy + top &&
			    decorY <= (*jter).dy + (*jter).sh + top)
			{
				*fcut = (*jter);
				return dw;
			}
		}
	}
	return 0;
}

bool MetisseWindow::isOverDecor(int x, int y)
{
	cut fcut;
	if (getFacadeAtPoint(x,y,&fcut) != 0)
	{
		return false;
	}

	int decorX = (int)(x + _width/2.0);
	int decorY = (int)(-y + _height/2.0);
	if (decorX > _decorLeft && decorX < _width - _decorRight &&
	    decorY > _decorTop && decorY < _height - _decorBottom)
	{
		return false;
	}

	return true;
}

void MetisseWindow::keyEvent(unsigned long key, bool down_flag)
{
	//std::cerr << "key: " << key << " flag: " << down_flag << std::endl ;
	_metisseSource->keyEvent(key, down_flag) ;
}

void MetisseWindow::pointerEvent(float x, float y, unsigned long button_mask)
{
  //  std::cerr << "MetisseWindow::pointerEvent: " << x << "," << y << ":" << button_mask << std::endl ;
#if 0
 	int realx = (int)(x + _width/2.0 + _real_x);
	int realy = (int)(-y + _height/2.0 + _real_y) - 1;
#else
	int realx = _ascr->obc2x11scX(x, _width/2.0, _real_x);
	int realy = _ascr->obc2x11scY(y, _height/2.0, _real_y);
#endif

	// FIXME!
	x = (int)x;
	y = (int)y;
#if 0
	if (realx < _real_x || realx > _real_x + _width ||
	     realy < _real_y || realy > _real_y + _height)
	  {
		  // OUTCH
		  std::cerr << button_mask << " : " << x << "," << y <<
			  " - " << realx << "," << realy << std::endl ;
	  }
#endif

	bool sendToDuplicate = false;

	if (_duplicateFor)
	{
		int decorX = (int)(x + _width/2.0);
		int decorY = (int)(-y + _height/2.0);
		if (decorX > _decorLeft && decorX < _width - _decorRight &&
		    decorY > _decorTop && decorY < _height - _decorBottom)
		{
			sendToDuplicate = true;
		}
	}
	if (sendToDuplicate)
	{
		MetisseWindow *dw = dynamic_cast<MetisseWindow *>(_duplicateFor);
		float drx,dry;
		dw->getRealPosition(&drx, &dry);
		int drealx = (int)(x + _width/2.0 + drx);
		int drealy = (int)(-y + _height/2.0 + dry) - 1;
		_metisseSource->pointerEvent(
			dw->getFrameID(), drealx, drealy, button_mask);
		return;
	}

	FacadeMapping::iterator iter;
	for (iter = _facade.begin(); iter != _facade.end(); iter++)
	{
		MetisseWindow *dw = dynamic_cast<MetisseWindow *>((*iter).first);
		float drx,dry;
		dw->getRealPosition(&drx, &dry);
		cutlist cl = (*iter).second;
		cutlist::iterator jter;
		int decorX = (int)(x + _width/2.0);
		int decorY = (int)(-y + _height/2.0);
		int top = 0, left = 0, right = 0, bottom = 0;
		if (isAFacadeWindow())
		{
			top = _decorTop;
			left = _decorLeft;
			right = _decorRight;
			bottom = _decorBottom;
		}
		for (jter = cl.begin(); jter != cl.end(); jter++)
		{
			if (decorX > (*jter).dx + left &&
			    decorX <= (*jter).dx + (*jter).sw + left &&
			    decorY > (*jter).dy + top &&
			    decorY <= (*jter).dy + (*jter).sh + top)
			{
				float drx,dry;
				dw->getRealPosition(&drx, &dry);
				int drealx =
					(int)(x + _width/2.0 + drx +
					      (*jter).sx - left);
				int drealy =
					(int)(-y + _height/2.0 + dry +
					      (*jter).sy - top) - 1;
				_metisseSource->pointerEvent(
					dw->getFrameID(),
					drealx - (*jter).dx,
					drealy - (*jter).dy, button_mask);
				return;
			}
		}
	}

#ifdef  HAVE_SPI
	// FIXME: clean_up this!!
	vallist::iterator vter;
	for (vter =  _widgetValuators.begin(); vter != _widgetValuators.end();
	     vter++)
	{
		AWidgetValuator *widget_val = (*vter);
		AWidgetOrthoZoom *widget_oz = 0;

		if (widget_val->getSpeed() == -2)
		{
			widget_oz = dynamic_cast<AWidgetOrthoZoom *>(widget_val);
		}
		double cur, min, max, v_per_p, diff, newv, start_diff;
		if (_prev_button_mask == 0 && button_mask != 0)
		{
			// get a button press
			if (_sb_relaction >= 2)
			{
				// end press
				_sb_relaction = 3;
				return;
			}
			else if (widget_val->getExtents(
				    &_sb_x, &_sb_y, &_sb_w, &_sb_h) &&
			    realx >= _sb_x && realx < _sb_x + _sb_w &&
			    realy >= _sb_y && realy < _sb_y + _sb_h)
			{
				widget_val->setInaction(true);
				if (button_mask == 2)
				{
					// start press
					_sb_relaction = 1;
				}
				else
				{
				}
				_start_button_mask = button_mask;
				_prev_button_mask = button_mask;
				_prev_x = realx;
				_prev_y = realy;
				_start_x = realx;
				_start_y = realy;
				_some_action_done = false;
				if (widget_oz != 0)
				{
					_sb_cross = true;
					getRenderer()->postRedisplay();
					widget_oz->start();
				}
				return; // yes!
			}
			else
			{
				widget_val->setInaction(false);
				_sb_relaction = 0;
				_sb_cross = false;
				_some_action_done = false;
			}
		}
		else if (widget_val->getInaction() &&
			 ((_sb_relaction == 2 && button_mask == 0)
			  || (_sb_relaction < 2 && _prev_button_mask != 0 &&
			      _prev_button_mask == button_mask)) &&
			 widget_val->getValues(&cur, &min, &max))
		{
			// dragging do something!
			#if 0
			std::cerr << " \n";
			std::cerr << "w: " << _sb_w  << " h: " << _sb_h
				  << " cur: " << cur << " min: " << min
				  << " max: " << max << "\n";
			#endif
			// FIXME
			if (_sb_h > _sb_w)
			{
				// vertical
				v_per_p = (max - min)/(double)(_sb_h);
				diff = realy - _prev_y;
				start_diff = realx - _start_x;
			}
			else
			{
				// horizontal
				v_per_p = (max - min)/(double)(_sb_w);
				diff = realx - _prev_x;
				start_diff = realy - _start_y;
			}
			//std::cerr <<" d: "<< diff <<" vp: "<< v_per_p<< "\n";

			double speed = widget_val->getSpeed();

			// orthozoom
			if (widget_oz != 0)
			{
				if (start_diff <= 0)
				{
					bool rc =
						widget_oz->doaction(
							(int)start_diff);
					_prev_button_mask = button_mask;
					_prev_x = realx;
					_prev_y = realy;
					speed = 1;
					if (rc)
					{
						_some_action_done = true;
						return;
					}
					diff = -diff;
				}
				else
				{
					diff = -diff;
					start_diff = - start_diff;
					speed = 0;
				}
			}
			// Automatic speed!
			// FIXME: do a better job?

			double tmp = fmin(fabs(start_diff),50);
			if (speed == 0 && start_diff > 5)
			{
				//speed = 1 + tmp/10;
				speed = 0.5 - tmp/101;
			}
			else if (speed == 0 && start_diff < -5)
			{
				speed = 0.5 - tmp/101;
			}
			else if (speed == 0)
			{
				speed = 1;
			}

			newv = cur + diff*speed*v_per_p;
			//std::cerr << "NEW: " << newv << "\n";
			if (newv > max)
			{
				newv = max;
			}
			else if (newv < min)
			{
				newv = min;
			}
			_some_action_done = true;
			widget_val->setValue(newv);
			_prev_button_mask = button_mask;
			_prev_x = realx;
			_prev_y = realy;
			return; //!
		}
		else if (widget_val->getInaction())
		{
			if (_sb_relaction == 1)
			{
				// start release
				_sb_relaction = 2;
				// FIXME: should forbid rest-selection!!
				return;
			}
			_sb_relaction = 0;
			if (_sb_cross)
			{
				_sb_cross = false;
				getRenderer()->postRedisplay();
			}
			// button released
			widget_val->setInaction(false);
			_prev_button_mask = button_mask;
			_prev_x = realx;
			_prev_y = realy;
			if (widget_oz != 0)
			{
				widget_oz->end();
			}
			if (!_some_action_done)
			{
				_metisseSource->pointerEvent(
					getFrameID(), realx, realy,
					_start_button_mask);
				usleep(400);
			}
			else
			{
				return;
			}
		}
		else
		{
			widget_val->setInaction(false);
		}
	}
	_prev_button_mask = button_mask;
	_prev_x = realx;
	_prev_y = realy;
#endif

	_metisseSource->pointerEvent(
		getFrameID(), realx, realy, button_mask);

}
