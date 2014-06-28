//document.onload = function() {
    tasdata.tcc_in_area  = document.getElementById('input');
    tasdata.tcc_out_area = tasdata.tas_in_area  = document.getElementById('preprocessed');
    tasdata.tas_out_area = tasdata.tsim_in_area = document.getElementById('output');
    tasdata.tsim_out_area = document.getElementById('simoutput');

    tasdata.tcc_get_line_char = make_char_getter(tasdata.tcc_in_area);
    tasdata.tas_get_line_char = make_char_getter(tasdata.tas_in_area);
    tasdata.tsim_get_line_char = make_char_getter(tasdata.tas_out_area);
//};

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

