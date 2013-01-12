%module TestExec
%{
#include "test_interp_glue.h"
%}
%include cpointer.i

%typemap(in) char ** {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($1);
	return NULL;
      }
    }
    $1[i] = 0;
for (i = 0; i < size; i++) { printf("s=%s\n", $1[i]);}

  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

//// This cleans up the char ** array we malloc'd before the function call
//%typemap(freearg) char ** {
//  free((char *) $1);
//}

%typemap(out) char** {
  int len,i;
  len = 0;
  while ($1[len]) len++;
  $result = PyList_New(len);
  for (i = 0; i < len; i++) {
    PyList_SetItem($result,i,PyString_FromString($1[i]));
  }
}


%typemap(out) char* {
    char* from_c = $1;

    if (from_c) {
        $result=PyString_FromString(from_c);
        free(from_c);
    }
}


%include test_interp_glue.h
