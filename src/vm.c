#define _POSIX_C_SOURCE 199309L

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm; // [one]
VM vm;
static void runtimeError(const char* format, ...);
static Value clockNative(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value mapNewNative(int argCount, Value* args) {
  return OBJ_VAL(newMap());
}
static Value mapSetNative(int argCount, Value* args) {
  if (argCount != 3) {
    runtimeError("map_set expects 3 arguments (map, key, value)");
    return NIL_VAL;
  }
  if (!IS_MAP(args[0]) || !IS_STRING(args[1])) {
    runtimeError("map_set: invalid argument types");
    return NIL_VAL;
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = AS_STRING(args[1]);
  tableSet(&map->entries, key, args[2]);
  return NIL_VAL;
}
static Value mapGetNative(int argCount, Value* args) {
  if (argCount != 2) {
    runtimeError("map_get expects 2 arguments (map, key)");
    return NIL_VAL;
  }
  if (!IS_MAP(args[0]) || !IS_STRING(args[1])) {
    runtimeError("map_get: invalid argument types");
    return NIL_VAL;
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = AS_STRING(args[1]);
  Value out;
  if (tableGet(&map->entries, key, &out)) return out;
  return NIL_VAL;
}

static Value mapDeleteNative(int argCount, Value* args) {
  if (argCount != 2) {
    runtimeError("map_delete expects 2 arguments (map, key)");
    return BOOL_VAL(false);
  }
  if (!IS_MAP(args[0]) || !IS_STRING(args[1])) {
    runtimeError("map_delete: invalid argument types");
    return BOOL_VAL(false);
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = AS_STRING(args[1]);
  return BOOL_VAL(tableDelete(&map->entries, key));
}

static Value mapHasNative(int argCount, Value* args) {
  if (argCount != 2) {
    runtimeError("map_has expects 2 arguments (map, key)");
    return BOOL_VAL(false);
  }
  if (!IS_MAP(args[0]) || !IS_STRING(args[1])) {
    runtimeError("map_has: invalid argument types");
    return BOOL_VAL(false);
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = AS_STRING(args[1]);
  Value out;
  return BOOL_VAL(tableGet(&map->entries, key, &out));
}

static Value openNative(int argCount, Value* args) {
  if (argCount < 1 || !IS_STRING(args[0])) {
    runtimeError("open expects at least a path string");
    return NIL_VAL;
  }
  const char* mode = "r";
  if (argCount >= 2 && IS_STRING(args[1])) mode = AS_CSTRING(args[1]);
  ObjString* path = AS_STRING(args[0]);
  FILE* f = fopen(path->chars, mode);
  if (f == NULL) return NIL_VAL;
  return OBJ_VAL(newFileObj(f, path));
}

static Value fileReadNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_FILE(args[0])) {
    runtimeError("file_read expects a file object");
    return NIL_VAL;
  }
  ObjFile* fo = AS_FILE(args[0]);
  FILE* f = fo->handle;
  if (f == NULL) return NIL_VAL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char* buf = ALLOCATE(char, (int)len + 1);
  size_t read = fread(buf, 1, (size_t)len, f);
  buf[read] = '\0';
  ObjString* s = takeString(buf, (int)read);
  return OBJ_VAL(s);
}

static Value fileWriteNative(int argCount, Value* args) {
  if (argCount != 2 || !IS_FILE(args[0]) || !IS_STRING(args[1])) {
    runtimeError("file_write expects (file, string)");
    return BOOL_VAL(false);
  }
  ObjFile* fo = AS_FILE(args[0]);
  if (fo->handle == NULL) return BOOL_VAL(false);
  ObjString* s = AS_STRING(args[1]);
  size_t written = fwrite(s->chars, 1, s->length, fo->handle);
  fflush(fo->handle);
  return BOOL_VAL(written == (size_t)s->length);
}

static Value fileCloseNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_FILE(args[0])) {
    runtimeError("file_close expects a file object");
    return NIL_VAL;
  }
  ObjFile* fo = AS_FILE(args[0]);
  if (fo->handle != NULL) {
    fclose(fo->handle);
    fo->handle = NULL;
  }
  return NIL_VAL;
}

static Value sleepNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("sleep expects a number (seconds)");
    return NIL_VAL;
  }
  double seconds = AS_NUMBER(args[0]);
  if (seconds <= 0) return NIL_VAL;

#ifdef _WIN32
  Sleep((unsigned long)(seconds * 1000));
#else
 struct timespec ts;
ts.tv_sec = (time_t)seconds;
ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1000000000L);

nanosleep(&ts, NULL);
#endif

  return NIL_VAL;
}

static Value importNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_STRING(args[0])) {
    runtimeError("import expects a path string");
    return NIL_VAL;
  }
  ObjString* path = AS_STRING(args[0]);
  FILE* f = fopen(path->chars, "r");
  if (f == NULL) {
    runtimeError("Could not open file '%s'", path->chars);
    return NIL_VAL;
  }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char* buf = ALLOCATE(char, (int)len + 1);
  size_t read = fread(buf, 1, (size_t)len, f);
  buf[read] = '\0';
  fclose(f);

  InterpretResult res = interpret(buf);

  FREE_ARRAY(char, buf, (int)len + 1);
  (void)res; // ignore for now; errors will be printed by interpret
  return NIL_VAL;
}

static Value listNative(int argCount, Value* args) {
  if (argCount != 0) {
    runtimeError("list() takes no arguments");
    return NIL_VAL;
  }
  return OBJ_VAL(newArray());
}

static Value listPushNative(int argCount, Value* args) {
  if (argCount != 2 || !IS_MAP(args[0]) && !IS_OBJ(args[0])) {
    if (argCount < 1 || !IS_OBJ(args[0])) {
      runtimeError("list_push expects (list, value)");
      return NIL_VAL;
    }
  }
  if (!IS_ARRAY(args[0])) {
    runtimeError("list_push: first argument must be a list");
    return NIL_VAL;
  }
  ObjArray* arr = AS_ARRAY(args[0]);
  writeValueArray(&arr->elements, args[1]);
  return NIL_VAL;
}

static Value listGetNative(int argCount, Value* args) {
  if (argCount != 2 || !IS_ARRAY(args[0]) || !IS_NUMBER(args[1])) {
    runtimeError("list_get expects (list, index)");
    return NIL_VAL;
  }
  ObjArray* arr = AS_ARRAY(args[0]);
  int idx = (int)AS_NUMBER(args[1]);
  if (idx < 0 || idx >= arr->elements.count) {
    runtimeError("list_get index out of bounds");
    return NIL_VAL;
  }
  return arr->elements.values[idx];
}

static Value listSetNative(int argCount, Value* args) {
  if (argCount != 3 || !IS_ARRAY(args[0]) || !IS_NUMBER(args[1])) {
    runtimeError("list_set expects (list, index, value)");
    return NIL_VAL;
  }
  ObjArray* arr = AS_ARRAY(args[0]);
  int idx = (int)AS_NUMBER(args[1]);
  if (idx < 0 || idx >= arr->elements.count) {
    runtimeError("list_set index out of bounds");
    return NIL_VAL;
  }
  arr->elements.values[idx] = args[2];
  return NIL_VAL;
}

static Value listPopNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_ARRAY(args[0])) {
    runtimeError("list_pop expects (list)");
    return NIL_VAL;
  }
  ObjArray* arr = AS_ARRAY(args[0]);
  if (arr->elements.count == 0) return NIL_VAL;
  Value val = arr->elements.values[arr->elements.count - 1];
  arr->elements.count--;
  return val;
}

static Value listLengthNative(int argCount, Value* args) {
  if (argCount != 1 || !IS_ARRAY(args[0])) {
    runtimeError("list_length expects (list)");
    return NIL_VAL;
  }
  ObjArray* arr = AS_ARRAY(args[0]);
  return NUMBER_VAL((double)arr->elements.count);
}
static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}
static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
  size_t instruction = frame->ip - function->chunk.code - 1;
  fprintf(stderr, "[line %d] in ",
      function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}
static void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  resetStack();
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);

  vm.initString = NULL;
  vm.initString = copyString("init", 4);

  defineNative("ঘড়ী", clockNative);
  defineNative("map", mapNewNative);
  defineNative("map_set", mapSetNative);
  defineNative("map_get", mapGetNative);
  defineNative("map_delete", mapDeleteNative);
  defineNative("map_has", mapHasNative);

  defineNative("খোল", openNative);
  defineNative("file_read", fileReadNative);
  defineNative("file_write  ", fileWriteNative);
  defineNative("file_close", fileCloseNative);
  defineNative("import", importNative);
  defineNative("list", listNative);
  defineNative("list_push", listPushNative);
  defineNative("list_get", listGetNative);
  defineNative("list_set", listSetNative);
  defineNative("list_pop", listPopNative);
  defineNative("list_length", listLengthNative);
  defineNative("sleep", sleepNative);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;
  freeObjects();
}
void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}
Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {

  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        closure->function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}
static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        vm.stackTop[-argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
      }
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
        Value initializer;
        if (tableGet(&klass->methods, vm.initString,
                     &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);
        } else if (argCount != 0) {
          runtimeError("Expected 0 arguments but got %d.",
                       argCount);
          return false;
        }
        return true;
      }
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);

      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.stackTop - argCount);
        vm.stackTop -= argCount + 1;
        push(result);
        return true;
      }
      default:
        break; // Non-callable object type.
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}
static bool invokeFromClass(ObjClass* klass, ObjString* name,
                            int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), argCount);
}
static bool invoke(ObjString* name, int argCount) {
  Value receiver = peek(argCount);

  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }

  ObjInstance* instance = AS_INSTANCE(receiver);

  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
}
static bool bindMethod(ObjClass* klass, ObjString* name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod* bound = newBoundMethod(peek(0),
                                         AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}
static ObjUpvalue* captureUpvalue(Value* local) {
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue* createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}
static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}
static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static void concatenate() {

  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}
static InterpretResult run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
//> trace-stack
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
//< Closures disassemble-instruction
#endif

//< trace-execution
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_POP: pop(); break;
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name); // [delete]
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtimeError("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = READ_STRING();
        
        Value value;
        if (tableGet(&instance->fields, name, &value)) {
          pop(); // Instance.
          push(value);
          break;
        }
        if (!bindMethod(instance->klass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtimeError("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(1));
        tableSet(&instance->fields, READ_STRING(), peek(0));
        Value value = pop();
        pop();
        push(value);
        break;
      }
      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());
        
        if (!bindMethod(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;

      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError(
              "Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();

        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();

        if (isFalsey(peek(0))) frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();

        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        int argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        if (!invoke(method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        ObjClass* superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = newClosure(function);
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] =
                captureUpvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
      case OP_RETURN: {
        Value result = pop();
        closeUpvalues(frame->slots);
        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CLASS:
        push(OBJ_VAL(newClass(READ_STRING())));
        break;
      case OP_INHERIT: {
        Value superclass = peek(1);
        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* subclass = AS_CLASS(peek(0));
        tableAddAll(&AS_CLASS(superclass)->methods,
                    &subclass->methods);
        pop(); // Subclass.
        break;
      }
      case OP_METHOD:
        defineMethod(READ_STRING());
        break;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}
void hack(bool b) {
  run();
  if (b) hack(false);
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure* closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();

}
//< interpret
