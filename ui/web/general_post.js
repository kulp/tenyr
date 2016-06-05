function make_go(prefix) {
    return function() {
        tenyr_state[prefix].out_area.value = "";
        tenyr_state[prefix].get_line_char.i = 0;
        return tenyr_state[prefix].app.callMain(tenyr_state[prefix].args);
    };
}

tenyr_state.tcc.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tcc.app = Module_tcc({
    'thisProgram':'tcc',
    'stdin':tenyr_state.tcc.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tcc.out_area.value += String.fromCharCode(ch);
    }
});
tenyr_state.tcc.go = make_go('tcc');

tenyr_state.tas.args = ['-ftext', '-'];
tenyr_state.tas.app = Module_tas({
    'thisProgram':'tas',
    'stdin':tenyr_state.tas.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tas.out_area.value += String.fromCharCode(ch);
    }
});
tenyr_state.tas.go = make_go('tas');

tenyr_state.tsim.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tsim.app = Module_tsim({
    'thisProgram':'tsim',
    'stdin':tenyr_state.tsim.get_line_char,
    'stdout':function(ch) {
        tenyr_state.tsim.out_area.value += String.fromCharCode(ch);
    }
});
tenyr_state.tsim.go = make_go('tsim');

