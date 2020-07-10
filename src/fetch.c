#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <fetch.h>
#include "mbedtls.h"

#define FETCH_IO_METATABLE "FETCH_IO*"
#define FETCH_METATABLE "FETCH_IO*"

#ifdef LIBFETCH_WITH_MBEDTLS
#include "mbedtls.h"
#endif

struct LFETCH_IO {
    fetchIO *io;
    int closed;
    unsigned char *url;
};

int l_fetch(lua_State *L, char method)
{
    const unsigned char *url = luaL_checkstring(L, 1);
    const unsigned char *flags = lua_tostring(L, 2);
    fetchIO *io = NULL;
    if (method == 'g')
    {
        io = fetchGetURL(url, flags);
    }
    else if (method == 'p')
    {
        io = fetchPutURL(url, flags);
    }
    if (io == NULL)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Failed to fetch '%s' - %d (%s)", url, fetchLastErrCode, fetchLastErrString);
        lua_pushinteger(L, fetchLastErrCode);
        return 3;
    }
    
    struct LFETCH_IO * fio = lua_newuserdata(L, sizeof(struct LFETCH_IO));
    fio->url = malloc(strlen(url) + 1); 
    strcpy(fio->url, url);
    fio->io = io;
    fio->closed = 0;

    luaL_getmetatable(L, FETCH_IO_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

int l_get(lua_State *L)
{
    return l_fetch(L, 'g');
}

int l_put(lua_State *L)
{
    return l_fetch(L, 'p');
}

#ifdef LIBFETCH_WITH_MBEDTLS
int l_set_tls_option(lua_State *L) {
    const char *_option = luaL_checkstring(L, 1);
    // no point to bother with switch for now
    if (strcmp(_option, "mtu") == 0) 
    {
        uint16_t _mtu = (uint16_t)luaL_checkinteger(L, 2);
        mbedtls_set_mtu(_mtu);
        return 0;
    } 
    else if (strcmp(_option, "readTimeout") == 0)
    {
        uint32_t _timeout = (uint32_t)luaL_checkinteger(L, 2);
        mbedtls_set_read_timeout(_timeout);
        return 0;
    }
    else    
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Unknown tls option '%s'!", _option);
        return 2;
    }
}
#endif

int l_read(lua_State *L)
{
    struct LFETCH_IO *fio = lua_touserdata(L, 1);
    if (fio->closed != 0)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Trying to read closed FETCH_IO*");
        return 2;
    }
    size_t len = (size_t)luaL_checkinteger(L, 2);
    unsigned char *output = malloc(len * sizeof(char));
    len = fetchIO_read(fio->io, output, len);
    if (len < 0)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Failed to fetch '%s' - %d (%s)", fio->url, fetchLastErrCode, fetchLastErrString);
        lua_pushinteger(L, fetchLastErrCode);
        return 3;
    }
    lua_pushlstring(L, output, len);
    return 1;
}

int l_write(lua_State *L)
{
    struct LFETCH_IO *fio = lua_touserdata(L, 1);
    size_t len;
    if (fio->closed != 0) 
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Trying to write closed FETCH_IO*");
        return 2;      
    }
    const unsigned char *toWrite = luaL_checklstring(L, 2, &len);
    len = fetchIO_write(fio->io, toWrite, len);
    if (len < 0)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "Failed to fetch '%s' - %d (%s)", fio->url, fetchLastErrCode, fetchLastErrString);
        lua_pushinteger(L, fetchLastErrCode);
        return 3;
    }
    return 0;
}

int l_close(lua_State *L)
{
    struct LFETCH_IO *fio = lua_touserdata(L, 1);
    if (fio->closed == 0) 
    {
        free(fio->url);
        fetchIO_close(fio->io);
        fio->closed = 1;
    }
    return 0;
}

static const struct luaL_Reg lua_fetch[] = {
    {"get", l_get},
    {"put", l_put},
#ifdef LIBFETCH_WITH_MBEDTLS
    {"set_tls_option", l_set_tls_option},
#endif

    {NULL, NULL}};

int fetchio_create_meta(lua_State *L)
{
    luaL_newmetatable(L, FETCH_IO_METATABLE);

    /* Method table */
    lua_newtable(L);
    lua_pushcfunction(L, l_read);
    lua_setfield(L, -2, "read");
    lua_pushcfunction(L, l_write);
    lua_setfield(L, -2, "write");
    lua_pushcfunction(L, l_close);
    lua_setfield(L, -2, "close");

    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_close);
    lua_setfield(L, -2, "__gc");
    return 1;
}

#ifdef LIBFETCH_WITH_MBEDTLS
int luaclose_lfetch(lua_State *L)
{
    fetch_mbedtls_close();
    return 0;
}

int fetch_create_meta(lua_State *L)
{
    luaL_newmetatable(L, FETCH_METATABLE);

    lua_pushcfunction(L, luaclose_lfetch);
    lua_setfield(L, -2, "__gc");
    return 1;
}
#endif

int luaopen_lfetch(lua_State *L)
{
    fetchio_create_meta(L);
    lua_newtable(L);
    luaL_setfuncs(L, lua_fetch, 0);
#ifdef LIBFETCH_WITH_MBEDTLS
    fetch_create_meta(L);
    lua_setmetatable(L, -2);
#endif
    return 1;
}
