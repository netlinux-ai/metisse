#ifndef WindowShaders_H
#define WindowShaders_H

/*
 *
 * renderer/WindowShaders.H --
 *
 * Copyright (C) Olivier Chapuis
 * Copyright (C) Nicolas Roussel
 *
 * See the file LICENSE for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <string>
#include <map>

#include <nucleo/gl/shading/glShader.H>

#include "desktop/MetisseDesktop.H"

using namespace nucleo;

class WindowShaders
{

private:
	MetisseDesktop *_desktop;

protected:
	std::map<std::string,glShader*> _shaders;

public:
	void removeShaderWindow(WindowRenderer *win, std::string shadername);
private:
	bool _parseWindowShaderArgs(std::string args, ShaderRendererOpt *sro);
public:
	void addShaderWindow(
		WindowRenderer *win, std::string args, bool clear = false,
		bool back = false);
	void animShaderWindow(
		WindowRenderer *win, std::string shadername, bool front);
private:
	void _destroyShader(
		std::map<std::string,glShader*>::iterator i,
		glShader *replacement);
public:
	void destroyShader(std::string shadername);
	void defineShader(
		std::string shadername, std::string filenames, bool add);

	WindowShaders(MetisseDesktop *desktop);
};

#endif
