#include "uv.h"
#include "v8.h"

namespace UV {
#ifdef _WIN32
# ifndef BUILDING_EXTENSION
#   define UV_EXTERN __declspec(dllexport)
# else
#   define UV_EXTERN __declspec(dllimport)
# endif
#else
# define UV_EXTERN /* nothing */
#endif

#ifdef BUILDING_EXTENSION
# undef BUILDING_V8_SHARED
# undef BUILDING_UV_SHARED
# define USING_V8_SHARED 1
# define USING_UV_SHARED 1
#endif

#ifndef UV_STRINGIFY
#define UV_STRINGIFY(n) UV_STRINGIFY_HELPER(n)
#define UV_STRINGIFY_HELPER(n) #n
#endif


#define UV_PSYMBOL(s) \
	v8::Persistent<v8::String>::New(v8::String::NewSymbol(s))

//#define UV_SET_METHOD UV::SetMethod
//	void SetMethod(v8::Handle<v8::Object> obj, const char* name,
//		v8::InvocationCallback callback)
//	{
//		obj->Set(v8::String::NewSymbol(name),
//			v8::FunctionTemplate::New(callback)->GetFunction());
//	}

//
//	UV_EXTERN typedef void (* addon_register_func)(
//		v8::Handle<v8::Object> exports, v8::Handle<v8::Value> module);
//	struct module_struct {
//		int version;
//		void *dso_handle;
//		const char *filename;
//		UV::addon_register_func register_func;
//		const char *modname;
//	};
//#define UV_MODULE_VERSION 0x0001 /* v0.1 */
//
//#define UV_STANDARD_MODULE_STUFF \
//	UV_MODULE_VERSION,     \
//	NULL,                    \
//	__FILE__
//
//#ifdef _WIN32
//# define UV_MODULE_EXPORT __declspec(dllexport)
//#else
//# define UV_MODULE_EXPORT /* empty */
//#endif
//
//#define UV_MODULE(modname, regfunc)                                 \
//	extern "C" {                                                        \
//	UV_MODULE_EXPORT UV::module_struct modname ## _module =  \
//	{                                                                 \
//	UV_STANDARD_MODULE_STUFF,                                     \
//	(UV::addon_register_func)regfunc,                             \
//	UV_STRINGIFY(modname)                                         \
//	};                                                                \
//	}
//
//
//
//
//	template <typename target_t>
//	void SetMethod(target_t obj, const char* name,
//		v8::InvocationCallback callback){
//			obj->Set(v8::String::NewSymbol(name),
//				v8::FunctionTemplate::New(callback)->GetFunction());
//	}
//
//
//	template <typename target_t>
//	void SetPrototypeMethod(target_t target,
//		const char* name, v8::InvocationCallback callback)
//	{
//		v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(callback);
//		target->PrototypeTemplate()->Set(v8::String::NewSymbol(name), templ);
//	}
//
//
//#define SET_METHOD SetMethod
//#define SET_PROTOTYPE_METHOD SetPrototypeMethod
//




	
}