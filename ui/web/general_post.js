var GLOBALS = (typeof window !== 'undefined' ? window : global);

function make_go(state,prefix) {
    return function() {
        state[prefix].out_area.value = "";
        state[prefix].get_line_char.i = 0;
        return state[prefix].app.callMain(state[prefix].args);
    };
}

function make_mod(prefix) {
    return {
        'memoryInitializerPrefixURL': 'build/',
        'filePackagePrefixURL': 'build/',
        'thisProgram': prefix,
        'stdin': tenyr_state[prefix].get_line_char,
        'stdout': function(ch) {
            tenyr_state[prefix].out_area.value += String.fromCharCode(ch);
        }
    };
}

function set_up(state,prefix) {
    state[prefix].args = [];
    state[prefix].mod = make_mod(prefix);
    state[prefix].app = GLOBALS['Module_' + prefix](state[prefix].mod);
    state[prefix].go = make_go(state,prefix);
}

set_up(tenyr_state,'tcc');
set_up(tenyr_state,'tas');
Module_tdis = Module_tas;
set_up(tenyr_state,'tdis');
set_up(tenyr_state,'tsim');

tenyr_state.tas.args = ['-ftext', '-'];
tenyr_state.tdis.args = ['-ftext', '-qd', '-'];
tenyr_state.tsim.args = ['-n', '-rprealloc', '-rserial', '-remscript', '-ftext', '--param=emscripten.insns_per_anim_frame=1000', '-'];

