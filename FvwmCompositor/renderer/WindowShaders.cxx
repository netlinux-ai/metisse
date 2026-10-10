/*
 *
 * renderer/WindowShaders.cxx --
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

#include "WindowShaders.H"

#include <math.h>

#include <nucleo/utils/StringUtils.H>
#include <nucleo/core/URI.H>

void WindowShaders::removeShaderWindow(
	WindowRenderer *win, std::string shadername)
{
	std::map<std::string,glShader*>::iterator i = _shaders.find(shadername);
	if (i==_shaders.end())
	{
		std::cerr << "WindowShaders::addShadersToWindow: unknown "
			" shader '" << shadername << "'" << std::endl ;
		return ;
	}
	bool doit = true;
	while (doit)
	{
		doit = win->removeShader((*i).second);
	}
}

bool WindowShaders::_parseWindowShaderArgs(
	std::string args, ShaderRendererOpt *sro)
{
	while (args != "")
	{
		std::string opt = extractNextWord(args);
		// FIXME: no case
		if (opt == "Anim")
		{
			int val[2] = {0,0};
			if (GetIntegerArguments(
				    (char *)args.c_str(), NULL, val, 2) == 2
			    && val[0] > 0 && val[1] > 0)
			{
				sro->nbr_anim_step = val[0];
				sro->anim_total_time = val[1];
				sro->type |= SHADER_RENDERER_ANIMATE;
				opt = extractNextWord(args);
				opt = extractNextWord(args);
			}
			else
			{
				std::cerr << "WindowShaderArgs ERROR: "
					  << "Anim needs 2 positive integers "
					  << "arg!\n";
				sro->type &= !SHADER_RENDERER_ANIMATE;
				return false;
			}
		}
		else if (opt == "Periodic")
		{
			sro->type |= SHADER_RENDERER_PERIODIC;
			sro->stale_time = 0;

			int val[1] = {0};
			if (GetIntegerArguments(
				    (char *)args.c_str(), NULL, val, 1) == 1
			    && val[0] > 0)
			{
				sro->stale_time = val[0];
				opt = extractNextWord(args);
			}
		}
		else if (opt == "Temp")
		{
			int val[1] = {0};
			if (GetIntegerArguments(
				    (char *)args.c_str(), NULL, val, 1) == 1
			    && val[0] > 0)
			{
				sro->life_time = val[0];
				sro->type |= SHADER_RENDERER_TMP;
				opt = extractNextWord(args);
			}
			else
			{
				std::cerr << "WindowShaderArgs ERROR: "
					  << "Temp needs 1 positive integer "
					  << "arg!\n";
				sro->type &= !SHADER_RENDERER_TMP;
				return false;
			}
		}
		else
		{
			std::cerr << "WindowShaderArgs ERROR: "
				  << "Unknown arg '" << opt << "'\n";
			return false;	
		}
	}
	return true;
}

void WindowShaders::addShaderWindow(
	WindowRenderer *win, std::string args, bool clear, bool back)
{
	std::string shadername = extractNextWord(args);
	std::map<std::string,glShader*>::iterator i = _shaders.find(shadername);
	if (i==_shaders.end())
	{
		std::cerr << "WindowShaders::addShadersToWindow: unknown "
			" shader '" << shadername << "'" << std::endl ;
		return ;
	}
	ShaderRendererOpt *sro = new ShaderRendererOpt();
	_parseWindowShaderArgs(args, sro);
	if (clear)
	{
		win->removeAllShaders();
	}
	win->addShader((*i).second, sro, back);
}

void WindowShaders::animShaderWindow(
	WindowRenderer *win, std::string shadername, bool front)
{
	win->animShader(front);
}

// -------------------------------------

void WindowShaders::_destroyShader(
	std::map<std::string,glShader*>::iterator i, glShader *replacement)
{
	glShader *shader = (*i).second;

	WindowMapping wins = _desktop->getWindowMapping();
	WindowMapping::iterator wini;
	for (wini = wins.begin(); wini != wins.end(); wini++)
	{
		WindowRenderer *w = ((*wini).second)->getRenderer();
		if (replacement)
		{
			w->replaceShader(shader, replacement);
		}
		else
		{
			w->removeShader(shader);
		}
	}
	delete shader ;
	_shaders.erase(i) ;
}

void WindowShaders::destroyShader(std::string shadername)
{
	std::map<std::string,glShader*>::iterator i = _shaders.find(shadername) ;
	if (i==_shaders.end())
	{
		std::cerr << "WindowRenderer::destroyWindowShader: unknown "
			" shader '" << shadername << "'" << std::endl ;
		return ;
	}

	std::cerr << "WindowRenderer::destroyWindowShader: destroying shader '"
		  << shadername << "'" << std::endl ;
	_destroyShader(i, 0);
}

void WindowShaders::defineShader(
	std::string shadername, std::string filenames, bool add)
{
	glShader *shader = 0 ;
	std::map<std::string,glShader*>::iterator i = _shaders.find(shadername) ;
	if (i==_shaders.end())
	{
		std::cerr << "WindowShaders::defineShader: creating "
			  << "shader " << shadername << std::endl ;
		_shaders[shadername] = shader = new glShader ;
	}
	else if (!add)
	{
		std::cerr << "WindowShaders::addToShader: redefine "
			  << "shader " << shadername << std::endl ;
		shader = new glShader ;
		// destroy and replace the shader
		_destroyShader(i, shader);
		_shaders[shadername] = shader;
	}
	else
	{
		shader = (*i).second;
	}

	if (filenames=="")
	{
		// define a dummy shader ...
		std::cerr << "WindowShaders::defineShader: no filename "
			<< "(defening dummy shader?)\n";
		return;
	}

	std::string token = extractNextWord(filenames);
	while(token != "")
	{
		URI uri(token) ;
		if (uri.scheme != "vertex" && uri.scheme != "fragment")
		{
			break;
		}
		std::string path = (uri.opaque!="" ? uri.opaque : uri.path) ;
		std::cerr << "WindowShaders::defineShader: attaching "
			  << uri.scheme << " shader " << path << " to '"
			  << shadername << "'" << std::endl ;
		shader->attachFromFile(path,uri.scheme,path) ;
		token = extractNextWord(filenames);
	}
	if (!shader->link())
	{
		std::cerr << "WindowShaders::defineShader: shader '"
			  << shadername << "' Fail to link!" << std::endl ;
		return;
	}

	// FIXME: tmp code ... should activate globally and desactivate
	// if we pass uniform parameters.
	while (token != "")
	{
		if (token == "int")
		{
			token = extractNextWord(filenames);
			if (token != "")
			{
				std::string tmp;
				tmp = extractNextWord(filenames);
				GLint p = (GLint)atoi(tmp.c_str());
				shader->setUniformParam(token, p);
			}
		}
		token = extractNextWord(filenames);
	}
}

WindowShaders::WindowShaders(MetisseDesktop *desktop)
{
	_desktop = desktop;
}
