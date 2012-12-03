%module TestExec
%{
#include "test_interp_glue.h"
%}
%include cpointer.i

%typemap(in) char** {
    AV* av =   SvRV($input);
    char** result = NULL;

    if (SvTYPE(av) == SVt_PVAV) {
	int array_len =  av_len(av);

	if (array_len < 0) {
	    result = NULL; 
	} else {
	    int i;
	    STRLEN str_len;
	    char* str;
	    SV** cur_sv;
	    result = (char**) malloc((sizeof(char*) * (array_len + 2)));

	    for (i = 0; i <= array_len; i++) {
		cur_sv = av_fetch(av, i, 0);
		str = SvPV(*cur_sv, str_len);
		result[i] = (char*) malloc(str_len+1);
		strcpy(result[i], str);
	    }

	    result[array_len + 1] = NULL;
	}
    } else {
	result = NULL;
    }

    $1 = result;
}

%typemap(out) char** {
    char** from_c = $1;
    char** tmp;
    int i;
    int num_el = 0;

    if (from_c) {
 	tmp = from_c;
	while (*tmp != NULL) {
	    num_el++;
	    tmp++;
	}

	EXTEND(sp, num_el);

	tmp = from_c;
	for (i = 0; i < num_el; i++) {
	    ST(argvi) = sv_newmortal();
	    sv_setpv(ST(argvi++), tmp[i]);
	    free(tmp[i]);
	}
	free(from_c);
    }
}

%typemap(out) char* {
    char* from_c = $1;
    //int i;
    //int num_el = 0;

    if (from_c) {
        ST(argvi) = sv_newmortal();
        sv_setpv(ST(argvi++), from_c);
        free(from_c);
    }
}

%include test_interp_glue.h
