/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#ifdef HILDON
#include <hildon/hildon-program.h>
#endif
                                         
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupgtk_drv.h"


static void gtkDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h);

/****************************************************************
                     Utilities
****************************************************************/

int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

void iupdrvDialogUpdateSize(Ihandle* ih)
{
  int width, height;
  gtk_window_get_size((GtkWindow*)ih->handle, &width, &height);
  ih->currentwidth = width;
  ih->currentheight = height;
}

void iupdrvDialogGetSize(InativeHandle* handle, int *w, int *h)
{
  int width, height;
  gtk_window_get_size((GtkWindow*)handle, &width, &height);
  if (w) *w = width;
  if (h) *h = height;
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  if (visible)
    gtk_widget_show(ih->handle);
  else
    gtk_widget_hide(ih->handle);
}

void iupdrvDialogGetPosition(InativeHandle* handle, int *x, int *y)
{
  gtk_window_get_position((GtkWindow*)handle, x, y);
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  gtk_window_move((GtkWindow*)ih->handle, x, y);
}

static int gtkDialogGetMenuSize(Ihandle* ih)
{
#ifdef HILDON
  return 0;
#else
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
#endif
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
#ifdef HILDON
  /* In Hildon, borders have fixed dimensions, but are drawn as part
     of the client area! */
  if (border)
    *border = (iupAttribGetInt(ih, "HILDONWINDOW") && !iupAttribGetInt(ih, "FULLSCREEN")) ? 12 : 0;
  if (caption)
    *caption = 0;
  if (menu)
    *menu = 0;
#else
  static int native_border = 0;
  static int native_caption = 0;

  int has_caption = iupAttribGetInt(ih, "MAXBOX")  ||
                    iupAttribGetInt(ih, "MINBOX")  ||
                    iupAttribGetInt(ih, "MENUBOX") || 
                    IupGetAttribute(ih, "TITLE"); /* must use IupGetAttribute to check from the native implementation */

  int has_border = has_caption ||
                   iupAttribGetInt(ih, "RESIZE") ||
                   iupAttribGetInt(ih, "BORDER");

  *menu = gtkDialogGetMenuSize(ih);

  if (ih->handle && iupdrvIsVisible(ih))
  {
    int win_border, win_caption;
    /* TODO: maybe we can use gdk_window_get_frame_extents to get a better decoration size 
    GdkRectangle rect;
    gdk_window_get_frame_extents(gtk_widget_get_window(ih->handle), &rect);  */

    if (iupdrvGetWindowDecor(iupgtkGetNativeWindowHandle(ih), &win_border, &win_caption))
    {
#ifdef WIN32
      if (*menu)
        win_caption -= *menu;
#endif

      *border = 0;
      if (has_border)
        *border = win_border;

      *caption = 0;
      if (has_caption)
        *caption = win_caption;

      if (!native_border && *border)
        native_border = win_border;

      if (!native_caption && *caption)
        native_caption = win_caption;
    }
  }

  /* I could not set the size of the window including the decorations when the dialog is hidden */
  /* So we have to estimate the size of borders and caption when the dialog is hidden           */

  *border = 0;
  if (has_border)
  {
    if (native_border)
      *border = native_border;
    else
      *border = 5;
  }

  *caption = 0;
  if (has_caption)
  {
    if (native_caption)
      *caption = native_caption;
    else
      *caption = 20;
  }
#endif
}

int iupdrvDialogSetPlacement(Ihandle* ih, int x, int y)
{
  char* placement;
  int old_state = ih->data->show_state;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetInt(ih, "FULLSCREEN"))
  {
    gtk_window_fullscreen((GtkWindow*)ih->handle);
    return 1;
  }
  
  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    gtk_window_unmaximize((GtkWindow*)ih->handle);
    gtk_window_deiconify((GtkWindow*)ih->handle);
    return 0;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    gtk_window_iconify((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    gtk_window_maximize((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* position the decoration outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    /* the dialog client area will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    height += menu; /* menu is inside the client area. */

    /* set the new size and position */
    /* The resize evt will update the layout */
    gtk_window_move((GtkWindow*)ih->handle, x, y);
    gtk_window_resize((GtkWindow*)ih->handle, width, height); 

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSetStr(ih, "PLACEMENT", NULL); /* reset to NORMAL */

  return 1;
}


/****************************************************************
                     Callbacks and Events
****************************************************************/


gboolean iupgtkDialogDeleteEvent(GtkWidget *widget, GdkEvent *evt, Ihandle *ih)
{
  Icallback cb;
  (void)widget;
  (void)evt;

  /* even when ACTIVE=NO the dialog gets this evt */
  if (!iupdrvIsActive(ih))
    return TRUE;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return TRUE;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih); /* default: close the window */

  return TRUE; /* do not propagate */
}

static gboolean gtkDialogConfigureEvent(GtkWidget *widget, GdkEventConfigure *evt, Ihandle *ih)
{
  int old_width, old_height;
  (void)widget;

#ifndef HILDON
  /* In hildon the menu is not a menubar */
  if (ih->data->menu && ih->data->menu->handle)
    gtk_widget_set_size_request(ih->data->menu->handle, evt->width, -1);
#endif

  if (ih->data->ignore_resize) return FALSE; 

  old_width = iupAttribGetInt(ih, "_IUPGTK_OLD_WIDTH");
  old_height = iupAttribGetInt(ih, "_IUPGTK_OLD_HEIGHT");

  /* Check the size change, because configure is called also for position changes */
  if (evt->width != old_width || evt->height != old_height)
  {
    IFnii cb;
    int border, caption, menu;
    iupAttribSetInt(ih, "_IUPGTK_OLD_WIDTH", evt->width);
    iupAttribSetInt(ih, "_IUPGTK_OLD_HEIGHT", evt->height);

    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

#ifdef HILDON
    /* In Hildon, the configure event contains the window size, not the client area size */
    ih->currentwidth = evt->width;
    ih->currentheight = evt->height;
#else
    ih->currentwidth = evt->width + 2*border;
    ih->currentheight = evt->height + 2*border + caption;  /* menu is inside the window client area */
#endif

    cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (cb) cb(ih, evt->width, evt->height - menu);  /* notify to the application size the client area size */

    ih->data->ignore_resize = 1; 
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }

  return FALSE;
}

static gboolean gtkDialogWindowStateEvent(GtkWidget *widget, GdkEventWindowState *evt, Ihandle *ih)
{
  int state = -1;
  (void)widget;

  if ((evt->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) &&        /* if flag changed and  */
      (evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&    /* is now set           */
      !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))     /* is visible           */
    state = IUP_MAXIMIZE;
  else if ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&
           (evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED) &&
           !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))
    state = IUP_MINIMIZE;
  else if ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&
           (evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&
           !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))
    state = IUP_MAXIMIZE;
  else if (((evt->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) &&       /* maximized changed */
            !(evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&  /* not maximized     */
            !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) &&  /* is visible        */
            !(evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED))    /* not minimized     */
            ||                                                       /* OR                     */
           ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&       /* minimized changed   */
            !(evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED) &&  /* not minimized       */
            !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) &&  /* is visible          */
            !(evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)))   /* not maximized       */
    state = IUP_RESTORE;

  if (state < 0)
    return FALSE;

  if (ih->data->show_state != state)
  {
    IFni cb;
    ih->data->show_state = state;

    cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (cb && cb(ih, state) == IUP_CLOSE) 
      IupExitLoop();
  }

  return FALSE;
}

static gboolean gtkDialogChildDestroyEvent(GtkWidget *widget, Ihandle *ih)
{
  /* It seems that the documentation for this callback is not correct */
  /* The second parameter must be the user_data or it will fail. */
  (void)widget;

  /* If the IUP dialog was not destroyed, destroy it here. */
  if (iupObjectCheck(ih))
    IupDestroy(ih);

  /* this callback is usefull to destroy children dialogs when the parent is destroyed. */
  /* The application is responsable for destroying the children before this happen. */

  return FALSE;
}


/****************************************************************
                     Idialog Methods
****************************************************************/


/* replace the common dialog SetPosition method because of 
   the menu that it is inside the dialog. */
static void gtkDialogSetPositionMethod(Ihandle* ih, int x, int y)
{
  /* x and y are always 0 for the dialog. */
  ih->x = x;
  ih->y = y;

  if (ih->firstchild)
  {
    int menu = gtkDialogGetMenuSize(ih);

    /* Child coordinates are relative to client left-top corner. */
    iupClassObjectSetPosition(ih->firstchild, 0, menu);
  }
}

static void* gtkDialogGetInnerNativeContainerMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return (void*)gtk_bin_get_child((GtkBin*)ih->handle);
}

static int gtkDialogMapMethod(Ihandle* ih)
{
  int decorations = 0;
  int functions = 0;
  InativeHandle* parent;
  GtkWidget* fixed;

#ifdef HILDON
  if (iupAttribGetInt(ih, "HILDONWINDOW")) 
  {
    HildonProgram *program = HILDON_PROGRAM(hildon_program_get_instance());
    ih->handle = hildon_window_new();
    if (ih->handle)
      hildon_program_add_window(program, HILDON_WINDOW(ih->handle)); 
  } 
  else 
  {
    iupAttribSetStr(ih, "DIALOGHINT", "YES");  /* otherwise not displayed correctly */ 
    ih->handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  }
#else
  ih->handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif
  if (!ih->handle)
    return IUP_ERROR;

  parent = iupDialogGetNativeParent(ih);
  if (parent)
  {
    gtk_window_set_transient_for((GtkWindow*)ih->handle, (GtkWindow*)parent);

    /* manually remove child windows when parent is destroyed */
    g_signal_connect(G_OBJECT(parent), "destroy", G_CALLBACK(gtkDialogChildDestroyEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  /* The iupgtkKeyPressEvent of the control with the focus will propagate the key up to the dialog. */
  /* Inside iupgtkKeyPressEvent we test this to avoid duplicate calls. */
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle), "configure-event",    G_CALLBACK(gtkDialogConfigureEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "window-state-event", G_CALLBACK(gtkDialogWindowStateEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "delete-event",       G_CALLBACK(iupgtkDialogDeleteEvent), ih);
                                    
  gtk_window_set_default_size((GtkWindow*)ih->handle, 100, 100); /* set this to avoid size calculation problems  */

  if (iupStrBoolean(iupAttribGet(ih, "DIALOGHINT"))) 
    gtk_window_set_type_hint(GTK_WINDOW(ih->handle), GDK_WINDOW_TYPE_HINT_DIALOG);

  /* the container that will receive the child element. */
  fixed = gtk_fixed_new();
  gtk_container_add((GtkContainer*)ih->handle, fixed);
  gtk_widget_show(fixed);

  /* initialize the widget */
  gtk_widget_realize(ih->handle);

  if (iupAttribGetInt(ih, "DIALOGFRAME")) {
    iupAttribSetStr(ih, "RESIZE", "NO");
  }

  if (!iupAttribGetInt(ih, "RESIZE")) {
    iupAttribSetStr(ih, "MAXBOX", "NO");  /* Must also remove these, so RESIZE=NO can work */
    iupAttribSetStr(ih, "MINBOX", "NO");
  }

  if (IupGetAttribute(ih, "TITLE")) {  /* must use IupGetAttribute to check from the native implementation */
    functions   |= GDK_FUNC_MOVE;
    decorations |= GDK_DECOR_TITLE;
  }

  if (iupAttribGetInt(ih, "MENUBOX")) {
    functions   |= GDK_FUNC_CLOSE;
    decorations |= GDK_DECOR_MENU;
  }

  if (iupAttribGetInt(ih, "MINBOX")) {
    functions   |= GDK_FUNC_MINIMIZE;
    decorations |= GDK_DECOR_MINIMIZE;
  }

  if (iupAttribGetInt(ih, "MAXBOX")) {
    functions   |= GDK_FUNC_MAXIMIZE;
    decorations |= GDK_DECOR_MAXIMIZE;
  }

  if (iupAttribGetInt(ih, "RESIZE")) {
    functions   |= GDK_FUNC_RESIZE;
    decorations |= GDK_DECOR_RESIZEH;
  }

  if (iupAttribGetInt(ih, "BORDER"))
    decorations |= GDK_DECOR_BORDER;

  if (decorations == 0)
    gtk_window_set_decorated((GtkWindow*)ih->handle, FALSE);
  else
  {
    GdkWindow* window = ih->handle->window;
    if (window)
    {
      gdk_window_set_decorations(window, (GdkWMDecoration)decorations);
      gdk_window_set_functions(window, (GdkWMFunction)functions);
    }
  }

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSetStr(ih, "DRAGDROP", "YES");

  {
    /* Reset the DLGBGCOLOR global attribute 
       if it is the first time a dialog is created. 
       The value returned by gtk_style_new is not accurate. */
    GtkStyle* style = gtk_widget_get_style(ih->handle);
    if (style && IupGetGlobal("_IUP_RESET_DLGBGCOLOR"))
    {
      iupgtkUpdateGlobalColors(style);
      IupSetGlobal("_IUP_RESET_DLGBGCOLOR", NULL);
    }
  }

  /* configure the size range */
  gtkDialogSetMinMax(ih, 1, 1, 65535, 65535);  /* MINSIZE and MAXSIZE default values */

  /* Ignore VISIBLE before mapping */
  iupAttribSetStr(ih, "VISIBLE", NULL);

  return IUP_NOERROR;
}

static void gtkDialogUnMapMethod(Ihandle* ih)
{
  GtkWidget* fixed;
#if GTK_CHECK_VERSION(2, 10, 0)
  GtkStatusIcon* status_icon;
#endif

  if (ih->data->menu) 
  {
    ih->data->menu->handle = NULL; /* the dialog will destroy the native menu */
    IupDestroy(ih->data->menu);  
  }

#if GTK_CHECK_VERSION(2, 10, 0)
  status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPDLG_STATUSICON");
  if (status_icon)
    g_object_unref(status_icon);
#endif

  fixed = gtk_bin_get_child((GtkBin*)ih->handle);
  gtk_widget_unrealize(fixed);
  gtk_widget_destroy(fixed);  

  gtk_widget_unrealize(ih->handle); /* To match the call to gtk_widget_realize */
  gtk_widget_destroy(ih->handle);   /* To match the call to gtk_window_new     */
}

static void gtkDialogLayoutUpdateMethod(Ihandle *ih)
{
  int border, caption, menu;
  int width, height;

  if (ih->data->ignore_resize ||
      iupAttribGet(ih, "_IUPGTK_FS_STYLE"))
    return;

  /* for dialogs the position is not updated here */
  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  /* set size excluding the border */
  width = ih->currentwidth - 2*border;
  height = ih->currentheight - 2*border - caption;   /* menu is inside the client area. */
  gtk_window_resize((GtkWindow*)ih->handle, width, height);

  if (!iupAttribGetInt(ih, "RESIZE"))
  {
    GdkGeometry geometry;
    geometry.min_width = width;
    geometry.min_height = height;
    geometry.max_width = width;
    geometry.max_height = height;
    gtk_window_set_geometry_hints((GtkWindow*)ih->handle, ih->handle,
                                  &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
  }

  ih->data->ignore_resize = 0;
}


/****************************************************************************
                                   Attributes
****************************************************************************/

static void gtkDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  /* The minmax size restricts the client area */
  GdkGeometry geometry;
  int decorwidth = 0, decorheight = 0;
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  geometry.min_width = 1;
  if (min_w > decorwidth)
    geometry.min_width = min_w-decorwidth;

  geometry.min_height = 1;
  if (min_h > decorheight)
    geometry.min_height = min_h-decorheight;

  geometry.max_width = 65535;
  if (max_w > decorwidth && max_w > geometry.min_width)
    geometry.max_width = max_w-decorwidth;

  geometry.max_height = 65535;
  if (max_h > decorheight && max_w > geometry.min_height)
    geometry.max_height = max_h-decorheight;

  gtk_window_set_geometry_hints((GtkWindow*)ih->handle, ih->handle,
                                &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

static int gtkDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  /* if MAXSIZE also set, must be also updated here */
  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  gtkDialogSetMinMax(ih, min_w, min_h, max_w, max_h);
  return 1;
}

static int gtkDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  /* if MINSIZE also set, must be also updated here */
  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  gtkDialogSetMinMax(ih, min_w, min_h, max_w, max_h);
  return 1;
}

static char* gtkDialogGetXAttrib(Ihandle *ih)
{
  char* str = iupStrGetMemory(20);
 
  gint x = 0;
  gtk_window_get_position((GtkWindow*)ih->handle, &x, NULL);

  sprintf(str, "%d", x);
  return str;
}

static char* gtkDialogGetYAttrib(Ihandle *ih)
{
  char* str = iupStrGetMemory(20);
 
  gint y = 0;
  gtk_window_get_position((GtkWindow*)ih->handle, NULL, &y);

  sprintf(str, "%d", y);
  return str;
}

static int gtkDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = "";
  gtk_window_set_title((GtkWindow*)ih->handle, iupgtkStrConvertToUTF8(value));
  return 0;
}

static char* gtkDialogGetTitleAttrib(Ihandle* ih)
{
  const char* title = gtk_window_get_title((GtkWindow*)ih->handle);

  if (!title || title[0] == 0)
    return NULL;
  else
    return iupStrGetMemoryCopy(iupgtkStrConvertFromUTF8(title));
}    

static char* gtkDialogGetClientSizeAttrib(Ihandle *ih)
{
  char* str = iupStrGetMemory(20);
 
  int width, height;
  gtk_window_get_size((GtkWindow*)ih->handle, &width, &height);
  height -= gtkDialogGetMenuSize(ih);

  sprintf(str, "%dx%d", width, height);
  return str;
}

static int gtkDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{                       
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPGTK_FS_STYLE"))
    {
      /* save the previous decoration attributes */
      /* during fullscreen these attributes can be consulted by the application */
      iupAttribStoreStr(ih, "_IUPGTK_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribStoreStr(ih, "_IUPGTK_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribStoreStr(ih, "_IUPGTK_FS_MENUBOX",iupAttribGet(ih, "MENUBOX"));
      iupAttribStoreStr(ih, "_IUPGTK_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribStoreStr(ih, "_IUPGTK_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribStoreStr(ih, "_IUPGTK_FS_TITLE",  IupGetAttribute(ih, "TITLE"));  /* must use IupGetAttribute to check from the native implementation */

      /* remove the decorations attributes */
      iupAttribSetStr(ih, "MAXBOX", "NO");
      iupAttribSetStr(ih, "MINBOX", "NO");
      iupAttribSetStr(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL); iupAttribSetStr(ih, "TITLE", NULL); /* remove from the hash table if we are during IupMap */
      iupAttribSetStr(ih, "RESIZE", "NO");
      iupAttribSetStr(ih, "BORDER", "NO");

      if (iupdrvIsVisible(ih))
        gtk_window_fullscreen((GtkWindow*)ih->handle);

      iupAttribSetStr(ih, "_IUPGTK_FS_STYLE", "YES");
    }
  }
  else
  {
    char* fs_style = iupAttribGet(ih, "_IUPGTK_FS_STYLE");
    if (fs_style)
    {
      iupAttribSetStr(ih, "_IUPGTK_FS_STYLE", NULL);

      /* restore the decorations attributes */
      iupAttribStoreStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPGTK_FS_MAXBOX"));
      iupAttribStoreStr(ih, "MINBOX", iupAttribGet(ih, "_IUPGTK_FS_MINBOX"));
      iupAttribStoreStr(ih, "MENUBOX",iupAttribGet(ih, "_IUPGTK_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE",  iupAttribGet(ih, "_IUPGTK_FS_TITLE"));   /* must use IupSetAttribute to update the native implementation */
      iupAttribStoreStr(ih, "RESIZE", iupAttribGet(ih, "_IUPGTK_FS_RESIZE"));
      iupAttribStoreStr(ih, "BORDER", iupAttribGet(ih, "_IUPGTK_FS_BORDER"));

      if (iupdrvIsVisible(ih))
        gtk_window_unfullscreen((GtkWindow*)ih->handle);

      /* remove auxiliar attributes */
      iupAttribSetStr(ih, "_IUPGTK_FS_MAXBOX", NULL);
      iupAttribSetStr(ih, "_IUPGTK_FS_MINBOX", NULL);
      iupAttribSetStr(ih, "_IUPGTK_FS_MENUBOX",NULL);
      iupAttribSetStr(ih, "_IUPGTK_FS_RESIZE", NULL);
      iupAttribSetStr(ih, "_IUPGTK_FS_BORDER", NULL);
      iupAttribSetStr(ih, "_IUPGTK_FS_TITLE",  NULL);
    }
  }
  return 1;
}

static int gtkDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    gtk_window_set_keep_above((GtkWindow*)ih->handle, TRUE);
  else
    gtk_window_set_keep_above((GtkWindow*)ih->handle, FALSE);
  return 1;
}

static int gtkDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  if (!value)
    gtk_window_set_icon((GtkWindow*)ih->handle, NULL);
  else
  {
    GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
    if (icon)
      gtk_window_set_icon((GtkWindow*)ih->handle, icon);
  }
  return 1;
}

static int gtkDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (iupdrvBaseSetBgColorAttrib(ih, value))
  {
    GtkStyle *style = gtk_widget_get_style(ih->handle);
    if (style->bg_pixmap[GTK_STATE_NORMAL])
    {
      style = gtk_style_copy(style);
      style->bg_pixmap[GTK_STATE_NORMAL] = NULL;
      gtk_widget_set_style(ih->handle, style);
    }
    return 1;
  }
  else
  {
    GdkPixbuf* pixbuf = iupImageGetImage(value, ih, 0);
    if (pixbuf)
    {
      GdkPixmap* pixmap;
      GtkStyle *style;

      gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap, NULL, 255);

      style = gtk_style_copy(gtk_widget_get_style(ih->handle));
      style->bg_pixmap[GTK_STATE_NORMAL] = pixmap;
      gtk_widget_set_style(ih->handle, style);

      return 1;
    }
  }

  return 0;
}

#if GTK_CHECK_VERSION(2, 10, 0)
static int gtkDialogTaskDoubleClick(int button)
{
  static int last_button = -1;
  static GTimer* timer = NULL;
  if (last_button == -1 || last_button != button)
  {
    last_button = button;
    if (timer)
      g_timer_destroy(timer);
    timer = g_timer_new();
    return 0;
  }
  else
  {
    double seconds;

    if (!timer)  /* just in case */
      return 0;

    seconds = g_timer_elapsed(timer, NULL);
    if (seconds < 0.4)
    {
      /* reset state */
      g_timer_destroy(timer);
      timer = NULL;
      last_button = -1;  
      return 1;
    }
    else
    {
      g_timer_reset(timer);
      return 0;
    }
  }
}

static void gtkDialogTaskAction(GtkStatusIcon *status_icon, Ihandle *ih)
{
  /* from GTK source code it is called only when button==1 and pressed==1 */
  int button = 1;
  int pressed = 1;
  int dclick = gtkDialogTaskDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();
  (void)status_icon;
}

static void gtkDialogTaskPopupMenu(GtkStatusIcon *status_icon, guint gbutton, guint activate_time, Ihandle *ih)
{
  /* from GTK source code it is called only when button==3 and pressed==1 */
  int button = 3;
  int pressed = 1;
  int dclick = gtkDialogTaskDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();
  (void)activate_time;
  (void)gbutton;
  (void)status_icon;
}

static GtkStatusIcon* gtkDialogGetStatusIcon(Ihandle *ih)
{
  GtkStatusIcon* status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPDLG_STATUSICON");
  if (!status_icon)
  {
    status_icon = gtk_status_icon_new();

    g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(gtkDialogTaskAction), ih);
    g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(gtkDialogTaskPopupMenu), ih);

    iupAttribSetStr(ih, "_IUPDLG_STATUSICON", (char*)status_icon);
  }
  return status_icon;
}

static int gtkDialogSetTrayAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
  gtk_status_icon_set_visible(status_icon, iupStrBoolean(value));
  return 1;
}

static int gtkDialogSetTrayTipAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
#if GTK_CHECK_VERSION(2, 16, 0)
  if (value)
  {
    gtk_status_icon_set_has_tooltip(status_icon, TRUE);
    gtk_status_icon_set_tooltip_text(status_icon, value);
  }
  else
    gtk_status_icon_set_has_tooltip(status_icon, FALSE);
#else
  gtk_status_icon_set_tooltip(status_icon, value);
#endif
  return 1;
}

static int gtkDialogSetTrayImageAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
  GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
  gtk_status_icon_set_from_pixbuf(status_icon, icon);
  return 1;
}
#endif  /* GTK_CHECK_VERSION(2, 10, 0) */

void iupdrvDialogInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = gtkDialogMapMethod;
  ic->UnMap = gtkDialogUnMapMethod;
  ic->LayoutUpdate = gtkDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtkDialogGetInnerNativeContainerMethod;
  ic->SetPosition = gtkDialogSetPositionMethod;

  /* Callback Windows and GTK Only */
  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");

  /* Driver Dependent Attribute functions */

#ifdef WIN32                                 
  iupClassRegisterAttribute(ic, "HWND", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
#else
  iupClassRegisterAttribute(ic, "XWINDOW", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);
#endif

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);  /* force new default value */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "X", gtkDialogGetXAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "Y", gtkDialogGetYAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", gtkDialogGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", gtkDialogGetTitleAttrib, gtkDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, gtkDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, gtkDialogSetIconAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, gtkDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, gtkDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, gtkDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);  /* saveunder not supported in GTK */

  /* IupDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, gtkDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, iupgtkSetDragDropAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#if GTK_CHECK_VERSION(2, 10, 0)
  iupClassRegisterAttribute(ic, "TRAY", NULL, gtkDialogSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, gtkDialogSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, gtkDialogSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
}
