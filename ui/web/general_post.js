tenyr_state.tcc.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tcc.app = Module_tcc({
    'thisProgram':'tcc',
    'stdin':tenyr_state.tcc.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tcc.out_area.value += String.fromCharCode(ch);
    }
});

tenyr_state.tas.args = ['-ftext', '-'];
tenyr_state.tas.app = Module_tas({
    'thisProgram':'tas',
    'stdin':tenyr_state.tas.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tas.out_area.value += String.fromCharCode(ch);
    }
});

tenyr_state.tsim.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tsim.app = Module_tsim({
    'thisProgram':'tsim',
    'stdin':tenyr_state.tsim.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tsim.out_area.value += String.fromCharCode(ch);
    }
});

