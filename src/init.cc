#include "init.h"
#include <stdio.h>
#include "util.h"
#include "v8.h"
#include "uv.h"

static bool run_shell;
using namespace v8;
Persistent<Object> process;
Persistent<Object> binding_cache;
Persistent<Array> module_load_list;
Persistent<String> process_symbol;




Handle<Object> SetupProcessObject(int argc, char *argv[]) {
	HandleScope scope;

	Local<FunctionTemplate> process_template = FunctionTemplate::New();
	process_template->SetClassName(String::NewSymbol("process"));
	process = Persistent<Object>::New(process_template->GetFunction()->NewInstance());

	// process.moduleLoadList
	//module_load_list = Persistent<Array>::New(Array::New());
	//process->Set(String::NewSymbol("moduleLoadList"), module_load_list);

	Local<Object> versions = Object::New();
	process->Set(String::NewSymbol("version"), String::New("0.1"));

	return process;
}



static Handle<Value> Log(const Arguments& args) {
	if (args.Length() < 1) return v8::Undefined();
	HandleScope scope;
	Handle<Value> arg = args[0];
	String::Utf8Value value(arg);
	printf("Logged: %s\n", value);
	return v8::Undefined();
}



void Load(Handle<Object> process_l) {
	//process_symbol = UV_PSYMBOL("process");


	//todo 模仿nodejs使用js2c工程，将js转换成c的字符
	Local<Function> f =Local<Function>::Cast(
		Script::Compile(String::New(
		"(function(process) {\n\
		this.global = this;\n\
		global.process = process;\n\
		global.global = global;\n\
		})"
		), String::New("binding:script"))->Run());
	Local<Object> global = v8::Context::GetCurrent()->Global();
	Local<Value> args[1] = { Local<Value>::New(process_l) };
	f->Call(global, 1, args);
}





const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
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
						   return false;
					   } else {
						   v8::Handle<v8::Value> result = script->Run();
						   if (result.IsEmpty()) {
							   return false;
						   } else {

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

void RunShell(v8::Handle<v8::Context> context) {
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



int Start(int argc, char *argv[]) {
	run_shell = (argc == 1);
	V8::Initialize();
	{
		Locker locker;
		HandleScope handle_scope;

		Handle<ObjectTemplate> global = ObjectTemplate::New();

		global->Set(String::New("log"), FunctionTemplate::New(Log));

		int r=UV::RUN::Initialize(global);




		Persistent<Context> context = Context::New(NULL, global);
		Context::Scope context_scope(context);


		Handle<Object> process_l = SetupProcessObject(argc, argv);

		Load(process_l);

		RunMain(argc, argv);
		if (run_shell) RunShell(context);
		//uv_run(uv_default_loop(), UV_RUN_DEFAULT);

		//context->Exit();
		context.Dispose();
	}
	V8::Dispose();
	return 0;
}

Handle<Value> 
	MakeCallback(const Handle<Object> object,
	const Handle<String> symbol,
	int argc,
	Handle<Value> argv[]) {
		HandleScope scope;
		Local<Function> callback = object->Get(symbol).As<Function>();
		TryCatch try_catch;
		Local<Value> ret = callback->Call(object, argc, argv);

		if (try_catch.HasCaught()) {
			ReportException(&try_catch);
			return Undefined();
		}

		return scope.Close(ret);
}


Handle<Value>
	MakeCallback(const Handle<Object> object,
	const char* method,
	int argc,
	Handle<Value> argv[]) {
		HandleScope scope;

		Handle<Value> ret =
			MakeCallback(object, String::NewSymbol(method), argc, argv);

		return scope.Close(ret);
}






