package com.niyor.tupal;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

class TupalClass implements TupalCallable {
  final String name;
  private final Map<String, TupalFunction> methods;

  TupalClass(String name, Map<String, TupalFunction> methods){
    this.name = name;
    this.methods = methods;
  }

  TupalFunction findMethod(String name){
  if(methods.containsKey(name)){
    return methods.get(name);
  }
  return null;
  }

  @Override
  public String toString(){
    return name;
  }
  @Override
  public Object call(Interpreter interpreter,List<Object> arguments){
    TupalInstance instance = new TupalInstance(this);
    return instance;
  }
  @Override
  public int arity(){
    return 0;
  }
}

