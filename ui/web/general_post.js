tenyr_state.tsim.args = ['-n', '-rprealloc', '-rserial', '-ftext', '-'];
tenyr_state.tsim.app = Module_tsim({
    'thisProgram':'tsim',
    'stdin':tenyr_state.tsim_get_line_char,
    'stdout':function(ch) {
        tenyr_state.tsim_out_area.value += String.fromCharCode(ch);
    }
});

