#ifndef UV_UVRUN_H_
#define UV_UVRUN_H_

#include "util.h"
#include "v8.h"
#include "uv.h"

namespace UV {

	class RUN {
	public:
		static int Initialize (v8::Handle<v8::ObjectTemplate> target);
	};

}  
#endif 











