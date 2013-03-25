#include "util.h"
#include "v8.h"
#include "uv.h"
#include "fs.h"
#include "init.h"
#include "req_wrap.h"
#include <fcntl.h>
#include <map>

std::map < std::string, int > FlagMaps; 

void map_insert(std::map < std::string, int  > *maps, std::string key, int x) 
{ 
	maps->insert(std::map < std::string, int  >::value_type(key, x)); 
} 

#ifndef O_SYNC
#define O_SYNC 0
#endif

int makFlagMaps(){

	map_insert(&FlagMaps,"r" ,O_RDONLY);
	map_insert(&FlagMaps,"rs" ,O_RDONLY | O_SYNC);

	map_insert(&FlagMaps,"r+" ,O_RDWR);
	map_insert(&FlagMaps,"rs+" ,O_RDWR | O_SYNC);
	map_insert(&FlagMaps,"w" ,O_TRUNC | O_CREAT | O_WRONLY);
	map_insert(&FlagMaps,"wx" ,O_TRUNC | O_CREAT | O_WRONLY | O_EXCL);
	map_insert(&FlagMaps,"xw" ,O_TRUNC | O_CREAT | O_WRONLY | O_EXCL);

	map_insert(&FlagMaps,"w+" ,O_TRUNC | O_CREAT | O_RDWR);
	map_insert(&FlagMaps,"wx+",O_TRUNC | O_CREAT | O_RDWR | O_EXCL);
	map_insert(&FlagMaps,"xw+",O_TRUNC | O_CREAT | O_RDWR | O_EXCL);

	map_insert(&FlagMaps,"a" ,O_APPEND | O_CREAT | O_WRONLY);
	map_insert(&FlagMaps,"ax" ,O_APPEND | O_CREAT | O_WRONLY | O_EXCL);
	map_insert(&FlagMaps,"xa" ,O_APPEND | O_CREAT | O_WRONLY | O_EXCL);

	map_insert(&FlagMaps,"a+" ,O_APPEND | O_CREAT | O_RDWR);
	map_insert(&FlagMaps,"ax+",O_APPEND | O_CREAT | O_RDWR | O_EXCL);
	map_insert(&FlagMaps,"xa+",O_APPEND | O_CREAT | O_RDWR | O_EXCL);
	return 0;
}
int _r=makFlagMaps();//todo js2c 在js里处理flag参数

int getFlagByKey(std::string key){
	int result=	FlagMaps[key];
	return result;
}



namespace UV {
	using namespace v8;

	//===========声明============
	static Persistent<String> oncomplete_sym;




	// This struct is only used on sync fs calls.
	// For async calls FSReqWrap is used.
	struct fs_req_wrap {
		fs_req_wrap() {}
		~fs_req_wrap() { uv_fs_req_cleanup(&req); }
		// Ensure that copy ctor and assignment operator are not used.
		fs_req_wrap(const fs_req_wrap& req);
		fs_req_wrap& operator=(const fs_req_wrap& req);
		uv_fs_t req;
	};


	class FSReqWrap: public ReqWrap<uv_fs_t> {
	public:
		FSReqWrap(const char* syscall)
			: syscall_(syscall) {
		}

		const char* syscall() { return syscall_; }

	private:
		const char* syscall_;
	};



	//===========声明 结束============



	//===========宏定义=============
#define ASYNC_CALL(func, callback, ...)                           \
	FSReqWrap* req_wrap = new FSReqWrap(#func);                     \
	int r = uv_fs_##func(uv_default_loop(), &req_wrap->req_,        \
	__VA_ARGS__, After);                                        \
	req_wrap->object_->Set(oncomplete_sym, callback);               \
	req_wrap->Dispatched();                                         \
	if (r < 0) {                                                    \
	uv_fs_t* req = &req_wrap->req_;                               \
	req->result = r;                                              \
	req->path = NULL;                                             \
	req->errorno = uv_last_error(uv_default_loop()).code;         \
	After(req);                                                   \
	}                                                               \
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);\
	return scope.Close(req_wrap->object_);

#define SYNC_CALL(func, path, ...)                                \
	fs_req_wrap req_wrap;                                           \
	int result = uv_fs_##func(uv_default_loop(), &req_wrap.req, __VA_ARGS__, NULL); \
	if (result < 0) {                                               \
	int code = uv_last_error(uv_default_loop()).code;             \
	return ThrowException(UVException(code, #func, "", path));    \
	}\
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

#define SYNC_REQ req_wrap.req

#define SYNC_RESULT result

	//===========宏定义 结束=============






	static Persistent<FunctionTemplate> stats_constructor_template;

	static Persistent<String> dev_symbol;
	static Persistent<String> ino_symbol;
	static Persistent<String> mode_symbol;
	static Persistent<String> nlink_symbol;
	static Persistent<String> uid_symbol;
	static Persistent<String> gid_symbol;
	static Persistent<String> rdev_symbol;
	static Persistent<String> size_symbol;
	static Persistent<String> blksize_symbol;
	static Persistent<String> blocks_symbol;
	static Persistent<String> atime_symbol;
	static Persistent<String> mtime_symbol;
	static Persistent<String> ctime_symbol;

	Local<Object> BuildStatsObject(const uv_statbuf_t* s) {
		HandleScope scope;

		if (dev_symbol.IsEmpty()) {
			dev_symbol = UV_PSYMBOL("dev");
			ino_symbol = UV_PSYMBOL("ino");
			mode_symbol = UV_PSYMBOL("mode");
			nlink_symbol = UV_PSYMBOL("nlink");
			uid_symbol = UV_PSYMBOL("uid");
			gid_symbol = UV_PSYMBOL("gid");
			rdev_symbol = UV_PSYMBOL("rdev");
			size_symbol = UV_PSYMBOL("size");
			blksize_symbol = UV_PSYMBOL("blksize");
			blocks_symbol = UV_PSYMBOL("blocks");
			atime_symbol = UV_PSYMBOL("atime");
			mtime_symbol = UV_PSYMBOL("mtime");
			ctime_symbol = UV_PSYMBOL("ctime");
		}

		Local<Object> stats =
			stats_constructor_template->GetFunction()->NewInstance();

		if (stats.IsEmpty()) return Local<Object>();

#define X(name)                                                               \
		{                                                                           \
		Local<Value> val = Integer::New(s->st_##name);                            \
		if (val.IsEmpty()) return Local<Object>();                                \
		stats->Set(name##_symbol, val);                                           \
		}
		X(dev)
			X(mode)
			X(nlink)
			X(uid)
			X(gid)
			X(rdev)
# if defined(__POSIX__)
			X(blksize)
# endif
#undef X

#define X(name)                                                               \
		{                                                                           \
		Local<Value> val = Number::New(static_cast<double>(s->st_##name));        \
		if (val.IsEmpty()) return Local<Object>();                                \
		stats->Set(name##_symbol, val);                                           \
		}
			X(ino)
			X(size)
# if defined(__POSIX__)
			X(blocks)
# endif
#undef X

#define X(name)                                                               \
		{                                                                           \
		Local<Value> val = UV_UNIXTIME_V8(s->st_##name);                        \
		if (val.IsEmpty()) return Local<Object>();                                \
		stats->Set(name##_symbol, val);                                           \
		}
			X(atime)
			X(mtime)
			X(ctime)
#undef X

			return scope.Close(stats);
	}
	static void After(uv_fs_t *req) {
		HandleScope scope;

		FSReqWrap* req_wrap = (FSReqWrap*) req->data;
		assert(&req_wrap->req_ == req);

		// there is always at least one argument. "error"
		int argc = 1;

		// Allocate space for two args. We may only use one depending on the case.
		// (Feel free to increase this if you need more)
		Local<Value> argv[2];

		// NOTE: This may be needed to be changed if something returns a -1
		// for a success, which is possible.
		if (req->result == -1) {
			// If the request doesn't have a path parameter set.

			if (!req->path) {
				argv[0] = UVException(req->errorno,
					NULL,
					req_wrap->syscall());
			} else {
				argv[0] = UVException(req->errorno,
					NULL,
					req_wrap->syscall(),
					static_cast<const char*>(req->path));
			}
		} else {
			// error value is empty or null for non-error.
			argv[0] = Local<Value>::New(Null());

			// All have at least two args now.
			argc = 2;

			switch (req->fs_type) {
				// These all have no data to pass.
			case UV_FS_CLOSE:
			case UV_FS_RENAME:
			case UV_FS_UNLINK:
			case UV_FS_RMDIR:
			case UV_FS_MKDIR:
			case UV_FS_FTRUNCATE:
			case UV_FS_FSYNC:
			case UV_FS_FDATASYNC:
			case UV_FS_LINK:
			case UV_FS_SYMLINK:
			case UV_FS_CHMOD:
			case UV_FS_FCHMOD:
			case UV_FS_CHOWN:
			case UV_FS_FCHOWN:
				// These, however, don't.
				argc = 1;
				break;

			case UV_FS_UTIME:
			case UV_FS_FUTIME:
				argc = 0;
				break;

			case UV_FS_OPEN:
				argv[1] = Integer::New(req->result);
				break;

			case UV_FS_WRITE:
				argv[1] = Integer::New(req->result);
				break;

			case UV_FS_STAT:
			case UV_FS_LSTAT:
			case UV_FS_FSTAT:
				argv[1] = BuildStatsObject(static_cast<const uv_statbuf_t*>(req->ptr));
				break;

			case UV_FS_READLINK:
				argv[1] = String::New(static_cast<char*>(req->ptr));
				break;

			case UV_FS_READ:
				// Buffer interface
				argv[1] = Integer::New(req->result);
				break;

			case UV_FS_READDIR:
				{
					char *namebuf = static_cast<char*>(req->ptr);
					int nnames = req->result;

					Local<Array> names = Array::New(nnames);

					for (int i = 0; i < nnames; i++) {
						Local<String> name = String::New(namebuf);
						names->Set(Integer::New(i), name);
#ifndef NDEBUG
						namebuf += strlen(namebuf);
						assert(*namebuf == '\0');
						namebuf += 1;
#else
						namebuf += strlen(namebuf) + 1;
#endif
					}

					argv[1] = names;
				}
				break;

			default:
				assert(0 && "Unhandled eio response");
			}
		}

		if (oncomplete_sym.IsEmpty()) {
			oncomplete_sym = UV_PSYMBOL("oncomplete");
		}
		MakeCallback(req_wrap->object_, oncomplete_sym, argc, argv);

		uv_fs_req_cleanup(&req_wrap->req_);
		delete req_wrap;
	}







	//================暴露的c++方法==================
	Handle<Value> Open(const Arguments& args) {
		HandleScope scope;

		if (oncomplete_sym.IsEmpty()) {
			oncomplete_sym = UV_PSYMBOL("oncomplete");
		}

		int len = args.Length();
		if (len < 1) return TYPE_ERROR("path required");
		if (len < 2) return TYPE_ERROR("flags required");
		if (len < 3) return TYPE_ERROR("mode required");
		if (!args[0]->IsString()) return TYPE_ERROR("path must be a string");
		if (!args[1]->IsString()) return TYPE_ERROR("flags must be a string");
		if (!args[2]->IsInt32()) return TYPE_ERROR("mode must be an int");


		String::Utf8Value path(args[0]);
	//	int flags = args[1]->Int32Value();

		v8::String::Utf8Value str(args[1]);
	
		int flags = getFlagByKey(*str);

		int mode = static_cast<int>(args[2]->Int32Value());

		if (args[3]->IsFunction()) {
			ASYNC_CALL(open, args[3], *path, flags, mode)
			//FSReqWrap* req_wrap = new FSReqWrap("open"); 
			//int r = uv_fs_open(uv_default_loop(), &req_wrap->req_, *path, flags, mode, After); 
			//req_wrap->object_->Set(oncomplete_sym, args[3]); 
			//req_wrap->Dispatched(); 
			//if (r < 0) {
			//	uv_fs_t* req = &req_wrap->req_; 
			//	req->result = r; 
			//	req->path = 0; 
			//	req->errorno = uv_last_error(uv_default_loop()).code; 
			//	After(req); 
			//} 
			//uv_run(uv_default_loop(), UV_RUN_DEFAULT); 
			//return scope.Close(req_wrap->object_);

		} else {
			SYNC_CALL(open, *path, *path, flags, mode)
				int fd = SYNC_RESULT;
			printf(" --- %d\n ",fd);
			return scope.Close(Integer::New(fd));
		}
	}

	Handle<Value> Close(const Arguments& args){
		HandleScope scope;
		printf(" --- %s\n ","Close");
		return Undefined();
	}


	int FS::Initialize(Handle<ObjectTemplate> target) {
		HandleScope scope;
		//UV_SET_METHOD(target, "uv_run", UvRun);
		target->Set(String::New("fs_open"), FunctionTemplate::New(Open));
		target->Set(String::New("fs_close"), FunctionTemplate::New(Close));
		return 0;
	}


}





