var mod;

var inarea  = document.getElementById('input');
var pparea  = document.getElementById('preprocessed');
var outarea = document.getElementById('output');

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

var get_in_line_char = make_char_getter(inarea);
var get_pp_line_char = make_char_getter(pparea);

