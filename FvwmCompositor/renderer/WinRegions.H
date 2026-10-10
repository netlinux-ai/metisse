#ifndef a_WinRegions_H
#define a_WinRegions_H

#include <list>
#include <cmath>

class WinRegions
{

public:

struct region
{
	double x,y,w,h ;
	region(double a, double b, double c, double d) :
		x(a), y(b), w(c), h(d) {}
	region(void) { region(0.0,0.0,0.0,0.0); }
      
};

struct xyregion
{
	double x1,y1,x2,y2 ;
	xyregion(double a, double b, double c, double d) :
		x1(a), y1(b), x2(c), y2(d) {}
	xyregion(void) { region(0.0,0.0,0.0,0.0); }
      
};
  

protected:

std::list<WinRegions::xyregion > _regions;
xyregion _currentXYRegion;
int _idCount;

public:

WinRegions()
{
	clear() ;
	//_currentXYRegion = WinRegions::region(0.0, 0.0, 0.0, 0.0);
	_idCount = 0;
};

std::list<WinRegions::xyregion > getNormalizedXYRegions()
{
	std::list<WinRegions::xyregion >::iterator i;
	std::list<WinRegions::xyregion > c;

	for (i=_regions.begin(); i!= _regions.end(); ++i)
	{
		double x1,y1,x2,y2;
		x1 = std::min((*i).x1,(*i).x2);
		y1 = std::max((*i).y1,(*i).y2);
		x2 = std::max((*i).x1,(*i).x2);
		y2 = std::min((*i).y1,(*i).y2);
		c.push_front(xyregion(x1, y1, x2, y2));
	}
	if (_currentXYRegion.x1 != _currentXYRegion.x2 &&
	    _currentXYRegion.y1 != _currentXYRegion.y2)
	{
		double x1,y1,x2,y2;
		x1 = std::min(_currentXYRegion.x1,_currentXYRegion.x2);
		y1 = std::max(_currentXYRegion.y1,_currentXYRegion.y2);
		x2 = std::max(_currentXYRegion.x1,_currentXYRegion.x2);
		y2 = std::min(_currentXYRegion.y1,_currentXYRegion.y2);
		c.push_front(xyregion(x1,y1,x2,y2));
	}
	return c;
};

std::list<WinRegions::region > getNormalizedRegions()
{
	std::list<WinRegions::xyregion >::iterator i;
	std::list<WinRegions::region > c;

	for (i=_regions.begin(); i!= _regions.end(); ++i)
	{
		double x1,y1,x2,y2;
		x1 = std::min((*i).x1,(*i).x2);
		y1 = std::max((*i).y1,(*i).y2);
		x2 = std::max((*i).x1,(*i).x2);
		y2 = std::min((*i).y1,(*i).y2);
		if (x2 != x1 && y2 != y1)
		{
			c.push_front(region(x1, y1, x2 - x1, y1 - y2));
		}
	}
	if (_currentXYRegion.x1 != _currentXYRegion.x2 &&
	    _currentXYRegion.y1 != _currentXYRegion.y2)
	{
		double x1,y1,x2,y2;
		x1 = std::min(_currentXYRegion.x1,_currentXYRegion.x2);
		y1 = std::max(_currentXYRegion.y1,_currentXYRegion.y2);
		x2 = std::max(_currentXYRegion.x1,_currentXYRegion.x2);
		y2 = std::min(_currentXYRegion.y1,_currentXYRegion.y2);
		c.push_front(region(x1,y1,x2-x1,y1-y2));
	}
	return c;
};

void clear(void)
{
	_regions.clear() ;
	_currentXYRegion = xyregion(0.0,0.0,0.0,0.0);
	_idCount = 0;
}

void rmMatchingRegion(WinRegions::region *reg)
{
	std::list<WinRegions::xyregion > oldReg;
	std::list<WinRegions::xyregion >::iterator i;

	for (i=_regions.begin(); i!= _regions.end(); ++i)
	{
		oldReg.push_front((*i));
	}

	_regions.clear() ;
	_idCount = 0;
	bool found = false;

	for (i=oldReg.begin(); i!= oldReg.end(); ++i)
	{
		double x1,y1,x2,y2;
		x1 = std::min((*i).x1,(*i).x2);
		y1 = std::max((*i).y1,(*i).y2);
		x2 = std::max((*i).x1,(*i).x2);
		y2 = std::min((*i).y1,(*i).y2);

		if (!found &&
		    (int)x1 == (int)reg->x &&
		    (int)y1 == (int)reg->y &&
		    (int)(x2 - x1) == (int)reg->w &&
		    (int)(y1 - y2) == (int)reg->h)
		{
			found = true;
			continue;
		}
		_regions.push_front((*i));
		_idCount++;
	}
}

int size(void)
{
  return _regions.size();
}

void storeCurrent(void)
{
	if (_currentXYRegion.x1 != _currentXYRegion.x2 &&
	    _currentXYRegion.y1 != _currentXYRegion.y2)
	{
		_regions.push_front(_currentXYRegion);
		_idCount++;
	}
	_currentXYRegion = xyregion(0.0,0.0,0.0,0.0);
}

xyregion getCurrentXYRegion(void)
{
	return _currentXYRegion;
}

void updateCurrentXYRegion(double x1, double y1, double x2, double y2)
{
	_currentXYRegion.x1 = x1;
	_currentXYRegion.y1 = y1;
	_currentXYRegion.x2 = x2;
	_currentXYRegion.y2 = y2;
}

};

#endif
