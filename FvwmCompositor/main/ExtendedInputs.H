#ifndef EXTENDED_INPUTS_H
#define EXTENDED_INPUTS_H

#include "AScreen.H"
#include "AUtils.H"

//int extev_pointers_init(AScreen *ascr);
//int extev_handle_event(AScreen *ascr, glWindow::event e);

using namespace nucleo ;

class ExtendedInputs;

class AExtDevice : public glWindow::extensionDevice {

private:
	ExtendedInputs *_inputs;
	AScreen *_ascr;
	//glWindow::extensionDevice *_ed;
	unsigned int _num;
	bool _on;

	ACursor *_cursor;

	bool _toolGlass;
	MetisseWindow *_toolglassWin;

	int _prevX;
	int _prevY;
	unsigned char _button_mask;
	unsigned int _swallow_release;
	bool _firtsExtMotionNotify;
	int _prevExtX;
	int _prevExtY;
	
	void _toggleToolGlass(void);
	
public:
	void on(void);
	void off(void);
	void cursor(void);
	void toolGlass(MetisseWindow *win);
	bool isOn(void);
	bool isCursor(void);
	bool isToolGlass(void);
	MetisseWindow *getToolGlassWin(void);
	int getNum(void);
	void windowDestroyed(MetisseWindow *win);
	int handleEvents(glWindow::event e);

	AExtDevice(
		AScreen *ascr, ExtendedInputs *inputs,
		glWindow::extensionDevice *ed, int num);
};

class ExtendedInputs {

private:
	AScreen *_ascr;
	std::list<AExtDevice *> _devices;

	void _generateCursorsMenus(void);

public:
	bool generateWindowMenus(MetisseWindow *win);
	void windowDestroyed(MetisseWindow *win);
	void configure(MetisseWindow *win, char *name, char *opt);
	int handleEvents(glWindow::event e);

	ExtendedInputs(AScreen *ascr);
};


#endif
