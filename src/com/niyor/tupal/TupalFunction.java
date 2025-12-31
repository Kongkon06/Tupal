package com.niyor.tupal;

import java.util.List;

class TupalFunction implements TupalCallable{
private final Stmt.Function declaration;
private final Environment closure;

TupalFunction(Stmt.Function declaration, Environment closure){
 this.closure = closure;
 this.declaration = declaration;
}

TupalFunction bind(TupalInstance instance){
  Environment environment = new Environment(closure);
  environment.define("this", instance);
  return new TupalFunction(declaration, environment);
}

@Override
public Object call(Interpreter interpreter,List<Object> arguments){
  Environment environment = new Environment(closure);

  for(int i =0 ; i < declaration.params.size() ; i++){
    environment.define(declaration.params.get(i).lexeme,
        arguments.get(i));
  }

  try {
   interpreter.executeBlock(declaration.body,environment);
  } catch (Return returnValue) {
      return returnValue.value;  
  }
  return null;
}
@Override
public int arity(){
  return declaration.params.size();
}
@Override
public String toString(){
  return "<fn " + declaration.name.lexeme + ">";
}
}
