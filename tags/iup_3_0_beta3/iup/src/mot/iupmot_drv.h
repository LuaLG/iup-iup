/** \file
 * \brief Motif Driver 
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMOT_DRV_H 
#define __IUPMOT_DRV_H

#ifdef __cplusplus
extern "C" {
#endif


/* global variables, declared in iupmot_open.c */
extern Widget         iupmot_appshell;         
extern Display*       iupmot_display;          
extern int            iupmot_screen;           
extern XtAppContext   iupmot_appcontext;       
extern Visual*        iupmot_visual;
extern Atom           iupmot_wm_deletewindow;

/* dialog */
void iupmotDialogSetVisual(Ihandle* ih, void* visual);
void iupmotDialogResetVisual(Ihandle* ih);

/* focus */
void iupmotFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);

/* key */
void iupmotCanvasKeyReleaseEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
void iupmotKeyPressEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
KeySym iupmotKeyCharToKeySym(char c);
void iupmotButtonKeySetStatus(unsigned int state, unsigned int but, char* status, int doubleclick);

/* font */
char* iupmotGetFontListAttrib(Ihandle *ih);
char* iupmotGetFontStructAttrib(Ihandle *ih);
XmFontList iupmotFontCreateNativeFont(const char* value);

/* tips */
/* called from Enter/Leave events to check if a TIP is present. */
void iupmotTipEnterNotify(Ihandle* ih);
void iupmotTipLeaveNotify(void);
void iupmotTipsFinish(void);

/* common */
void iupmotPointerMotionEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
void iupmotButtonPressReleaseEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont);
void iupmotEnterLeaveWindowEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
void iupmotHelpCallback(Widget w, Ihandle *ih, XtPointer call_data);
void iupmotSetString(Widget w, const char *resource, const char* value);
char* iupmotConvertString(XmString str);
void iupmotSetMnemonicTitle(Ihandle *ih, const char* value);
void iupmotDisableDragSource(Widget w);
void iupmotSetPixmap(Ihandle* ih, const char* name, const char* prop, int make_inactive, const char* attrib_name);
void iupmotSetGlobalColorAttrib(Widget w, const char* xmname, const char* name);
void iupmotSetBgColor(Widget w, Pixel color);
char* iupmotGetBgColorAttrib(Ihandle* ih);

void iupmotGetWindowSize(Ihandle *ih, int *width, int *height);

char* iupmotGetXWindowAttrib(Ihandle *ih);

#define iupmotSetArg(_a, _n, _d) ((_a).name = (_n), (_a).value = (XtArgVal)(_d))


#ifdef __cplusplus
}
#endif

#endif
