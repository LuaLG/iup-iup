/** \file
 * \brief Bindig of iupcontrols to Lua 5.
 *
 * See Copyright Notice in iup.h
 * $Id: controls.c,v 1.3 2008-11-21 04:21:25 scuri Exp $
 */
 
#include <lua.h>
#include <lualib.h>

#include "iup.h"
#include "iupcontrols.h"

#include "iuplua.h"
#include "iupluacontrols.h"
#include "il.h"
#include "il_controls.h"

int iupcontrolslua_open(lua_State * L)
{
  if (iuplua_opencall_internal(L))
    IupControlsOpen();

  iuplua_changeEnv(L);

  iupgaugelua_open(L);
  iuptreelua_open(L);
  iupmatrixlua_open(L);
  iupmasklua_open(L);
  iupdiallua_open(L);
  iupcolorbrowserlua_open(L);
  iupcellslua_open(L);
  iupcolorbarlua_open(L);

#if (IUP_VERSION_NUMBER < 300000)
  iupgclua_open(L);
  iupgetparamlua_open(L);
  iupvallua_open(L);
  iuptabslua_open(L);
#endif

  iuplua_returnEnv(L);

  return 0;
}

int iupcontrolslua_close(lua_State * L)
{
  if (iuplua_opencall_internal(L))
    IupControlsClose();
  return 0;
}

/* obligatory to use require"iupluacontrols" */
int luaopen_iupluacontrols(lua_State* L)
{
  return iupcontrolslua_open(L);
}

/* obligatory to use require"iupluacontrols51" */
int luaopen_iupluacontrols51(lua_State* L)
{
  return iupcontrolslua_open(L);
}
