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
#include <math.h>
#include "graphics.h"

VM vm;

static ObjString* mapKeyString(Value value) {
  if (IS_STRING(value)) {
    return AS_STRING(value);
  }
  if (IS_NUMBER(value)) {
    char buf[64];
    sprintf(buf, "%.14g", AS_NUMBER(value));
    return copyString(buf, (int)strlen(buf));
  }
  if (IS_BOOL(value)) {
    return AS_BOOL(value) ? copyString("true", 4) : copyString("false", 5);
  }
  if (IS_NIL(value)) {
    return copyString("nil", 3);
  }
  return copyString("<object>", 8);
}

static void runtimeError(const char *format, ...);
static Value clockNative(int argCount, Value *args)
{
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value openNative(int argCount, Value *args)
{
  if (argCount < 1 || !IS_STRING(args[0]))
  {
    runtimeError("open expects at least a path string");
    return NIL_VAL;
  }
  const char *mode = "r";
  if (argCount >= 2 && IS_STRING(args[1]))
    mode = AS_CSTRING(args[1]);
  ObjString *path = AS_STRING(args[0]);
  FILE *f = fopen(path->chars, mode);
  if (f == NULL)
    return NIL_VAL;
  return OBJ_VAL(newFileObj(f, path));
}

static Value fileReadNative(int argCount, Value *args)
{
  if (argCount != 1 || !IS_FILE(args[0]))
  {
    runtimeError("file_read expects a file object");
    return NIL_VAL;
  }
  ObjFile *fo = AS_FILE(args[0]);
  FILE *f = fo->handle;
  if (f == NULL)
    return NIL_VAL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  char *buf = ALLOCATE(char, (int)len + 1);
  size_t read = fread(buf, 1, (size_t)len, f);
  buf[read] = '\0';
  ObjString *s = takeString(buf, (int)read);
  return OBJ_VAL(s);
}

static Value fileWriteNative(int argCount, Value *args)
{
  if (argCount != 2 || !IS_FILE(args[0]) || !IS_STRING(args[1]))
  {
    runtimeError("file_write expects (file, string)");
    return BOOL_VAL(false);
  }
  ObjFile *fo = AS_FILE(args[0]);
  if (fo->handle == NULL)
    return BOOL_VAL(false);
  ObjString *s = AS_STRING(args[1]);
  size_t written = fwrite(s->chars, 1, s->length, fo->handle);
  fflush(fo->handle);
  return BOOL_VAL(written == (size_t)s->length);
}

static Value fileCloseNative(int argCount, Value *args)
{
  if (argCount != 1 || !IS_FILE(args[0]))
  {
    runtimeError("file_close expects a file object");
    return NIL_VAL;
  }
  ObjFile *fo = AS_FILE(args[0]);
  if (fo->handle != NULL)
  {
    fclose(fo->handle);
    fo->handle = NULL;
  }
  return NIL_VAL;
}

static Value sleepNative(int argCount, Value *args)
{
  if (argCount != 1 || !IS_NUMBER(args[0]))
  {
    runtimeError("sleep expects a number (seconds)");
    return NIL_VAL;
  }
  double seconds = AS_NUMBER(args[0]);
  if (seconds <= 0)
    return NIL_VAL;

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

static Value sinNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("sin expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

static Value cosNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("cos expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

static Value tanNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("tan expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

static Value sqrtNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("sqrt expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
}

static Value atan2Native(int argCount, Value *args) {
  if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
    runtimeError("atan2 expects 2 number arguments (y, x).");
    return NIL_VAL;
  }
  return NUMBER_VAL(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

static Value absNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("abs expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

static Value floorNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("floor expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

static Value ceilNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("ceil expects 1 number argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

static Value toStringNative(int argCount, Value *args) {
  if (argCount != 1) {
    runtimeError("to_string expects 1 argument.");
    return NIL_VAL;
  }
  ObjString* s = mapKeyString(args[0]);
  return OBJ_VAL(s);
}

static Value toNumberNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_STRING(args[0])) {
    runtimeError("to_number expects 1 string argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(strtod(AS_CSTRING(args[0]), NULL));
}

static Value charAtNative(int argCount, Value *args) {
  if (argCount != 2 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
    runtimeError("char_at expects (string, index).");
    return NIL_VAL;
  }
  ObjString* s = AS_STRING(args[0]);
  int index = (int)AS_NUMBER(args[1]);
  if (index < 0 || index >= s->length) {
    runtimeError("char_at index out of bounds.");
    return NIL_VAL;
  }
  return OBJ_VAL(copyString(&s->chars[index], 1));
}

static Value substringNative(int argCount, Value *args) {
  if (argCount != 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
    runtimeError("substring expects (string, start, length).");
    return NIL_VAL;
  }
  ObjString* s = AS_STRING(args[0]);
  int start = (int)AS_NUMBER(args[1]);
  int len = (int)AS_NUMBER(args[2]);
  if (start < 0 || start > s->length || len < 0 || start + len > s->length) {
    runtimeError("substring bounds out of range.");
    return NIL_VAL;
  }
  return OBJ_VAL(copyString(&s->chars[start], len));
}

static Value strLenNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_STRING(args[0])) {
    runtimeError("str_len expects 1 string argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(AS_STRING(args[0])->length);
}

static Value byteArrayNewNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("byte_array_new expects 1 number argument (size).");
    return NIL_VAL;
  }
  int size = (int)AS_NUMBER(args[0]);
  if (size <= 0) {
    runtimeError("byte_array_new size must be positive.");
    return NIL_VAL;
  }
  return OBJ_VAL(newByteArray(size));
}

static Value byteArraySizeNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_BYTE_ARRAY(args[0])) {
    runtimeError("byte_array_size expects 1 byte array argument.");
    return NIL_VAL;
  }
  return NUMBER_VAL(AS_BYTE_ARRAY(args[0])->count);
}

static Value byteArrayToStringNative(int argCount, Value *args) {
  if (argCount != 3 || !IS_BYTE_ARRAY(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
    runtimeError("byte_array_to_string expects (array, start, length).");
    return NIL_VAL;
  }
  ObjByteArray* array = AS_BYTE_ARRAY(args[0]);
  int start = (int)AS_NUMBER(args[1]);
  int len = (int)AS_NUMBER(args[2]);
  if (start < 0 || start > array->count || len < 0 || start + len > array->count) {
    runtimeError("byte_array_to_string index out of bounds.");
    return NIL_VAL;
  }
  int actual_len = 0;
  while (actual_len < len && array->bytes[start + actual_len] != '\0') {
    actual_len++;
  }
  return OBJ_VAL(copyString((const char*)&array->bytes[start], actual_len));
}

static Value fileReadBytesNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_FILE(args[0])) {
    runtimeError("file_read_bytes expects a file object");
    return NIL_VAL;
  }
  ObjFile *fo = AS_FILE(args[0]);
  FILE *f = fo->handle;
  if (f == NULL)
    return NIL_VAL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  if (len < 0) len = 0;
  ObjByteArray *array = newByteArray((int)len);
  push(OBJ_VAL(array));
  size_t read = fread(array->bytes, 1, (size_t)len, f);
  if (read < (size_t)len) {
    array->count = (int)read;
  }
  pop();
  return OBJ_VAL(array);
}

static Value mapNewNative(int argCount, Value *args) {
  return OBJ_VAL(newMap());
}

static Value mapKeysNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_MAP(args[0])) {
    runtimeError("map_keys expects 1 map argument.");
    return NIL_VAL;
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjList* list = newList();
  push(OBJ_VAL(list)); // Avoid GC
  for (int i = 0; i < map->table.capacity; i++) {
    Entry* entry = &map->table.entries[i];
    if (entry->key != NULL) {
      writeList(list, OBJ_VAL(entry->key));
    }
  }
  pop();
  return OBJ_VAL(list);
}

static Value mapHasNative(int argCount, Value *args) {
  if (argCount != 2 || !IS_MAP(args[0])) {
    runtimeError("map_has expects (map, key).");
    return BOOL_VAL(false);
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = mapKeyString(args[1]);
  Value val;
  return BOOL_VAL(tableGet(&map->table, key, &val));
}

static Value mapDeleteNative(int argCount, Value *args) {
  if (argCount != 2 || !IS_MAP(args[0])) {
    runtimeError("map_delete expects (map, key).");
    return BOOL_VAL(false);
  }
  ObjMap* map = AS_MAP(args[0]);
  ObjString* key = mapKeyString(args[1]);
  return BOOL_VAL(tableDelete(&map->table, key));
}
static Value windowInitNative(int argCount, Value *args) {
  if (argCount != 3 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1]) || !IS_STRING(args[2])) {
    runtimeError("window_init expects (width, height, title).");
    return BOOL_VAL(false);
  }
  int w = (int)AS_NUMBER(args[0]);
  int h = (int)AS_NUMBER(args[1]);
  const char* title = AS_CSTRING(args[2]);
  return BOOL_VAL(initWindow(w, h, title));
}

static Value windowUpdateNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_BYTE_ARRAY(args[0])) {
    runtimeError("window_update expects 1 byte array argument (framebuffer).");
    return BOOL_VAL(false);
  }
  ObjByteArray* array = AS_BYTE_ARRAY(args[0]);
  return BOOL_VAL(updateWindow(array->bytes, array->count));
}

static Value windowCloseNative(int argCount, Value *args) {
  closeWindow();
  return NIL_VAL;
}

static Value keyPressedNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0])) {
    runtimeError("key_pressed expects 1 number argument (vk_code).");
    return BOOL_VAL(false);
  }
  int code = (int)AS_NUMBER(args[0]);
  return BOOL_VAL(isKeyPressed(code));
}

static Value mouseXNative(int argCount, Value *args) {
  int x, y; bool l, r;
  getMouseState(&x, &y, &l, &r);
  return NUMBER_VAL(x);
}

static Value mouseYNative(int argCount, Value *args) {
  int x, y; bool l, r;
  getMouseState(&x, &y, &l, &r);
  return NUMBER_VAL(y);
}

static Value mouseLeftNative(int argCount, Value *args) {
  int x, y; bool l, r;
  getMouseState(&x, &y, &l, &r);
  return BOOL_VAL(l);
}

static Value mouseRightNative(int argCount, Value *args) {
  int x, y; bool l, r;
  getMouseState(&x, &y, &l, &r);
  return BOOL_VAL(r);
}

static void resetStack()
{
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}
static void runtimeError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--)
  {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ",
            function->chunk.lines[instruction]);
    if (function->name == NULL)
    {
      fprintf(stderr, "script\n");
    }
    else
    {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}
static void defineNative(const char *name, NativeFn function)
{
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM()
{
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

  defineNative("খোল", openNative);
  defineNative("file_read", fileReadNative);
  defineNative("file_read_bytes", fileReadBytesNative);
  defineNative("file_write", fileWriteNative);
  defineNative("file_close", fileCloseNative);
  defineNative("sleep", sleepNative);

  defineNative("sin", sinNative);
  defineNative("cos", cosNative);
  defineNative("tan", tanNative);
  defineNative("sqrt", sqrtNative);
  defineNative("atan2", atan2Native);
  defineNative("abs", absNative);
  defineNative("floor", floorNative);
  defineNative("ceil", ceilNative);

  defineNative("to_string", toStringNative);
  defineNative("to_number", toNumberNative);
  defineNative("char_at", charAtNative);
  defineNative("substring", substringNative);
  defineNative("str_len", strLenNative);

  defineNative("byte_array_new", byteArrayNewNative);
  defineNative("byte_array_size", byteArraySizeNative);
  defineNative("byte_array_to_string", byteArrayToStringNative);

  defineNative("map_new", mapNewNative);
  defineNative("map_keys", mapKeysNative);
  defineNative("map_has", mapHasNative);
  defineNative("map_delete", mapDeleteNative);

  defineNative("window_init", windowInitNative);
  defineNative("window_update", windowUpdateNative);
  defineNative("window_close", windowCloseNative);
  defineNative("key_pressed", keyPressedNative);
  defineNative("mouse_x", mouseXNative);
  defineNative("mouse_y", mouseYNative);
  defineNative("mouse_left", mouseLeftNative);
  defineNative("mouse_right", mouseRightNative);
}

void freeVM()
{
  closeWindow();
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;
  freeObjects();
}
void push(Value value)
{
  *vm.stackTop = value;
  vm.stackTop++;
}
Value pop()
{
  vm.stackTop--;
  return *vm.stackTop;
}
static Value peek(int distance)
{
  return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure *closure, int argCount)
{

  if (argCount != closure->function->arity)
  {
    runtimeError("Expected %d arguments but got %d.",
                 closure->function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX)
  {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frameCount++];

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}
static bool callValue(Value callee, int argCount)
{
  if (IS_OBJ(callee))
  {
    switch (OBJ_TYPE(callee))
    {
    case OBJ_BOUND_METHOD:
    {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
      vm.stackTop[-argCount - 1] = bound->receiver;
      return call(bound->method, argCount);
    }
    case OBJ_CLASS:
    {
      ObjClass *klass = AS_CLASS(callee);
      vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
      Value initializer;
      if (tableGet(&klass->methods, vm.initString,
                   &initializer))
      {
        return call(AS_CLOSURE(initializer), argCount);
      }
      else if (argCount != 0)
      {
        runtimeError("Expected 0 arguments but got %d.",
                     argCount);
        return false;
      }
      return true;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);

    case OBJ_NATIVE:
    {
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
static bool invokeFromClass(ObjClass *klass, ObjString *name,
                            int argCount)
{
  Value method;
  if (!tableGet(&klass->methods, name, &method))
  {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), argCount);
}
static bool invoke(ObjString *name, int argCount)
{
  Value receiver = peek(argCount);

  if (!IS_INSTANCE(receiver))
  {
    runtimeError("Only instances have methods.");
    return false;
  }

  ObjInstance *instance = AS_INSTANCE(receiver);

  Value value;
  if (tableGet(&instance->fields, name, &value))
  {
    vm.stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
}
static bool bindMethod(ObjClass *klass, ObjString *name)
{
  Value method;
  if (!tableGet(&klass->methods, name, &method))
  {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod *bound = newBoundMethod(peek(0),
                                         AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

static ObjUpvalue *captureUpvalue(Value *local)
{
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local)
  {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local)
  {
    return upvalue;
  }

  ObjUpvalue *createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL)
  {
    vm.openUpvalues = createdUpvalue;
  }
  else
  {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}
static void closeUpvalues(Value *last)
{
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last)
  {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}
static void defineMethod(ObjString *name)
{
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static bool invokeListMethod(ObjList *list, ObjString *method, int argCount)
{
  // get(index)
  if (strcmp(method->chars, "get") == 0)
  {
    if (argCount != 1 || !IS_NUMBER(peek(0)))
    {
      runtimeError("get() expects 1 number argument.");
      return false;
    }
    int index = (int)AS_NUMBER(peek(0));
    if (index < 0 || index >= list->count)
    {
      runtimeError("List index out of range.");
      return false;
    }
    Value value = list->items[index];
    pop(); // index
    pop(); // receiver
    push(value);
    return true;
  }

  // set(index, value)
  if (strcmp(method->chars, "set") == 0)
  {
    if (argCount != 2 || !IS_NUMBER(peek(1)))
    {
      runtimeError("set() expects (index, value).");
      return false;
    }
    int index = (int)AS_NUMBER(peek(1));
    if (index < 0 || index >= list->count)
    {
      runtimeError("List index out of range.");
      return false;
    }
    Value value = peek(0);
    list->items[index] = value;
    pop();
    pop(); // value, index
    pop(); // receiver
    push(NIL_VAL);
    return true;
  }

  // push(value)
  if (strcmp(method->chars, "push") == 0)
  {
    if (argCount != 1)
    {
      runtimeError("push() expects 1 argument.");
      return false;
    }
    writeList(list, peek(0));
    pop(); // argument
    pop(); // receiver list
    push(NIL_VAL);
    return true;
  }

  // pop()
  if (strcmp(method->chars, "pop") == 0)
  {
    if (argCount != 0)
    {
      runtimeError("pop() expects no arguments.");
      return false;
    }

    if (list->count == 0)
    {
      runtimeError("Cannot pop from empty list.");
      return false;
    }
    Value value = list->items[list->count - 1];
    list->count--;
    pop(); // receiver
    push(value);
    return true;
  }

  // size()
  if (strcmp(method->chars, "size") == 0)
  {
    if (argCount != 0)
    {
      runtimeError("size() expects no arguments.");
      return false;
    }
    pop(); // receiver
    push(NUMBER_VAL(list->count));
    return true;
  }

  // reverse()
  if (strcmp(method->chars, "reverse") == 0)
  {
    if (argCount != 0)
    {
      runtimeError("reverse() expects no arguments.");
      return false;
    }

    for (int i = 0, j = list->count - 1;
         i < j;
         i++, j--)
    {
      Value tmp = list->items[i];
      list->items[i] = list->items[j];
      list->items[j] = tmp;
    }

    pop(); // receiver
    push(NIL_VAL);
    return true;
  }
  runtimeError("Undefined list method '%s'.", method->chars);
  return false;
}

static bool isFalsey(Value value)
{
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static void concatenate()
{

  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}
static InterpretResult run()
{
  CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
  (frame->ip += 2,   \
   (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                    \
  do                                                \
  {                                                 \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
    {                                               \
      runtimeError("Operands must be numbers.");    \
      return INTERPRET_RUNTIME_ERROR;               \
    }                                               \
    double b = AS_NUMBER(pop());                    \
    double a = AS_NUMBER(pop());                    \
    push(valueType(a op b));                        \
  } while (false)

  for (;;)
  {
#ifdef DEBUG_TRACE_EXECUTION
    //> trace-stack
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
    {
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
    switch (instruction = READ_BYTE())
    {
    case OP_CONSTANT:
    {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_CONSTANT_LONG:
    {
      uint8_t b1 = READ_BYTE();
      uint8_t b2 = READ_BYTE();
      uint8_t b3 = READ_BYTE();
      int index = b1 | (b2 << 8) | (b3 << 16);
      Value constant = frame->closure->function->chunk.constants.values[index];
      push(constant);
      break;
    }
    case OP_NIL:
      push(NIL_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_POP:
      pop();
      break;
    case OP_GET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_GET_GLOBAL:
    {
      ObjString *name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value))
      {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_DEFINE_GLOBAL:
    {
      ObjString *name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_SET_GLOBAL:
    {
      ObjString *name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0)))
      {
        tableDelete(&vm.globals, name); // [delete]
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_UPVALUE:
    {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE:
    {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }
    case OP_GET_PROPERTY:
    {
      if (!IS_INSTANCE(peek(0)))
      {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();

      Value value;
      if (tableGet(&instance->fields, name, &value))
      {
        pop(); // Instance.
        push(value);
        break;
      }
      if (!bindMethod(instance->klass, name))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SET_PROPERTY:
    {
      if (!IS_INSTANCE(peek(1)))
      {
        runtimeError("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(1));
      tableSet(&instance->fields, READ_STRING(), peek(0));
      Value value = pop();
      pop();
      push(value);
      break;
    }
    case OP_GET_SUPER:
    {
      ObjString *name = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());

      if (!bindMethod(superclass, name))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_EQUAL:
    {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }
    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
      BINARY_OP(BOOL_VAL, <);
      break;

    case OP_ADD:
    {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
      {
        concatenate();
      }
      else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else
      {
        runtimeError(
            "Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;
    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;
    case OP_NEGATE:
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    case OP_MODULO:
    {
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))
      {
        runtimeError("Operands must be numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());
      push(NUMBER_VAL(fmod(a, b)));
      break;
    }
#define BITWISE_BINARY_OP(op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      int32_t b = (int32_t)AS_NUMBER(pop()); \
      int32_t a = (int32_t)AS_NUMBER(pop()); \
      push(NUMBER_VAL((double)(a op b))); \
    } while (false)
    case OP_BITWISE_AND:
      BITWISE_BINARY_OP(&);
      break;
    case OP_BITWISE_OR:
      BITWISE_BINARY_OP(|);
      break;
    case OP_BITWISE_XOR:
      BITWISE_BINARY_OP(^);
      break;
    case OP_LEFT_SHIFT:
      BITWISE_BINARY_OP(<<);
      break;
    case OP_RIGHT_SHIFT:
      BITWISE_BINARY_OP(>>);
      break;
#undef BITWISE_BINARY_OP
    case OP_BITWISE_NOT:
    {
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int32_t a = (int32_t)AS_NUMBER(pop());
      push(NUMBER_VAL((double)(~a)));
      break;
    }
    case OP_PRINT:
    {
      printValue(pop());
      printf("\n");
      break;
    }
    case OP_JUMP:
    {
      uint16_t offset = READ_SHORT();

      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE:
    {
      uint16_t offset = READ_SHORT();

      if (isFalsey(peek(0)))
        frame->ip += offset;
      break;
    }
    case OP_LOOP:
    {
      uint16_t offset = READ_SHORT();

      frame->ip -= offset;
      break;
    }
    case OP_CALL:
    {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_INVOKE:
    {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();

      Value receiver = peek(argCount);

      if (IS_LIST(receiver))
      {
        if (!invokeListMethod(
                AS_LIST(receiver),
                method,
                argCount))
        {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }

      if (!invoke(method, argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_SUPER_INVOKE:
    {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      ObjClass *superclass = AS_CLASS(pop());
      if (!invokeFromClass(superclass, method, argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_CLOSURE:
    {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++)
      {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal)
        {
          closure->upvalues[i] =
              captureUpvalue(frame->slots + index);
        }
        else
        {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE:
      closeUpvalues(vm.stackTop - 1);
      pop();
      break;
    case OP_RETURN:
    {
      Value result = pop();
      closeUpvalues(frame->slots);
      vm.frameCount--;
      if (vm.frameCount == 0)
      {
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
    case OP_INHERIT:
    {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass))
      {
        runtimeError("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjClass *subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods,
                  &subclass->methods);
      pop(); // Subclass.
      break;
    }
    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    case OP_LIST:
    {
      int count = READ_BYTE();
      ObjList *list = newList();
      list->capacity = count;
      list->items = ALLOCATE(Value, count);
      for (int i = count - 1; i >= 0; i--)
      {
        list->items[i] = pop();
      }
      list->count = count;
      push(OBJ_VAL(list));
      break;
    }
    case OP_GET_INDEX:
    {
      Value indexVal = peek(0);
      Value receiver = peek(1);
      if (IS_LIST(receiver)) {
        if (!IS_NUMBER(indexVal)) {
          runtimeError("List index must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        int index = (int)AS_NUMBER(pop());
        ObjList *list = AS_LIST(pop());
        if (index < 0 || index >= list->count) {
          runtimeError("List index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(list->items[index]);
      } else if (IS_BYTE_ARRAY(receiver)) {
        if (!IS_NUMBER(indexVal)) {
          runtimeError("Byte array index must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        int index = (int)AS_NUMBER(pop());
        ObjByteArray *array = AS_BYTE_ARRAY(pop());
        if (index < 0 || index >= array->count) {
          runtimeError("Byte array index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(array->bytes[index]));
      } else if (IS_MAP(receiver)) {
        ObjString *key = mapKeyString(pop());
        ObjMap *map = AS_MAP(pop());
        Value result;
        if (tableGet(&map->table, key, &result)) {
          push(result);
        } else {
          push(NIL_VAL);
        }
      } else {
        runtimeError("Can only index lists, byte arrays, and maps.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SET_INDEX:
    {
      Value value = peek(0);
      Value indexVal = peek(1);
      Value receiver = peek(2);
      if (IS_LIST(receiver)) {
        if (!IS_NUMBER(indexVal)) {
          runtimeError("List index must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        int index = (int)AS_NUMBER(indexVal);
        ObjList *list = AS_LIST(receiver);
        if (index < 0 || index >= list->count) {
          runtimeError("List index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        list->items[index] = value;
        pop(); pop(); pop();
        push(value);
      } else if (IS_BYTE_ARRAY(receiver)) {
        if (!IS_NUMBER(indexVal)) {
          runtimeError("Byte array index must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        if (!IS_NUMBER(value)) {
          runtimeError("Byte array value must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        int index = (int)AS_NUMBER(indexVal);
        ObjByteArray *array = AS_BYTE_ARRAY(receiver);
        if (index < 0 || index >= array->count) {
          runtimeError("Byte array index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        array->bytes[index] = (uint8_t)AS_NUMBER(value);
        pop(); pop(); pop();
        push(value);
      } else if (IS_MAP(receiver)) {
        ObjString *key = mapKeyString(indexVal);
        ObjMap *map = AS_MAP(receiver);
        tableSet(&map->table, key, value);
        pop(); pop(); pop();
        push(value);
      } else {
        runtimeError("Can only index lists, byte arrays, and maps.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}
void hack(bool b)
{
  run();
  if (b)
    hack(false);
}

InterpretResult interpret(const char *source)
{
  ObjFunction *function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure *closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}
//< interpret
