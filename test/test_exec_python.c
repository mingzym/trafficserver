//#include <stdio.h>
#include <Python.h>

extern void init_TestExec(void);

void run_script(int argc, char** argv) {
  FILE* exp_file;

  Py_SetProgramName("test_exec");  /* set prog name to test_exec */
  Py_Initialize();
  init_TestExec();  /* init local module */
  PySys_SetArgvEx(argc, argv, 1);
  // TODO: we should set the ats_deft home into sys.path

  PyRun_SimpleString("import sys \n");
  PyRun_SimpleString("sys.path.append('~/ats_deft') \n");
  PyRun_SimpleString("sys.path.append('.') \n");

  // we import TestExec in the global, hopes we can save some typing
  PyRun_SimpleString("from TestExec import * \n");

  PyRun_SimpleString("import sys\n");
//  PyRun_SimpleString("print sys.builtin_module_names\n");
//  PyRun_SimpleString("print sys.modules.keys()\n");
//  PyRun_SimpleString("print sys.executable\n");
//  PyRun_SimpleString("print sys.argv\n");
  PyRun_SimpleString("print sys.path \n");

  exp_file = fopen(argv[0], "r");
  PyRun_SimpleFile(exp_file, argv[0]);
  Py_Finalize();
}
