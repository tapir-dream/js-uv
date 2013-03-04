#include <iostream>
#include <node.h>
#include <v8.h>

std::string TO_String(v8::Handle<v8::String> v8str) {
	v8::String::Utf8Value utf8str(v8str);
	return std::string(*utf8str, utf8str.length());
}

char *TO_CHAR(v8::Handle<v8::Value> val) {
	v8::String::Utf8Value utf8(val->ToString());
	int len = utf8.length() + 1;
	char *str = (char *) calloc(sizeof(char), len);
	strncpy(str, *utf8, len);
	return str;
}


void run_command(uv_fs_event_t *handle, const char *filename, int events, int status) {
	fprintf(stderr, "Change detected in %s: ", handle->filename);
	if (events == UV_RENAME)
		fprintf(stderr, "renamed");
	if (events == UV_CHANGE)
		fprintf(stderr, "changed");

	fprintf(stderr, " %s\n", filename ? filename : "");

}


v8::Handle<v8::Value> Detect(const v8::Arguments& args) {
	v8::HandleScope scope;
	if(args.Length()&&!args[0]->IsString()){
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong arguments")));
		return scope.Close(v8::Undefined());
	}

	uv_loop_t *loop = uv_default_loop();

	v8::Handle<v8::Value> arg = args[0];
	v8::String::Utf8Value value(arg);

	char* filePath=TO_CHAR(arg);
	//printf("filePath --- %s\n ",filePath);
	printf("监听: %s\n ",filePath);
	uv_fs_event_init(loop, (uv_fs_event_t*) malloc(sizeof(uv_fs_event_t)), filePath, run_command, 0);

	//uv_run(loop,UV_RUN_DEFAULT); //v0.9
	uv_run(loop);
	return scope.Close(v8::Undefined());
}

void init(v8::Handle<v8::Object> target) {
	NODE_SET_METHOD(target, "detect", Detect);
}

NODE_MODULE(binding, init);
