#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
  (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type)
{
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;

  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif

  return object;
}
ObjBoundMethod *newBoundMethod(Value receiver,
                               ObjClosure *method)
{
  ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod,
                                       OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}
ObjClass *newClass(ObjString *name)
{
  ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name; // [klass]
  initTable(&klass->methods);
  return klass;
}
ObjClosure *newClosure(ObjFunction *function)
{
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *,
                                   function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++)
  {
    upvalues[i] = NULL;
  }

  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}
ObjFunction *newFunction()
{
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}

ObjList *newList()
{
  ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
  list->count = 0;
  list->capacity = 0;
  list->items = NULL;
  return list;
}

ObjInstance *newInstance(ObjClass *klass)
{
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}

ObjNative *newNative(NativeFn function)
{
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjArray *newArray()
{
  ObjArray *array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
  initValueArray(&array->elements);
  return array;
}

ObjFile *newFileObj(FILE *handle, ObjString *name)
{
  ObjFile *file = ALLOCATE_OBJ(ObjFile, OBJ_FILE);
  file->handle = handle;
  file->name = name;
  return file;
}

static ObjString *allocateString(char *chars, int length,
                                 uint32_t hash)
{
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  tableSet(&vm.strings, string, NIL_VAL);
  pop();

  return string;
}

static uint32_t hashString(const char *key, int length)
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++)
  {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *takeString(char *chars, int length)
{
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL)
  {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length)
{
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length,
                                        hash);
  if (interned != NULL)
    return interned;

  char *heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';

  return allocateString(heapChars, length, hash);
}

ObjUpvalue *newUpvalue(Value *slot)
{
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void printFunction(ObjFunction *function)
{
  if (function->name == NULL)
  {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}
void printObject(Value value)
{
  switch (OBJ_TYPE(value))
  {
  case OBJ_BOUND_METHOD:
    printFunction(AS_BOUND_METHOD(value)->method->function);
    break;
  case OBJ_CLASS:
    printf("%s", AS_CLASS(value)->name->chars);
    break;
  case OBJ_CLOSURE:
    printFunction(AS_CLOSURE(value)->function);
    break;
  case OBJ_FUNCTION:
    printFunction(AS_FUNCTION(value));
    break;
  case OBJ_INSTANCE:
    printf("%s instance",
           AS_INSTANCE(value)->klass->name->chars);
    break;
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_ARRAY:
  {
    ObjArray *arr = AS_ARRAY(value);
    printf("[");
    for (int i = 0; i < arr->elements.count; i++)
    {
      if (i > 0)
        printf(", ");
      printValue(arr->elements.values[i]);
    }
    printf("]");
    break;
  }
  case OBJ_LIST:
  {
    ObjList *list = AS_LIST(value);
    printf("[ ");
    for (int i = 0; i < list->count; i++)
    {
      printValue(list->items[i]);
      if (i < list->count - 1){
        printf(", ");
      }
    }
    printf(" ]");
    break;
  }
  case OBJ_FILE:
  {
    ObjFile *f = (ObjFile *)AS_OBJ(value);
    if (f->name != NULL)
    {
      printf("<file %s>", f->name->chars);
    }
    else
    {
      printf("<file>");
    }
    break;
  }
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  }
}
