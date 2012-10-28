var tasdata = { };

//document.onload = function() {
    tasdata.inarea  = document.getElementById('input');
    tasdata.pparea  = document.getElementById('preprocessed');
    tasdata.outarea = document.getElementById('output');

    tasdata.get_in_line_char = make_char_getter(tasdata.inarea);
    tasdata.get_pp_line_char = make_char_getter(tasdata.pparea);
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

