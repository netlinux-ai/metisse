/*
 *
 * nucleo/gl/texture/glTiledTexturedImage.cxx --
 *
 * Copyright (C) Nicolas Roussel, Olivier Chapuis
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Some codes inspired by "manuscript" by Jean-Daniel Fekete
 *
*/

#include "config.h"

#include "texture/glTiledTexturedImage.H"

#include <nucleo/image/Image.H>
#include <nucleo/image/encoding/Conversion.H>
#include <nucleo/utils/ByteOrder.H>
#include <algorithm>
#include <cstring>

using namespace std;

/* Mac OS X notes:
 *
 *    - "OpenGL Image" sample code from Apple says that the
 *      GL_TEXTURE_RECTANGLE_EXT only supports even texels per row
 *
 *    - should check maximum size...
 *      Done, but it will be better to use GL_PROXY_TEXTURE_2D
 */

bool glTiledTexturedImage::debugMode = false ;
bool glTiledTexturedImage::use_npot_ext = true ;

GLint glTiledTexturedImage::_potMaxTextureSize = 0;
GLint glTiledTexturedImage::_npotNVMaxTextureSize = 0;
GLint glTiledTexturedImage::_npotEXTMaxTextureSize = 0;

GLUtesselator* glTiledTexturedImage::_tessellator = 0;

GLint glTiledTexturedImage::_can_generate_mipmaps = -1;
bool glTiledTexturedImage::do_generate_mipmaps  = false ;

//----------------------------------------------------------------------
// utils functions

static
bool glExtensionIsSupported(const char *extension)
{
	int extlen = strlen(extension) ;

	char *p = (char *)glGetString(GL_EXTENSIONS) ;
	if (!p) return false ;

	char *end = p+strlen(p) ;
	while (p<end)
	{
		int n = strcspn(p," ") ;
		if ((extlen==n) && (!strncmp(extension,p,n))) return true ;
		p +=  n+1 ;
	}

	return false ;
}

static
bool getImageEncodingParameters(
	Image::Encoding e, GLenum *format, GLint *internalformat,
	GLint *alignment, GLenum *type)
{
	switch (e)
	{
	case Image::L:
		*format = *internalformat = GL_LUMINANCE ;
		*alignment = 1 ;
		*type = GL_UNSIGNED_BYTE ;
		break ;
	case Image::A:
		*format = *internalformat = GL_ALPHA ;
		*alignment = 1 ;
		*type = GL_UNSIGNED_BYTE ;
		break ;
	case Image::RGB:
		*format = GL_RGB ;
		*internalformat = GL_RGB;
		*alignment = 1;
		*type = GL_UNSIGNED_BYTE ;
		break;
	case Image::RGB565:
		*format = GL_RGB ;
		*internalformat = GL_RGB;
		*alignment = 1;
		*type = GL_UNSIGNED_SHORT_5_6_5;
		break ;
	case Image::RGBA:
		*format = GL_RGBA ;
		*internalformat = GL_RGBA ;
		*type = GL_UNSIGNED_BYTE ;
		*alignment = 1 ;
		break ;
	case Image::ARGB:
		*format = GL_BGRA ;
		*internalformat = GL_RGBA ;
		*alignment = 4 ;
		*type = ByteOrder::isLittleEndian() ? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_INT_8_8_8_8_REV ;
		break ;
	default:
		return false ;
	}
	return true ;
}

static
void setupTextureImage(
	Image *img, GLuint texture_target, bool useTexSub, GLint xoffset,
	GLint yoffset, GLsizei width, GLsizei height)
{
	GLenum format ;
	GLint internalformat ;
	GLint alignment ;
	GLenum type ;


      if(!img->getSize())
      {
	      // should not happen
	      std::cerr << "ERROR: setupTextureImage has been called "
			<< "with an empty Image" << std::endl;
	      return;
      }

	getImageEncodingParameters(
		img->getEncoding(), &format, &internalformat, &alignment, &type);

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment) ;
	if (useTexSub)
	{
		if (!xoffset && !yoffset && !width && !height)
		{
			xoffset = yoffset = 0 ;
			width = img->getWidth() ;
			height = img->getHeight() ;
		}
		glTexSubImage2D(
			texture_target,
			0, // level
			xoffset, yoffset, width, height,
			format, type,
			img->getData()) ;
	}
	else
	{
		if (width == 0) width = img->getWidth();
		if (height == 0) height = img->getHeight();
		glTexImage2D(
			texture_target,
			0, // level
			internalformat,
			width, height,
			0, // border
			format, type,
			img->getData()) ;
	}
}

static
void setupDummyTextureImage(
	Image::Encoding enc, GLuint texture_target, GLsizei width,
	GLsizei height)
{
	GLenum format ;
	GLint internalformat ;
	GLint alignment ;
	GLenum type ;
	getImageEncodingParameters(
		enc, &format, &internalformat, &alignment, &type) ;

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment) ;
	glTexImage2D(
		texture_target,
		0, // level
		internalformat,
		width, height,
		0, // border
		format, type,
		NULL) ;
}

//----------------------------------------------------------------------

void
glTiledTexturedImage::_init(void)
{
	if (_target == GL_TEXTURE_2D)
	{
		if (_potMaxTextureSize)
		{
			_maxTextureSize = _potMaxTextureSize;
			return;
		}
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_potMaxTextureSize);
#if 0
		std::cerr << "glTiledTexturedImage uses GL_TEXTURE_2D"
			  << std::endl << "  Max texture size: "
			  << _potMaxTextureSize
			  << std::endl;
#endif
		_maxTextureSize = _potMaxTextureSize;
		
		if (_can_generate_mipmaps==-1)
		{
			_can_generate_mipmaps =
				glExtensionIsSupported(
					"GL_SGIS_generate_mipmap");
			if (0 && _can_generate_mipmaps)
			{
				std::cerr << "GL_SGIS_generate_mipmap is "
					"supported" << std::endl;
			}
		}
	}
#ifdef GL_TEXTURE_RECTANGLE_NV
	else if (_target == GL_TEXTURE_RECTANGLE_NV)
	{
		if (_npotNVMaxTextureSize)
		{
			_maxTextureSize = _npotNVMaxTextureSize;
			return;
		}
		glGetIntegerv(
			GL_MAX_RECTANGLE_TEXTURE_SIZE_NV,
			&_npotNVMaxTextureSize);
#if 0
		std::cerr << "glTiledTexturedImage uses " 
			  << "GL_TEXTURE_RECTANGLE_NV"
			  << std::endl << "  Max texture size: "
			  << _npotNVMaxTextureSize << std::endl;
#endif
		_maxTextureSize = _npotNVMaxTextureSize;
	}
#endif
#ifdef GL_TEXTURE_RECTANGLE_EXT
	else if (_target == GL_TEXTURE_RECTANGLE_EXT)
	{
		if (_npotEXTMaxTextureSize)
		{
			_maxTextureSize = _npotEXTMaxTextureSize;
		}
		glGetIntegerv(
			GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT,
			&_npotEXTMaxTextureSize);
#if 0
		std::cerr << "glTiledTexturedImage uses " 
			  << "GL_TEXTURE_RECTANGLE_EXT"
			  << std::endl <<  "  Max texture size: "
			  << _npotEXTMaxTextureSize << std::endl;
#endif
		_maxTextureSize = _npotEXTMaxTextureSize;
	}
#endif

}

// ---------------------------------------------------------------------

glTiledTexturedImage::glTiledTexturedImage(int size) {

	_target = GL_TEXTURE_2D ;

#ifdef GL_TEXTURE_RECTANGLE_NV
	if (use_npot_ext && glExtensionIsSupported("GL_NV_texture_rectangle"))
	{
		if (debugMode)
		{
			std::cerr << "glTiledTexturedImage: target for "
				  << this << " is GL_TEXTURE_RECTANGLE_NV"
				  << std::endl ;
		}
		_target = GL_TEXTURE_RECTANGLE_NV ;
	}
#endif

#ifdef GL_TEXTURE_RECTANGLE_EXT
	if (use_npot_ext && glExtensionIsSupported("GL_EXT_texture_rectangle"))
	{
		 if (debugMode)
		 {
			 std::cerr << "glTiledTexturedImage: target for "
				   << this << " is GL_TEXTURE_RECTANGLE_EXT"
				   << std::endl;
		 }
		 _target = GL_TEXTURE_RECTANGLE_EXT;
	}
#endif

	// setup _maxTextureSize for the _target
	_init();

	_minTextureSize = 32;
	//size = 0;
	if (size > 0)
	{
		if (size < _minTextureSize)
		{
			_maxTextureSize = _minTextureSize;
		}
		if (_target == GL_TEXTURE_2D)
		{
			// use a pot size
			unsigned int tmp = 32;
			while (tmp < size) tmp *=2;
			if (tmp <= _maxTextureSize)
			{
				_maxTextureSize = tmp;
			}
		}
	}

	 _tex_per_col = 0;
	 _tex_per_row = 0;
	 _texture_count = 0;
}

glTiledTexturedImage::~glTiledTexturedImage()
{
	int i;
	GLuint tn;

	glFinish() ;
	for(i = 0; i < _texture_count; i++)
	{
		tn = _texSet[i].getTexnum();
		glDeleteTextures(1, &tn) ;
	}
}

// ------------------------------------------------------------------------

#define CHECK_IMAGE_ERROR           0
#define CHECK_IMAGE_INDIRECT_SOURCE 1
#define CHECK_IMAGE_OK              2

int
glTiledTexturedImage::_checkImage(Image *img)
{
	int rc = CHECK_IMAGE_OK;
	Image::Encoding encoding = img->getEncoding() ;
    
	switch (encoding)
	{
	case Image::RGB:
	case Image::ARGB:
	case Image::RGBA:
	case Image::RGB565:	
		break ;
	default:
	  std::cerr << "glTiledTexturedImage::_checkImage: ERROR unknown"
		    << " image encoding"
		    << std::endl;
	  return CHECK_IMAGE_ERROR;
	  break ;
	}
	
	return rc;
}

bool
glTiledTexturedImage::subUpdate(
	Image *img, int x, int y, unsigned int w, unsigned int h)
{
	int rc = _checkImage(img);

	if (rc == CHECK_IMAGE_ERROR)
	{
		return false;
	}
	else if  (rc == CHECK_IMAGE_INDIRECT_SOURCE)
	{
		img = &_indirect_source ;
	}

	glEnable(_target);

	int tx = 0, ty = 0;
	unsigned int tw = 0, th = 0;
	uTex utex;

	int row, col;
	for (row = 0; row < _tex_per_row; row++)
	{
		for (col = 0; col < _tex_per_col; col++)
		{
			utex = _texSet[row * _tex_per_col + col];
			tw = utex.getWidth();
			th = utex.getHeight();

			int x1 = max(x, tx);
			int y1 = max(y, ty);
			int x2 = min(x + w, tx + tw);
			int y2 = min(y + h, ty + th);

			if (x2 <= x1 || y2 <= y1)
			{
				// WARNING!
				tx += utex.getWidth();
				continue;
			}
	
			int w1 = x2 - x1;
			int h1 = y2 - y1;

			//glBindTexture(_target, utex.getTexnum());
			_bind(utex.getTexnum(), tw, th);

			//glPixelStorei(GL_UNPACK_ALIGNMENT, 1) ;
			glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, x1 - x);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, y1 - y);

			setupTextureImage(
				img, _target, true, x1-tx, y1-ty, w1, h1);

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			tx += utex.getWidth();
		}
		tx = 0;
		ty += utex.getHeight();
	}

	glDisable(_target) ;

	return true ;
}

bool
glTiledTexturedImage::update(Image *img)
{
	int rc = _checkImage(img);

	if (rc == CHECK_IMAGE_ERROR)
	{
		return false;
	}
	else if  (rc == CHECK_IMAGE_INDIRECT_SOURCE)
	{
		img = &_indirect_source ;
	}

	_oimageWidth = img->getWidth();
	_oimageHeight= img->getHeight() ;
	_tiledTessellation.clear();

	int col = 0;
	int row = 0;
	long int w = _oimageWidth;
	long int h = _oimageHeight;
	int count;

	while (w > 0)
	{
		col++;
		w -= _maxTextureSize;
	}
	while (h > 0)
	{
		row++;
		h -= _maxTextureSize;
	}
	
	count = col*row;

	if (count == 0)
	{
		return false;
	}
	if (count > MAX_NBR_OF_UTEXTURE)
	{
		std::cerr << "nucleo: Texture to big!" << std::endl;
		return false;
	}

	_textureImage.stealDataFrom(*img) ;

	glEnable(_target) ;

	if (_texture_count > count)
	{
		int i;
		GLuint tn;

		glFinish() ;
		for(i = count; i < _texture_count; i++)
		{
			tn = _texSet[i].getTexnum();
			glDeleteTextures(1, &tn) ;
		}
	}
	else if (_texture_count < count)
	{
		int i;
		GLuint tn;
		for(i = _texture_count; i < count; i++)
		{
			glGenTextures(1, &tn) ;
			_texSet[i].setTexnum(tn);
		}
	}

	_texture_count = count;
	_tex_per_col = col;
	_tex_per_row = row;

	int lc;
	int prevW = 0, prevH = 0;

	for (row = 0; row < _tex_per_row; row++)
	{
		int rh,rw;

		if (row <  _tex_per_row - 1)
		{
			rh = h = _maxTextureSize;
		}
		else if (_target == GL_TEXTURE_2D)
		{
			rh = _oimageHeight - _maxTextureSize*row;
			h = _minTextureSize;
			while (h < rh) h *= 2;
		}
		else
		{
			rh = h = _oimageHeight - _maxTextureSize*row;
		}

		for (col = 0; col < _tex_per_col; col++)
		{
			if (col <  _tex_per_col - 1)
			{
				rw = w = _maxTextureSize;
			}
			else if (_target == GL_TEXTURE_2D)
			{
				rw = _oimageWidth - _maxTextureSize*col;
				w = _minTextureSize;
				while (w < rw) w *= 2;
			}
			else
			{
				rw = w = _oimageWidth - _maxTextureSize*col;
			}

			lc = row * _tex_per_col + col;
			_texSet[lc].setSize(w, h);
		
			//glBindTexture(_target, _texSet[lc].getTexnum());
			_bind(_texSet[lc].getTexnum(), w, h);

			//glPixelStorei(GL_UNPACK_ALIGNMENT, 1) ;
			glPixelStorei(
				GL_UNPACK_ROW_LENGTH, _textureImage.getWidth());
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, prevW);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, prevH);
			if (rw < w || rh < h)
			{
				// setup the texture with a null image, here
				// target must be TEXTURE_2D
				setupDummyTextureImage(
					_textureImage.getEncoding(), _target,
					w, h);
				// then draw on the texture
				setupTextureImage(
					&_textureImage, _target, true,
					0, 0, rw, rh);
			}
			else
			{
				setupTextureImage(
					&_textureImage, _target, false,
					0, 0, w, h);
			}

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			prevW += w;
		}
		prevW = 0;
		prevH += h;
	}

	glDisable(_target) ;

	// ----------------------------------------
	
	return true ;
}

// ------------------------------------------------------------------------

void
glTiledTexturedImage::enable(void)
{
	glEnable(_target) ;
	// glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void
glTiledTexturedImage::disable(void)
{
	glDisable(_target) ;
	if (_target != GL_TEXTURE_2D)
	{
		glMatrixMode(GL_TEXTURE) ;
		glLoadIdentity() ;
		glMatrixMode(GL_MODELVIEW) ;
	}
}


// ------------------------------------------------------------------------
// rendering helpers functions

void glTiledTexturedImage::_bind(GLuint texture, GLdouble w, GLdouble h)
{
	glBindTexture(_target, texture);

	glTexParameteri(
		_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(
		_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if GL_SGIS_generate_mipmap
	if (_target == GL_TEXTURE_2D && _can_generate_mipmaps &&
	    do_generate_mipmaps)
	{
		glTexParameteri(
			_target, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(
			_target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		glTexParameteri(
			_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
#endif
#if 1
	{
		// This must be done at each bind!
		glTexParameteri(
			_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(
			_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// ugly
#else
	{
		glTexParameteri(
			_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(
			_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
#endif

	if (_target!=GL_TEXTURE_2D)
	{
		glMatrixMode(GL_TEXTURE) ;
		glLoadIdentity() ;
		glScalef(w, h, 1.0) ;
		glMatrixMode(GL_MODELVIEW) ;
	}
}

void glTiledTexturedImage::_getRenderingParameters(
	int col, int row, int x, int y,
	GLdouble *w, GLdouble *h, GLdouble *right, GLdouble *bottom)
{
	uTex utex = _texSet[row * _tex_per_col + col];
			
	if (x + utex.getWidth() <= _oimageWidth)
	{
		*w = utex.getWidth();
		*right = 1.0;
	}
	else
	{
		*w = _oimageWidth - x;
		*right = *w/utex.getWidth();
	}
	if (y + utex.getHeight() <= _oimageHeight)
	{
		*h = utex.getHeight();
		*bottom = 1.0;
	}
	else
	{
		*h = (_oimageHeight - y);
		*bottom = *h/utex.getHeight();
	}
}

// ------------------------------------------------------------------------
// tessellator stuff

#ifdef __APPLE__

#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_5)
#define MY_GLU_CALLBACK GLvoid (*) ()
#else
#define MY_GLU_CALLBACK GLvoid (*) (...)
#endif

#else
#define MY_GLU_CALLBACK GLvoid (*) ()
#endif

// for storing the tessellation
static glTiledTexturedImage::utess currentuTess;
static glTiledTexturedImage::tessellation currentTessellation;
static glTiledTexturedImage::tiledTessellation currentTiledTessellation;
static bool saveTessellation = false;
 
static GLvoid
_tessVertexCallback(void *vetex_data, void *user_data)
{
	GLdouble *v = (GLdouble *)vetex_data;
	GLdouble *t = (GLdouble *)user_data;

	glTexCoord2d( (v[0] - t[0])/t[2], (-v[1] + t[1])/t[3]);
	glVertex2d(v[0], v[1]);
	if (saveTessellation)
	{
		currentuTess.points.push_back(
			glTiledTexturedImage::point(v[0],v[1]));
	}
}

static GLvoid
_tessBeginCallback(GLenum type, void *user_data)
{
	glBegin(type);
	if (saveTessellation)
	{
		currentuTess.type = type;
		currentuTess.points.clear();
	}
}

static GLvoid
_tessEndCallback(void *user_data)
{
	glEnd();
	if (saveTessellation)
	{
		currentTessellation.push_back(currentuTess);
	}
}

static GLvoid
_tessErrorCallback(GLenum errno, void *user_data)
{
	std::cerr << "[nucleo::glTiledTexturedImage] Tessellator Error: "
		  <<  std::hex << errno << std::dec
		  << " (" << gluErrorString(errno) << ")" << std::endl;
}

static GLvoid
_tessCombineCallback(
	GLdouble coords[3], GLdouble *vetex_data[4], GLfloat weight[4],
	void **outData, void *user_data)
{
	GLdouble *vertex;
	int i;

	vertex = (GLdouble *)malloc(3 * sizeof(GLdouble));
	vertex[0] =  coords[0];
	vertex[1] =  coords[1];
	vertex[2] =  coords[2];
	
	*outData = vertex;
}

static GLvoid
_tessEdgeFlagCallback(GLboolean flag, void *user_data)
{
	//std::cerr << "_tessEdgeFlagCallback " << flag << std::endl;
}


void
glTiledTexturedImage::_tessInit(void)
{
	if (_tessellator != 0)
	{
		return;
	}

	_tessellator = gluNewTess();

	gluTessCallback(_tessellator, GLU_TESS_VERTEX_DATA,
				 (MY_GLU_CALLBACK)_tessVertexCallback);
	gluTessCallback(_tessellator, GLU_TESS_BEGIN_DATA,
				 (MY_GLU_CALLBACK)_tessBeginCallback);
	gluTessCallback(_tessellator, GLU_TESS_BEGIN,
				 (MY_GLU_CALLBACK)glBegin);
	gluTessCallback(_tessellator, GLU_TESS_END_DATA,
				 (MY_GLU_CALLBACK)_tessEndCallback);
	gluTessCallback(_tessellator, GLU_TESS_ERROR_DATA,
				 (MY_GLU_CALLBACK)_tessErrorCallback);
	gluTessCallback(_tessellator, GLU_TESS_COMBINE_DATA,
				 (MY_GLU_CALLBACK)_tessCombineCallback);
	gluTessCallback(_tessellator, GLU_TESS_EDGE_FLAG_DATA,
				 (MY_GLU_CALLBACK)_tessEdgeFlagCallback);

	// properties
	gluTessProperty(_tessellator, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
	gluTessProperty(_tessellator, GLU_TESS_TOLERANCE, 0); // default 0
	gluTessProperty(_tessellator, GLU_TESS_WINDING_RULE,GLU_TESS_WINDING_ABS_GEQ_TWO);

	// all the points are in the xy plan
	gluTessNormal(_tessellator, 0, 0, 1);
}


void glTiledTexturedImage::_tessDisplayCached(void)
{
	GLdouble ww = (GLdouble)_oimageWidth / 2;
	GLdouble hh = (GLdouble)_oimageHeight / 2;
	int x = 0, y = 0;
	uTex utex;
	int row, col;

	glTiledTexturedImage::tiledTessellation:: iterator itt;
	glTiledTexturedImage::tessellation:: iterator it;
	std::list<glTiledTexturedImage::point >:: iterator ip;

	itt = _tiledTessellation.begin();

	for (row = 0; row < _tex_per_row; row++)
	{
		for (col = 0; col < _tex_per_col; col++, ++itt)
		{
			GLdouble w1,h1;
			GLdouble right, bottom;

			if (itt == _tiledTessellation.end())
			{
				std::cerr << "_tessDisplayCached ERROR!"
					  << col << " " << row << std::endl;
				return;
			}
			
			utex = _texSet[row * _tex_per_col + col];

			_getRenderingParameters(
				col, row, x, y, &w1, &h1, &right, &bottom);

			_bind(utex.getTexnum(), w1, h1);

			for (it = (*itt).begin(); it != (*itt).end(); ++it)
			{
				glBegin((*it).type);
				for (ip = (*it).points.begin();
				     ip != (*it).points.end(); ++ip)
				{
					glTexCoord2d(
						((*ip).x + ww - x)/(w1/right),
						(-(*ip).y + hh - y)/(h1/bottom));
					glVertex2d((*ip).x, (*ip).y);
				}
				glEnd();     
			}
			x += utex.getWidth();
		}
		x = 0;
		y += utex.getHeight();
	}
}

void
glTiledTexturedImage::tessDisplay(
	std::list<glTiledTexturedImage::point> *poly, bool useTessCache)
{
	GLdouble ww = (GLdouble)_oimageWidth / 2;
	GLdouble hh = (GLdouble)_oimageHeight / 2;
	int x = 0, y = 0;
	uTex utex;
	int row, col;

	// try to do not use tessellation
	if (poly == 0 || poly->size() == 0)
	{
		display(poly);
		return;
	}

	if (useTessCache &&_tiledTessellation.size() > 0)
	{
		_tessDisplayCached();
		return;
	}

	_tessInit();

	saveTessellation = true;

	currentTiledTessellation.clear();

	for (row = 0; row < _tex_per_row; row++)
	{
		for (col = 0; col < _tex_per_col; col++)
		{
			utex = _texSet[row * _tex_per_col + col];
			
			GLdouble w1,h1;
			GLdouble right, bottom;

			currentTessellation.clear();
			_getRenderingParameters(
				col, row, x, y, &w1, &h1, &right, &bottom);

			// intersection with -ww, -hh, ww, -hh, ww, hh, -ww, hh
			GLdouble vertices[4][3] = {
				-ww + x, hh - (y + h1) , 0.0,
				-ww + (x + w1), hh - (y + h1), 0.0,
				-ww + (x + w1), hh - y, 0.0,
				-ww + x, hh - y, 0.0,
			} ;
			GLdouble texCoords[] = {
				-ww + x, hh - y,     // tex 0,0
				w1/right, h1/bottom  // tex width,height
			} ;

			_bind(utex.getTexnum(), w1, h1);

			// warning, this should be global vs gluTessBeginPolygon
			GLdouble p[poly->size()][3];

			gluTessBeginPolygon(_tessellator, texCoords);

			gluTessBeginContour(_tessellator);
			std::list<glTiledTexturedImage::point>::iterator it;
			int i;
			for (i = 0, it = poly->begin(); it != poly->end();
			     ++it, i++)
			{
			  
				p[i][0] = (*it).x;
				p[i][1] = (*it).y;
				p[i][2] = 0.0;
				gluTessVertex(_tessellator, p[i], p[i]);
			}
			gluTessEndContour(_tessellator);

			gluTessBeginContour(_tessellator);
			for (int i=0; i<4; i++)
			{
				gluTessVertex(
					_tessellator, vertices[i], vertices[i]);
			}
			gluTessEndContour(_tessellator);
			gluTessEndPolygon(_tessellator);

			currentTiledTessellation.push_back(currentTessellation);
			x += utex.getWidth();
		}
		x = 0;
		y += utex.getHeight();
	}
	
	saveTessellation = false;

	_tiledTessellation = currentTiledTessellation;

}

void
glTiledTexturedImage::getLastTessellation(
	glTiledTexturedImage::tiledTessellation *tess)
{
	*tess = _tiledTessellation;
}

//-------------------------------------------------------------------------

void
glTiledTexturedImage::display(std::list<glTiledTexturedImage::point> *poly)
{
	GLdouble ww = (GLdouble)_oimageWidth / 2;
	GLdouble hh = (GLdouble)_oimageHeight / 2;
	int x = 0, y = 0;
	uTex utex;
	int row, col;

	_tiledTessellation.clear();

	for (row = 0; row < _tex_per_row; row++)
	{
		for (col = 0; col < _tex_per_col; col++)
		{
			utex = _texSet[row * _tex_per_col + col];
			
			GLdouble w1,h1;
			GLdouble right, bottom;

			_getRenderingParameters(
				col, row, x, y, &w1, &h1, &right, &bottom);

			_bind(utex.getTexnum(), w1, h1);

			if (poly)
			{
				std::list<glTiledTexturedImage::point>::iterator
					it;

				// this simply does not work in general
				// if we have more than one texture. In this
				// case tessDisplay must be used
				glBegin(GL_POLYGON) ;
				for (it = poly->begin(); it != poly->end(); ++it)
				{
					GLdouble xp,yp;

					if ((*it).x < -ww + x)
						xp = -ww + x;
					else if ((*it).x > -ww + (x + w1))
						xp = -ww + (x + w1);
					else xp = (*it).x;

					if ((*it).y < hh - (y + h1))
						yp = hh - (y + h1);
					else if ((*it).y > hh - y)
						yp = hh - y;
					else yp = (*it).y;

					glTexCoord2d(
						(xp + ww - x)/(w1/right),
						(-yp + hh - y)/(h1/bottom));
					glVertex2d(xp, yp);
				}
				glEnd();
			}
			else
			{
				// intersection with
				// -ww, -hh, ww, -hh, ww, hh, -ww, hh
				GLdouble vertices[] = {
					-ww + x, hh - (y + h1) ,
					//(-ww + + (x + w1))/2, (hh - (y + h1)),
					-ww + (x + w1), hh - (y + h1),
					//-ww + (x + w1), (hh - y)/2,
					-ww + (x + w1), hh - y,
					-ww + x, hh - y
				};
			
				GLdouble texCoords[] = {
					0.0, bottom,
					//right/2, bottom/2,
					right, bottom,
					//right-0.10, bottom/2,
					right, 0.0,
					0.0, 0.0
				};
				glBegin(GL_POLYGON) ;
				for (int i=0; i<8; i+=2)
				{
					glTexCoord2d(
						texCoords[i], texCoords[i+1]);
					glVertex2d(
						vertices[i], vertices[i+1]) ;
				}
				glEnd();
			}

			x += utex.getWidth();
		}
		x = 0;
		y += utex.getHeight();
	}

}

void
glTiledTexturedImage::display(
	std::list<glTiledTexturedImage::point> *poly_img,
	std::list<glTiledTexturedImage::point> *poly_tex)
{
	GLdouble ww = (GLdouble)_oimageWidth / 2;
	GLdouble hh = (GLdouble)_oimageHeight / 2;
	int x = 0, y = 0;
	uTex utex;
	int row, col;

	_tiledTessellation.clear();

	for (row = 0; row < _tex_per_row; row++)
	{
		for (col = 0; col < _tex_per_col; col++)
		{
			utex = _texSet[row * _tex_per_col + col];
			
			GLdouble w1,h1;
			GLdouble right, bottom;

			_getRenderingParameters(
				col, row, x, y, &w1, &h1, &right, &bottom);

			_bind(utex.getTexnum(), w1, h1);
			//fprintf(stderr,"TEX c: %i, r: %i, w1: %f, h1: %f, r: %f, b: %f\n",
			//col, row, w1, h1, right, bottom);

			if (poly_img && poly_tex)
			{
				std::list<glTiledTexturedImage::point>::iterator
					tex_it;
				std::list<glTiledTexturedImage::point>::iterator
					img_it;
				// this simply does not work in general
				// if we have more than one texture. In this
				// case tessDisplay must be used
				glBegin(GL_POLYGON) ;
				for (tex_it = poly_tex->begin(),
				     img_it = poly_img->begin();
				     tex_it != poly_tex->end() &&
				     img_it != poly_img->end();
				     tex_it++,img_it++)
				{
					GLdouble xt,yt,xi,yi;

					// tex
					if ((*tex_it).x == 0 &&
					    (*tex_it).y == 0)
					{
						glEnd();
						glBegin(GL_POLYGON);
						continue;
					}
					if ((*tex_it).x < -ww + x)
						xt = -ww + x;
					else if ((*tex_it).x > -ww + (x + w1))
						xt = -ww + (x + w1);
					else xt = (*tex_it).x;
					xt = (*tex_it).x;
					if ((*tex_it).y < hh - (y + h1))
						yt = hh - (y + h1);
					else if ((*tex_it).y > hh - y)
						yt = hh - y;
					else yt = (*tex_it).y;
					yt = (*tex_it).y;

					// img
					if ((*img_it).x < -ww + x)
						xi = -ww + x;
					else if ((*img_it).x > -ww + (x + w1))
						xi = -ww + (x + w1);
					else xi = (*img_it).x;
					xi = (*img_it).x;
					if ((*img_it).y < hh - (y + h1))
						yi = hh - (y + h1);
					else if ((*img_it).y > hh - y)
						yi = hh - y;
					else yi = (*img_it).y;
					yi = (*img_it).y;

					//glVertex2d(xi, yi);
					xt = (xt + ww - x)/(w1/right);
					yt = (-yt + hh - y)/(h1/bottom);
					glTexCoord2d(xt, yt);
					glVertex2d(xi, yi);
					//fprintf(stderr,"%f %f, %f %f \\ ",xt,yt,xi,yi);
				}
				glEnd();
				//fprintf(stderr,"\n");
			}
			else
			{
				display(poly_tex);
			}

			x += utex.getWidth();
		}
		x = 0;
		y += utex.getHeight();
	}

}

// ------------------------------------------------------------------------

GLuint
glTiledTexturedImage::getTarget(void)
{
	return _target ;
}

unsigned int
glTiledTexturedImage::getWidth(void)
{
	return _textureImage.getWidth() ;
}

unsigned int
glTiledTexturedImage::getHeight(void)
{
	return _textureImage.getHeight() ;
}

const Image&
glTiledTexturedImage::getTexture(void)
{
	return _textureImage ;
}
  
  
// ...etc
