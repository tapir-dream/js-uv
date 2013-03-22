#include "util.h"
#include "v8.h"
#include "uv.h"
#include "uvrun.h"
#include "init.h"

namespace UV {
	using namespace v8;

#define TYPE_ERROR(msg) \
	ThrowException(Exception::TypeError(String::New(msg)));



	static uv_idle_t idle_handle;
	v8::Handle<v8::Value> UvRun(const v8::Arguments& args);
	static void idle_cb(uv_idle_t* handle, int status);





	v8::Persistent<v8::Object> object_;
	Handle<Value> cb_function;
	static Persistent<String> oncomplete_sym;
	static void idle_cb(uv_idle_t* handle, int status) {
		if(cb_function->IsUndefined()){
			uv_idle_stop(handle);
			return ;
		}

		object_ = v8::Persistent<v8::Object>::New(v8::Object::New());
		if (oncomplete_sym.IsEmpty()) {
			oncomplete_sym = UV_PSYMBOL("oncomplete");
		}
		object_->Set(oncomplete_sym, cb_function); 
		Local<Value> argv[1]={String::New("This is a callback arguments")};
		MakeCallback(object_, oncomplete_sym, 1, argv);

		//uv_idle_stop(handle);
	}


	v8::Handle<v8::Value> UvRun(const v8::Arguments& args){
		HandleScope scope;

		uv_loop_t *loop=uv_default_loop();
		uv_run_mode mode=UV_RUN_DEFAULT;
		int len = args.Length();

		if(len>0){

			if(args[0]->IsInt32()){
				mode=uv_run_mode(args[0]->Int32Value());
			}else if(args[0]->IsFunction()) {
				cb_function=args[0];
			}else{
				ThrowException(Exception::TypeError(String::New("wrong arguments")));
				cb_function=v8::Undefined();
			}

			if(len==2&&args[1]->IsFunction()){
				cb_function=args[1];
			}else{
				ThrowException(Exception::TypeError(String::New("wrong arguments")));
				cb_function=v8::Undefined();
			}
		}

		uv_idle_init(uv_default_loop(), &idle_handle);
		uv_idle_start(&idle_handle, idle_cb);

		uv_run(loop, mode);
		return v8::Undefined();
	}

	v8::Handle<v8::Value> UvStop(const v8::Arguments& args){
		HandleScope scope;

		uv_idle_stop(&idle_handle);
		return v8::Undefined();
	}

	int RUN::Initialize(v8::Handle<v8::ObjectTemplate> target) {
		HandleScope scope;
		//UV_SET_METHOD(target, "uv_run", UvRun);
		target->Set(String::New("uv_run"), FunctionTemplate::New(UvRun));
		target->Set(String::New("uv_stop"), FunctionTemplate::New(UvStop));
		return 0;
	}


}

//int r=UV::RUN::Initialize( v8::Context::GetCurrent()->Global());
