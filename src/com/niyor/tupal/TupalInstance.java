package com.niyor.tupal;

import java.util.HashMap;
import java.util.Map;

class TupalInstance{
  private TupalClass klass;
  private final Map<String, Object> fields = new HashMap<>();

  TupalInstance(TupalClass klass){
    this.klass = klass;
  }

  Object get(Token name){
    if(fields.containsKey(name.lexeme)){
      return fields.get(name.lexeme);
    }
    TupalFunction method = klass.findMethod(name.lexeme);
    if(method != null) return method.bind(this);

    throw new RuntimeError(name,"Undefined Property '" + name.lexeme + "'");
  }

  void set(Token name, Object value){
    fields.put(name.lexeme, value);
  };

  @Override
  public String toString(){
    return klass.name + " instance";
  }
  
}
