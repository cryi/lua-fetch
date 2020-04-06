#ifdef _WIN32
  #define FETCH_EXPORT __declspec (dllexport)
#else
  #define FETCH_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

FETCH_EXPORT int luaopen_lfetch (lua_State *L);

#ifdef __cplusplus
}
#endif