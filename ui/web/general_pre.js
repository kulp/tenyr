//document.onload = function() {
    tenyr_state.tcc_in_area  = document.getElementById('input');
    tenyr_state.tcc_out_area = tenyr_state.tas_in_area  = document.getElementById('preprocessed');
    tenyr_state.tas_out_area = tenyr_state.tsim_in_area = document.getElementById('output');
    tenyr_state.tsim_out_area = document.getElementById('simoutput');

    tenyr_state.tcc_get_line_char = make_char_getter(tenyr_state.tcc_in_area);
    tenyr_state.tas_get_line_char = make_char_getter(tenyr_state.tas_in_area);
    tenyr_state.tsim_get_line_char = make_char_getter(tenyr_state.tas_out_area);
//};

tenyr_state.tcc = { };
tenyr_state.tas = { };
tenyr_state.tsim = { };

function make_char_getter(area)
{
    return function() {
        me = arguments.callee;
        if (typeof me.i == 'undefined') {
            me.i = 0;
        }

        var what = area.value;
        if (me.i < what.length) {
            var result = what.charCodeAt(me.i++);
            return result;
        }

        return null;
    }
}

