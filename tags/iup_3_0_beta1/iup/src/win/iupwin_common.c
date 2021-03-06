/** \file
 * \brief Windows Base Procedure
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include <windows.h>
#include <commctrl.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_dialog.h"
#include "iup_drvinfo.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"


#ifndef  XBUTTON1
#define XBUTTON1      0x0001     /* not defined in MingW3 */
#endif

int iupwinClassExist(const char* name)
{
  WNDCLASS WndClass;
  if (GetClassInfo(iupwin_hinstance, name, &WndClass))
    return 1;
  return 0;
}

int iupwinGetScreenRes(void)
{
  int res;
  HDC ScreenDC = GetDC(NULL);
  res = GetDeviceCaps(ScreenDC, LOGPIXELSY);
  ReleaseDC(NULL, ScreenDC);
  return res;
}

void iupdrvActivate(Ihandle* ih)
{
  /* do not use BM_CLICK because it changes the focus 
     and does not animates the button press */
  SendMessage(ih->handle, BM_SETSTATE, TRUE, 0);
  IupFlush();
  Sleep(150);
  SendMessage(GetParent(ih->handle), WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), (LPARAM)ih->handle);
  SendMessage(ih->handle, BM_SETSTATE, FALSE, 0);
}

WCHAR* iupwinStrChar2Wide(const char* str)
{
  if (str)
  {
    int len = strlen(str)+1;
    WCHAR* wstr = malloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, len);
    return wstr;
  }

  return NULL;
}

int iupdrvGetScrollbarSize(void)
{
  int xv = GetSystemMetrics(SM_CXVSCROLL);
  int yh = GetSystemMetrics(SM_CYHSCROLL);
  return xv > yh ? xv : yh;
}

void iupdrvReparent(Ihandle* ih)
{
  SetParent(ih->handle, iupChildTreeGetNativeParentHandle(ih));
}

void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  SetWindowPos(ih->handle, NULL, ih->x, ih->y, ih->currentwidth, ih->currentheight,
               SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
}

void iupwinRedrawNow(Ihandle *ih)
{
  InvalidateRect(ih->handle, NULL, FALSE);  
  UpdateWindow(ih->handle);
}

void iupdrvDisplayUpdate(Ihandle *ih)
{
  RedrawWindow(ih->handle,NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_INTERNALPAINT);
}

void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  POINT p;
  p.x = *x;
  p.y = *y;
  ScreenToClient(ih->handle, &p);
  *x = p.x;
  *y = p.y;
}

static void winTrackMouse(HWND hwnd, int enter)
{
  TRACKMOUSEEVENT mouse;
  mouse.cbSize = sizeof(TRACKMOUSEEVENT);

  if (enter)
    mouse.dwFlags = TME_HOVER;
  else
    mouse.dwFlags = TME_LEAVE;

  mouse.hwndTrack = hwnd;
  mouse.dwHoverTime = 1;
  TrackMouseEvent(&mouse);
}

static void winCallEnterLeaveWindow(Ihandle *ih, int enter)
{
  Icallback cb = NULL;

  if (!ih->iclass->is_interactive)
    return;

  if (enter)
  {
    winTrackMouse(ih->handle, 0);

    if (!iupAttribGetInt(ih, "_IUPWIN_ENTERWIN"))
    {
      cb = IupGetCallback(ih,"ENTERWINDOW_CB");
      iupAttribSetStr(ih, "_IUPWIN_ENTERWIN", "1");
    }
  }
  else 
  {
    cb = IupGetCallback(ih,"LEAVEWINDOW_CB");
    iupAttribSetStr(ih, "_IUPWIN_ENTERWIN", NULL);
  }

  if (cb)
    cb(ih);
}

void iupwinMergeStyle(Ihandle* ih, DWORD old_mask, DWORD value)
{
  DWORD dwStyle = GetWindowLongPtr(ih->handle, GWL_STYLE);
  dwStyle &= ~(old_mask); /* clear old bits */
  dwStyle |= value;
  SetWindowLongPtr(ih->handle, GWL_STYLE, dwStyle);
}

void iupwinSetStyle(Ihandle* ih, DWORD value, int set)
{
  DWORD dwStyle = GetWindowLongPtr(ih->handle, GWL_STYLE);
  if (set)
    dwStyle |= value;
  else
    dwStyle &= ~(value);
  SetWindowLongPtr(ih->handle, GWL_STYLE, dwStyle);
}

int iupwinBaseProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{ 
  (void)lp;

  switch (msg)
  {
  case WM_NOTIFY: /* usually sent only to parent, 
                     but TIPs are configured to be handled here */
    {
      NMHDR* msg_info = (NMHDR*)lp;
      if (msg_info->code == TTN_GETDISPINFO)
        iupwinTipsGetDispInfo(lp);
      break;
    }
  case WM_DROPFILES:
    iupwinDropFiles((HDROP)wp, ih);
    break;
  case WM_HELP:
    {
      Ihandle* child;
      HELPINFO* help_info = (HELPINFO*)lp;

      if (help_info->iContextType == HELPINFO_MENUITEM)
        child = iupwinMenuGetItemHandle((HMENU)help_info->hItemHandle, (int)help_info->iCtrlId);
      else
        child = iupwinHandleGet(help_info->hItemHandle);

      if (child)
      {
        Icallback cb = (Icallback) IupGetCallback(child, "HELP_CB");
        if (cb) 
        {
          if (cb(child) == IUP_CLOSE) 
            IupExitLoop();

          *result = 0;
          return 1;  /* abort default processing */
        }
      }
      break;
    }
  case WM_MOUSELEAVE:
    winCallEnterLeaveWindow(ih, 0);
    break;
  case WM_MOUSEMOVE:
    winCallEnterLeaveWindow(ih, 1);
    break;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if (!iupwinKeyEvent(ih, (int)wp, 1))
    {
      *result = 0;
      return 1;   /* abort default processing */
    }
    break;
  case WM_SYSKEYUP:
  case WM_KEYUP:
    {
      int ret;
      if (wp == VK_SNAPSHOT || wp == VK_RETURN) /* called only on key up */
        ret = iupwinKeyEvent(ih, (int)wp, 1);
      else
        ret = iupwinKeyEvent(ih, (int)wp, 0);

      if (!ret)
      {
        *result = 0;
        return 1;  /* abort default processing */
      }

      break;
    }
  case WM_SETFOCUS:
    iupwinWmSetFocus(ih);
    break;
  case WM_KILLFOCUS:
    iupCallKillFocusCb(ih);
    break;
  case WOM_CLOSE:
  case WOM_DONE:
  case WOM_OPEN:
    {
      IFni cb = (IFni)IupGetCallback(ih, "WOM_CB");
      if (cb)
      {
        int v = -2; /* Error */
        switch(msg)
        {
        case WOM_OPEN:  v = 1;  break;
        case WOM_DONE:  v = 0;  break;
        case WOM_CLOSE: v = -1; break;
        }
        cb(ih, v);
      }
      break;
    }
  }

  return 0;
}

LRESULT CALLBACK iupwinBaseWinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  int ret = 0;
  LRESULT result = 0;
  IwinProc winProc;
  WNDPROC oldProc;
  Ihandle *ih;

  ih = iupwinHandleGet(hwnd); 
  if (!ih)
    return DefWindowProc(hwnd, msg, wp, lp);  /* should never happen */

  /* retrieve the control previous procedure for subclassing */
  oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDPROC_CB");

  /* check if the element defines a custom procedure */
  winProc = (IwinProc)IupGetCallback(ih, "_IUPWIN_CTRLPROC_CB");
  if (winProc)
    ret = winProc(ih, msg, wp, lp, &result);
  else
    ret = iupwinBaseProc(ih, msg, wp, lp, &result);

  if (ret)
    return result;
  else
    return CallWindowProc(oldProc, hwnd, msg, wp, lp);
}

static Ihandle* winContainerWmCommandGetIhandle(Ihandle *ih, WPARAM wp, LPARAM lp)
{
  /* WPARAM - if HIWORD is 0 if the message is from a menu.
              or HIWORD is 1 if the message is from an accelerator.
              or HIWORD is the notification code if the message is from a control.
              LOWORD is the identifier.
     LPARAM - the control sending the message or 0. */

  Ihandle *child = NULL;

  if (HIWORD(wp)==0 && lp==0 && LOWORD(wp)>10)
  {
    Ihandle* dlg_menu = IupGetAttributeHandle(ih, "MENU");
    if (dlg_menu)
      child = iupwinMenuGetItemHandle((HMENU)dlg_menu->handle, LOWORD(wp));  /* menu */
  }
  else 
  {
    if (lp==0)
      child = ih;                              /* dialog */
    else
      child = iupwinHandleGet((void*)lp);       /* control */
  }

  return child;
}

int iupwinBaseContainerProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  /* All messages here are sent to the parent Window, 
     but they are usefull for child controls.  */

  switch (msg)
  {
  case WM_COMMAND:
    {
      Ihandle* child = winContainerWmCommandGetIhandle(ih, wp, lp);

      if (child == ih) /* dialog */
      {
        if (HIWORD(wp) == 0)
        {
          char key[5];
          Ihandle* bt;
          char* default_but = LOWORD(wp) == IDOK? "DEFAULTENTER": LOWORD(wp) == IDCANCEL? "DEFAULTESC": NULL;

          if (LOWORD(wp) == IDOK)  /* the current button is activated */
          {
            Ihandle* focus = IupGetFocus();
            if (iupObjectCheck(focus) && iupStrEqual(focus->iclass->name, "button"))
            {
              iupdrvActivate(focus);
              break;
            }
          }

          iupdrvGetKeyState(key);
          if (iupStrEqual(key, "    "))
          {
            /* try the default buttons */
            bt = IupGetAttributeHandle(ih, default_but);
            if (iupObjectCheck(bt) && iupStrEqual(bt->iclass->name, "button"))
              iupdrvActivate(bt);
          }
        }
      }
      else if (child)
      {
        IFnii cb = (IFnii)IupGetCallback(child, "_IUPWIN_COMMAND_CB");
        if (cb)
          cb(child, wp, lp);
      }

      break;
    }
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORSTATIC:
    {
      Ihandle* child = iupwinHandleGet((void*)lp);
      if (child)
      {
        IFctlColor cb = (IFctlColor)IupGetCallback(child, "_IUPWIN_CTLCOLOR_CB");
        if (cb)
          return cb(child, (HDC)wp, result);
      }
      break;
    }
  case WM_DRAWITEM:   /* for OWNERDRAW controls */
    {
      Ihandle *child = NULL;
      DRAWITEMSTRUCT *drawitem = (LPDRAWITEMSTRUCT)lp;
      if (!drawitem)
        break;

      if (wp == 0) /* a menu */
        child = iupwinMenuGetItemHandle((HMENU)drawitem->hwndItem, drawitem->itemID);
      else
        child = iupwinHandleGet(drawitem->hwndItem); 

      if (child)
      {
        IFdrawItem cb = (IFdrawItem)IupGetCallback(child, "_IUPWIN_DRAWITEM_CB");
        if (cb) 
        {
          cb(child, (void*)drawitem);
          *result = TRUE;
          return 1;
        }
      }
      break;
    }
  case WM_HSCROLL:
  case WM_VSCROLL:
    {
      Ihandle *child = iupwinHandleGet((void*)lp);
      if (child)
      {
        IFni cb = (IFni)IupGetCallback(child, "_IUPWIN_CUSTOMSCROLL_CB");
        if (cb)
          cb(child, LOWORD(wp));
      }
      break;
    }
  case WM_NOTIFY:   /* Currently, the following controls support custom draw functionality: 
                       Header, List-view, Rebar, Toolbar, ToolTip, Trackbar, Tree-view.
                       And for Button if using Windows XP Style. */
    {
      Ihandle *child;
      NMHDR* msg_info = (NMHDR*)lp;
      if (!msg_info)
        break;

      child = iupwinHandleGet(msg_info->hwndFrom); 
      if (child)
      {
        IFnotify cb = (IFnotify)IupGetCallback(child, "_IUPWIN_NOTIFY_CB");
        if (cb)
        {
          if (cb(child, (void*)msg_info, result))
            return 1;
        }
      }
      break;
    }
  }

  return iupwinBaseProc(ih, msg, wp, lp, result);
}

void iupwinChangeProc(Ihandle *ih, WNDPROC new_proc)
{
  IupSetCallback(ih, "_IUPWIN_OLDPROC_CB", (Icallback)GetWindowLongPtr(ih->handle, GWLP_WNDPROC));
  SetWindowLongPtr(ih->handle, GWLP_WNDPROC, (LONG_PTR)new_proc);
}

void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDPROC_CB");
  if (oldProc)
  {
    SetWindowLongPtr(ih->handle, GWLP_WNDPROC, (LONG_PTR)oldProc);
    IupSetCallback(ih, "_IUPWIN_OLDPROC_CB",  NULL);
  }

  /* remove the association before destroying */
  iupwinHandleRemove(ih);

  /* remove from parent and destroys window */
  SetParent(ih->handle, NULL);
  DestroyWindow(ih->handle);
}

void iupwinDropFiles(HDROP hDrop, Ihandle *ih)
{
  char *filename;
  int i, numFiles, numchar, ret;
  POINT point;

  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb) return; 

  numFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
  DragQueryPoint(hDrop, &point);  
  for (i = 0; i < numFiles; i++)
  {
    numchar = DragQueryFile(hDrop, i, NULL, 0);
    filename = malloc(numchar+1); 
    if (!filename)
      break;

    DragQueryFile(hDrop, i, filename, numchar+1);

    ret = cb(ih, filename, numFiles-i-1, (int) point.x, (int) point.y); 

    free(filename);

    if (ret == IUP_IGNORE)
      break;
  }
  DragFinish(hDrop);
}

int iupwinGetColorRef(Ihandle *ih, char *name, COLORREF *color)
{
  unsigned char r, g, b;
  /* must use IupGetAttribute to use inheritance */
  if (iupStrToRGB(IupGetAttribute(ih, name), &r, &g, &b))
  {
    *color = RGB(r,g,b);
    return 1;
  }
  return 0;
}

int iupwinGetParentBgColor(Ihandle* ih, COLORREF* cr)
{
  unsigned char r, g, b;
  char* color = iupBaseNativeParentGetBgColorAttrib(ih);
  if (iupStrToRGB(color, &r, &g, &b))
  {
    *cr = RGB(r,g,b);
    return 1;
  }
  return 0;
}

int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (IsWindowVisible(ih->handle))
  {
    if (iupStrEqualNoCase(value, "TOP"))
      SetWindowPos(ih->handle, HWND_TOP, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    else
      SetWindowPos(ih->handle, HWND_BOTTOM, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
  }

  return 0;
}

void iupdrvSetVisible(Ihandle* ih, int visible)
{
  ShowWindow(ih->handle, visible? SW_SHOWNORMAL: SW_HIDE);
}

int iupdrvIsVisible(Ihandle* ih)
{
  return IsWindowVisible(ih->handle);
}

int iupdrvIsActive(Ihandle* ih)
{
  return IsWindowEnabled(ih->handle);
}

void iupdrvSetActive(Ihandle* ih, int enable)
{
  EnableWindow(ih->handle, enable);
}

int iupdrvBaseSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  SetWindowText(ih->handle, value);
  return 0;
}

char* iupdrvBaseGetTitleAttrib(Ihandle* ih)
{
  int nc = GetWindowTextLength(ih->handle);
  if (nc)
  {
    char* str = iupStrGetMemory(nc+1);
    GetWindowText(ih->handle, str, nc+1);
    return str;
  }
  else
    return NULL;
}

int iupwinSetDragDropAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    DragAcceptFiles(ih->handle, TRUE);
  else
    DragAcceptFiles(ih->handle, FALSE);
  return 1;
}

char *iupdrvBaseGetXAttrib(Ihandle *ih)
{
  char* str = iupStrGetMemory(20);
  RECT rect;
  GetWindowRect(ih->handle, &rect);
  sprintf(str, "%d", (int)rect.left);
  return str;
}

char *iupdrvBaseGetYAttrib(Ihandle *ih)
{
  char* str = iupStrGetMemory(20);
  RECT rect;
  GetWindowRect(ih->handle, &rect);
  sprintf(str, "%d", (int)rect.top);
  return str;
}

char* iupdrvBaseGetClientSizeAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(20);
  RECT rect;
  GetClientRect(ih->handle, &rect);
  sprintf(str, "%dx%d", (int)(rect.right-rect.left), (int)(rect.bottom-rect.top));
  return str;
}

#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif
#ifndef IDC_APPSTARTING
#define IDC_APPSTARTING     MAKEINTRESOURCE(32650) 
#endif
#ifndef IDC_HELP
#define IDC_HELP            MAKEINTRESOURCE(32651)
#endif

static HCURSOR winGetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    const char* sysname;
  } table[] = {
    {"NONE",      NULL}, 
    {"NULL",      NULL}, 
    {"ARROW",     IDC_ARROW},
    {"BUSY",      IDC_WAIT},
    {"CROSS",     IDC_CROSS},
    {"HAND",      IDC_HAND},
    {"MOVE",      IDC_SIZEALL},
    {"RESIZE_N",  IDC_SIZENS},
    {"RESIZE_S",  IDC_SIZENS},
    {"RESIZE_NS", IDC_SIZENS},
    {"RESIZE_W",  IDC_SIZEWE},
    {"RESIZE_E",  IDC_SIZEWE},
    {"RESIZE_WE", IDC_SIZEWE},
    {"RESIZE_NE", IDC_SIZENESW},
    {"RESIZE_SE", IDC_SIZENWSE},
    {"RESIZE_NW", IDC_SIZENWSE},
    {"RESIZE_SW", IDC_SIZENESW},
    {"TEXT",      IDC_IBEAM},
    {"HELP",      IDC_HELP},
    {"IUP",       IDC_HELP},
    {"NO",        IDC_NO},
    {"UPARROW",   IDC_UPARROW},
    {"APPSTARTING", IDC_APPSTARTING}
  };

  HCURSOR cur;
  char str[50];
  int i, count = sizeof(table)/sizeof(table[0]);

  /* check the cursor cache first (per control)*/
  sprintf(str, "_IUPWIN_CURSOR_%s", name);
  cur = (HCURSOR)iupAttribGetStr(ih, str);
  if (cur)
    return cur;

  /* check the pre-defined IUP names first */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname)) 
    {
      if (table[i].sysname)
        cur = LoadCursor(NULL, table[i].sysname);
      else
        cur = NULL;

      break;
    }
  }

  if (i == count)
  {
    /* check other system cursors */
    /* cursor PEN is handled here */
    if (iupStrEqualNoCase(name, "PEN"))
      name = "CURSOR_PEN";

    /* check for an name defined cursor */
    cur = iupImageGetCursor(name);
  }

  iupAttribSetStr(ih, str, (char*)cur);
  return cur;
}

int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  /* Cursor can be NULL in Windows. */
  HCURSOR hCur = winGetCursor(ih, value);
  iupAttribSetStr(ih, "_IUPWIN_HCURSOR", (char*)hCur);  /* To be used in WM_SETCURSOR */
  /* refresh the cursor */
  SendMessage(ih->handle, WM_SETCURSOR, (WPARAM)ih->handle, MAKELPARAM(1,WM_MOUSEMOVE));
  return 1;
}

void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "HFONT", iupwinGetHFontAttrib, NULL, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
}

int iupwinButtonDown(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, doubleclick = 0;
  int b = 0;

  IFniiiis cb = (IFniiiis) IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  if (msg==WM_XBUTTONDBLCLK ||
      msg==WM_LBUTTONDBLCLK ||
      msg==WM_MBUTTONDBLCLK ||
      msg==WM_RBUTTONDBLCLK)
    doubleclick = 1;

  iupwinButtonKeySetStatus(LOWORD(wp), status, doubleclick);

  if (msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK)
    b = IUP_BUTTON1;
  else if (msg==WM_MBUTTONDOWN || msg==WM_MBUTTONDBLCLK)
    b = IUP_BUTTON2;
  else if (msg==WM_RBUTTONDOWN || msg==WM_RBUTTONDBLCLK)
    b = IUP_BUTTON3;
  else if (msg==WM_XBUTTONDOWN || msg==WM_XBUTTONDBLCLK)
  {
    if (HIWORD(wp) == XBUTTON1)
      b = IUP_BUTTON4;
    else
      b = IUP_BUTTON5;
  }

  ret = cb(ih, b, 1, (int)(short)LOWORD(lp), (int)(short)HIWORD(lp), status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

int iupwinButtonUp(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, b=0;
  IFniiiis cb = (IFniiiis) IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  iupwinButtonKeySetStatus(LOWORD(wp), status, 0);

  /* also updates the button status, since wp could not have the flag */
  if (msg==WM_LBUTTONUP)
  {
    b = IUP_BUTTON1;
    iupKEYSETBUTTON1(status);  
  }
  else if (msg==WM_MBUTTONUP)
  {
    b = IUP_BUTTON2;
    iupKEYSETBUTTON2(status);
  }
  else if (msg==WM_RBUTTONUP)
  {
    b = IUP_BUTTON3;
    iupKEYSETBUTTON3(status);
  }
  else if (msg==WM_XBUTTONUP)
  {
    if (HIWORD(wp) == XBUTTON1)
    {
      b = IUP_BUTTON4;
      iupKEYSETBUTTON4(status);
    }
    else
    {
      b = IUP_BUTTON5;
      iupKEYSETBUTTON5(status);
    }
  }

  ret = cb(ih, b, 0, (int)(short)LOWORD(lp), (int)(short)HIWORD(lp), status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

int iupwinMouseMove(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp)
{
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupwinButtonKeySetStatus(LOWORD(wp), status, 0);
    cb(ih, (int)(short)LOWORD(lp), (int)(short)HIWORD(lp), status);
    return 1;
  }
  (void)msg;
  return 0;
}

int iupwinCreateWindowEx(Ihandle* ih, LPCSTR lpClassName, DWORD dwExStyle, DWORD dwStyle)
{
  ih->serial = iupDialogGetChildId(ih);

  ih->handle = CreateWindowEx(dwExStyle,  /* extended window style */
    lpClassName,                  /* window class */
    NULL,                         /* title */
    dwStyle,                      /* window style */
    0,                            /* x-position */
    0,                            /* y-position */
    10,                           /* default width to avoid 0 */
    10,                           /* default height to avoid 0 */
    iupChildTreeGetNativeParentHandle(ih),     /* window parent */
    (HMENU)ih->serial,            /* child identifier */
    iupwin_hinstance,             /* instance of app. */
    NULL);

  if (!ih->handle)
    return 0;

  /* associate HWND with Ihandle*, all Win32 controls must call this. */
  iupwinHandleSet(ih);

  /* replace the WinProc to handle base callbacks */
  iupwinChangeProc(ih, iupwinBaseWinProc);

  return 1;
}

char* iupwinGetClipboardText(Ihandle* ih)
{
  HANDLE Handle;
  char* str;

  if (!IsClipboardFormatAvailable(CF_TEXT))
    return NULL;

  if (!OpenClipboard(ih->handle))
    return NULL;

  Handle = GetClipboardData(CF_TEXT);
  if (!Handle)
  {
    CloseClipboard();
    return NULL;
  }
  
  str = (char*)GlobalLock(Handle);
  str = iupStrDup(str);
  
  GlobalUnlock(Handle);
  
  CloseClipboard();

  return str;
}
