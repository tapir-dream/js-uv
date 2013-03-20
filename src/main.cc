#include <v8.h>
#include <assert.h>

#include <string.h>
#include <stdio.h>

#include "uv.h"
static uv_idle_t idle_handle;

using namespace v8;

v8::Persistent<v8::Context> CreateShellContext();
void RunShell(v8::Handle<v8::Context> context);
int RunMain(int argc, char* argv[]);
bool ExecuteString(v8::Handle<v8::String> source,
				   v8::Handle<v8::Value> name,
				   bool print_result,
				   bool report_exceptions);
v8::Handle<v8::Value> Print(const v8::Arguments& args);
v8::Handle<v8::Value> Read(const v8::Arguments& args);
v8::Handle<v8::Value> Load(const v8::Arguments& args);
v8::Handle<v8::Value> Quit(const v8::Arguments& args);
v8::Handle<v8::Value> Version(const v8::Arguments& args);
v8::Handle<v8::String> ReadFile(const char* name);

v8::Handle<v8::Value> UvRun(const v8::Arguments& args);

void ReportException(v8::TryCatch* handler);


static bool run_shell;


int main(int argc, char* argv[]) {
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

	run_shell = (argc == 1);
	int result;
	{
		v8::HandleScope handle_scope;
		v8::Persistent<v8::Context> context = CreateShellContext();
		if (context.IsEmpty()) {
			fprintf(stderr, "Error creating context\n");
			return 1;
		}
		context->Enter();
		result = RunMain(argc, argv);
		if (run_shell) RunShell(context);
		context->Exit();
		context.Dispose();
	}
	v8::V8::Dispose();
	return result;
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}


// Creates a new execution environment containing the built-in
// functions.
v8::Persistent<v8::Context> CreateShellContext() {
	// Create a template for the global object.
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	// Bind the global 'print' function to the C++ Print callback.
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
	// Bind the global 'read' function to the C++ Read callback.
	global->Set(v8::String::New("read"), v8::FunctionTemplate::New(Read));
	// Bind the global 'load' function to the C++ Load callback.
	global->Set(v8::String::New("load"), v8::FunctionTemplate::New(Load));
	// Bind the 'quit' function
	global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(Quit));
	// Bind the 'version' function
	global->Set(v8::String::New("version"), v8::FunctionTemplate::New(Version));


	global->Set(v8::String::New("log"), v8::FunctionTemplate::New(Print));
	global->Set(v8::String::New("uv_run"), v8::FunctionTemplate::New(UvRun));

	return v8::Context::New(NULL, global);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args) {
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		if (first) {
			first = false;
		} else {
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);
	}
	printf("\n");
	fflush(stdout);
	return v8::Undefined();
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
v8::Handle<v8::Value> Read(const v8::Arguments& args) {
	if (args.Length() != 1) {
		return v8::ThrowException(v8::String::New("Bad parameters"));
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL) {
		return v8::ThrowException(v8::String::New("Error loading file"));
	}
	v8::Handle<v8::String> source = ReadFile(*file);
	if (source.IsEmpty()) {
		return v8::ThrowException(v8::String::New("Error loading file"));
	}
	return source;
}


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
v8::Handle<v8::Value> Load(const v8::Arguments& args) {
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		v8::String::Utf8Value file(args[i]);
		if (*file == NULL) {
			return v8::ThrowException(v8::String::New("Error loading file"));
		}
		v8::Handle<v8::String> source = ReadFile(*file);
		if (source.IsEmpty()) {
			return v8::ThrowException(v8::String::New("Error loading file"));
		}
		if (!ExecuteString(source, v8::String::New(*file), false, false)) {
			return v8::ThrowException(v8::String::New("Error executing file"));
		}
	}
	return v8::Undefined();
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
v8::Handle<v8::Value> Quit(const v8::Arguments& args) {
	// If not arguments are given args[0] will yield undefined which
	// converts to the integer value 0.
	int exit_code = args[0]->Int32Value();
	fflush(stdout);
	fflush(stderr);
	exit(exit_code);
	return v8::Undefined();
}


v8::Handle<v8::Value> Version(const v8::Arguments& args) {
	return v8::String::New(v8::V8::GetVersion());
}


// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL) return v8::Handle<v8::String>();

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
		i += read;
	}
	fclose(file);
	v8::Handle<v8::String> result = v8::String::New(chars, size);
	delete[] chars;
	return result;
}


// Process remaining command line arguments and execute files
int RunMain(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		const char* str = argv[i];
		if (strcmp(str, "--shell") == 0) {
			run_shell = true;
		} else if (strcmp(str, "-f") == 0) {
			// Ignore any -f flags for compatibility with the other stand-
			// alone JavaScript engines.
			continue;
		} else if (strncmp(str, "--", 2) == 0) {
			fprintf(stderr,
				"Warning: unknown flag %s.\nTry --help for options\n", str);
		} else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
			// Execute argument given to -e option directly.
			v8::Handle<v8::String> file_name = v8::String::New("unnamed");
			v8::Handle<v8::String> source = v8::String::New(argv[++i]);
			if (!ExecuteString(source, file_name, false, true)) return 1;
		} else {
			// Use all other arguments as names of files to load and run.
			v8::Handle<v8::String> file_name = v8::String::New(str);
			v8::Handle<v8::String> source = ReadFile(str);
			if (source.IsEmpty()) {
				fprintf(stderr, "Error reading '%s'\n", str);
				continue;
			}
			if (!ExecuteString(source, file_name, false, true)) return 1;
		}
	}
	return 0;
}



#define ASSERT(expr)                                      \
	do {                                                     \
	if (!(expr)) {                                          \
	fprintf(stderr,                                       \
	"Assertion failed in %s on line %d: %s\n",    \
	__FILE__,                                     \
	__LINE__,                                     \
#expr);                                       \
	abort();                                              \
	}                                                       \
	} while (0)

#define NUM_TICKS 64

static int idle_counter;

static void idle_cb2(uv_idle_t* handle, int status) {
	ASSERT(handle == &idle_handle);
	ASSERT(status == 0);
	
	if (++idle_counter == NUM_TICKS){

		printf("%d \n",idle_counter);
		uv_idle_stop(handle);
	}else{
		printf("%d ",idle_counter);
	}
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Handle<v8::Context> context) {
	fprintf(stderr, "js2uv main test\nV8 version %s [sample shell]\n", v8::V8::GetVersion());

	uv_idle_init(uv_default_loop(), &idle_handle);
	uv_idle_start(&idle_handle, idle_cb2);

	while (uv_run(uv_default_loop(), UV_RUN_ONCE));
	ASSERT(idle_counter == NUM_TICKS);


	static const int kBufferSize = 256;
	// Enter the execution environment before evaluating any code.
	v8::Context::Scope context_scope(context);
	v8::Local<v8::String> name(v8::String::New("(shell)"));
	while (true) {
		char buffer[kBufferSize];
		fprintf(stderr, "> ");
		char* str = fgets(buffer, kBufferSize, stdin);
		if (str == NULL) break;
		v8::HandleScope handle_scope;
		ExecuteString(v8::String::New(str), name, true, true);
	}
	fprintf(stderr, "\n");
}


// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source,
				   v8::Handle<v8::Value> name,
				   bool print_result,
				   bool report_exceptions) {
					   v8::HandleScope handle_scope;
					   v8::TryCatch try_catch;
					   v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
					   if (script.IsEmpty()) {
						   // Print errors that happened during compilation.
						   if (report_exceptions)
							   ReportException(&try_catch);
						   return false;
					   } else {
						   v8::Handle<v8::Value> result = script->Run();
						   if (result.IsEmpty()) {
							   assert(try_catch.HasCaught());
							   // Print errors that happened during execution.
							   if (report_exceptions)
								   ReportException(&try_catch);
							   return false;
						   } else {
							   assert(!try_catch.HasCaught());
							   if (print_result && !result->IsUndefined()) {
								   // If all went well and the result wasn't undefined then print
								   // the returned value.
								   v8::String::Utf8Value str(result);
								   const char* cstr = ToCString(str);
								   printf("%s\n", cstr);
							   }
							   return true;
						   }
					   }
}


void ReportException(v8::TryCatch* try_catch) {
	v8::HandleScope handle_scope;
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Handle<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		fprintf(stderr, "%s\n", exception_string);
	} else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		fprintf(stderr, "%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			fprintf(stderr, " ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			fprintf(stderr, "^");
		}
		fprintf(stderr, "\n");
		v8::String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = ToCString(stack_trace);
			fprintf(stderr, "%s\n", stack_trace_string);
		}
	}
}






v8::Handle<v8::Value> Log(const v8::Arguments& args) {
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		if (first) {
			first = false;
		} else {
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);
	}
	printf("\n");
	fflush(stdout);
	return v8::Undefined();
}


static void idle_cb(uv_idle_t* handle, int status) {

	Handle<String> source = String::New("log('test')");  
	Handle<Script> script = Script::Compile(source);  
	Handle<Value> result = script->Run();  

	uv_idle_stop(handle);
}

v8::Handle<v8::Value> UvRun(const v8::Arguments& args){
	uv_loop_t *loop=uv_default_loop();
	uv_run_mode mode=UV_RUN_ONCE;

	uv_idle_init(uv_default_loop(), &idle_handle);
	uv_idle_start(&idle_handle, idle_cb);



	uv_run(loop, mode);


	return v8::Undefined();
}


v8::Handle<v8::Value> FsOpen(const v8::Arguments& args){
	uv_loop_t *loop=uv_default_loop();
	uv_run_mode mode=UV_RUN_ONCE;

	uv_idle_init(uv_default_loop(), &idle_handle);
	uv_idle_start(&idle_handle, idle_cb);



	uv_run(loop, mode);


	return v8::Undefined();
}