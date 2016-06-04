tenyr_state.tcc.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tcc.app = Module_tcc({
    'thisProgram':'tcc',
    'stdin':tenyr_state.tcc_get_line_char,
    'stdout':function(ch) {
        tenyr_state.tcc_out_area.value += String.fromCharCode(ch);
    }
});

tenyr_state.tas.args = ['-ftext', '-'];
tenyr_state.tas.app = Module_tas({
    'thisProgram':'tas',
    'stdin':tenyr_state.tas_get_line_char,
    'stdout':function(ch) {
        tenyr_state.tas_out_area.value += String.fromCharCode(ch);
    }
});

tenyr_state.tsim.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tsim.app = Module_tsim({
    'thisProgram':'tsim',
    'stdin':tenyr_state.tsim_get_line_char,
    'stdout':function(ch) {
        tenyr_state.tsim_out_area.value += String.fromCharCode(ch);
    }
});

