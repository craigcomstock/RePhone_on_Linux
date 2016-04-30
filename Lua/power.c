
#include "vmpwr.h"

#include "lua.h"
#include "lauxlib.h"

int power_battery_level(lua_State *L)
{
	int level = vm_pwr_get_battery_level();

	lua_pushnumber(L, level);
	
	return 1;
}

int power_reboot(lua_State *L)
{
	vm_pwr_reboot();

	return 1;
}

int power_shutdown(lua_State *L)
{
	vm_pwr_shutdown(0);
	return 1;
}

#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???? TODO
#include "lrodefs.h"

const LUA_REG_TYPE power_map[] =
{
	{LSTRKEY("battery"), LFUNCVAL(power_battery_level)},
	{LSTRKEY("reboot"), LFUNCVAL(power_reboot)},
	{LSTRKEY("shutdown"), LFUNCVAL(power_shutdown)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_power(lua_State *L)
{
	luaL_register(L, "power", power_map);
	return 1;
}
