/******************************************************************************
 * Automatically generated file (iuplua5). Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int button_action(Ihandle *self)
{
  lua_State *L = iuplua_call_start(self, "action");
  return iuplua_call(L, 0);
}

static int Button(lua_State *L)
{
  Ihandle *ih = IupButton((char *) luaL_optstring(L, 1, NULL), NULL);
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupbuttonlua_open(lua_State * L)
{
  iuplua_register(L, Button, "Button");

  iuplua_register_cb(L, "ACTION", (lua_CFunction)button_action, "button");

#ifdef IUPLUA_USELOH
#ifdef TEC_BIGENDIAN
#ifdef TEC_64
#include "loh/button_be64.loh"
#else
#include "loh/button_be32.loh"
#endif
#else
#ifdef TEC_64
#ifdef WIN64
#include "loh/button_le64w.loh"
#else
#include "loh/button_le64.loh"
#endif
#else
#include "loh/button.loh"
#endif
#endif
#else
  iuplua_dofile(L, "button.lua");
#endif

  return 0;
}

