package com.niyor.tupal;

import java.util.List;

interface TupalCallable{
  int arity();
  Object call(Interpreter interpreter,List<Object> arguments);
}
