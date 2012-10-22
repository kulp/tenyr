var inarea  = document.getElementById('input');
var outarea = document.getElementById('output');
function get_line_char()
{
    me = arguments.callee;
    if (typeof me.i == 'undefined') {
        me.i = 0;
    }

    var what = inarea.value;
    if (me.i < what.length) {
        var result = what.charCodeAt(me.i++);
        return result;
    }

    return null;
}

function put_char(ch)
{
    outarea.value += String.fromCharCode(ch);
}

function set_up_fs()
{
    FS.init(get_line_char, put_char, put_char);
}

var Module = {
    noInitialRun: true,
    preInit: [ set_up_fs ],
    //preRun: [ set_up_fs ],
};

function assemble()
{
    //outarea.value = "";
    Module.run();
}

