var INHIBIT_RUN = true;

tenyr_state.tcc = { };
tenyr_state.tas = { };
tenyr_state.tdis = { };
tenyr_state.tsim = { };

tenyr_state.tcc.in_area  = document.getElementById('input');
tenyr_state.tcc.out_area = tenyr_state.tas.in_area  = document.getElementById('preprocessed');
tenyr_state.tas.out_area = tenyr_state.tsim.in_area = document.getElementById('output');
tenyr_state.tdis.in_area = tenyr_state.tas.out_area;
tenyr_state.tdis.out_area = tenyr_state.tas.in_area;
tenyr_state.tsim.out_area = document.getElementById('simoutput');

tenyr_state.tcc.get_line_char = make_char_getter(tenyr_state.tcc.in_area);
tenyr_state.tas.get_line_char = make_char_getter(tenyr_state.tas.in_area);
tenyr_state.tdis.get_line_char = make_char_getter(tenyr_state.tdis.in_area);
tenyr_state.tsim.get_line_char = make_char_getter(tenyr_state.tsim.in_area);

function make_char_getter(area)
{
    return function me () {
        if (typeof me.i == 'undefined') {
            me.i = 0;
        }

        var what = area.value;
        if (me.i < what.length) {
            return what.charCodeAt(me.i++);
        } else if (me.i++ == what.length) {
            // Supply a final newline to ensure parse succeeds
            return '\n'.charCodeAt(0);
        }

        return null;
    }
}

