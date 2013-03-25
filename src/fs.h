#ifndef UV_UVFS_H_
#define UV_UVFS_H_

#include "util.h"
#include "v8.h"
#include "uv.h"
#include <fcntl.h>
namespace UV {

	class FS {
	public:
		static int Initialize (v8::Handle<v8::ObjectTemplate> target);
	};

}  
#endif 











