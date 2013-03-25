#include "util.h"
#include "uvrun.h"
#include "fs.h"

v8::Handle<v8::Object> SetupProcessObject(int argc, char *argv[]);

void Load(v8::Handle<v8::Object> process);
char** Init(int argc, char *argv[]);

UV_EXTERN int Start(int argc, char *argv[]);

void ReportException(v8::TryCatch* handler);

UV_EXTERN v8::Handle<v8::Value>
	MakeCallback(const v8::Handle<v8::Object> object,
	const char* method,
	int argc,
	v8::Handle<v8::Value> argv[]);

UV_EXTERN v8::Handle<v8::Value>
	MakeCallback(const v8::Handle<v8::Object> object,
	const v8::Handle<v8::String> symbol,
	int argc,
	v8::Handle<v8::Value> argv[]);

UV_EXTERN v8::Local<v8::Value> UVException(int errorno,
										   const char *syscall = NULL,
										   const char *msg     = NULL,
										   const char *path    = NULL);
