/** \file
 * \brief Bindig of iupdial to Lua 3.
 *
 * See Copyright Notice in iup.h
 * $Id: luadial.c,v 1.1 2008-10-17 06:21:03 scuri Exp $
 */
 
#include <stdlib.h>

#include "iup.h"
#include "iupdial.h"

#include <lua.h>
#include <lauxlib.h>

#include "iuplua.h"
#include "il.h"
#include "il_controls.h"


static void CreateDial(void)
{
  int tag = (int)lua_getnumber(lua_getglobal("iuplua_tag"));
  lua_pushusertag(IupDial(luaL_check_string(1)), tag);
}

int diallua_open(void)
{
  lua_register("iupCreateDial", CreateDial);

#ifdef IUPLUA_USELOH
#ifdef TEC_BIGENDIAN
#ifdef TEC_64
#include "loh/luadial_be64.loh"
#else
#include "loh/luadial_be32.loh"
#endif  
#else
#ifdef TEC_64
#ifdef WIN64
#include "loh/luadial_le64w.loh"
#else
#include "loh/luadial_le64.loh"
#endif  
#else
#include "loh/luadial.loh"
#endif  
#endif  
#else
  iuplua_dofile("luadial.lua");
#endif

  return 1;
}
