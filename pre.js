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

function nothing()
{
    return null;
}

function set_up_fs()
{
    var ENVIRONMENT_IS_NODE = typeof process === 'object';
    var ENVIRONMENT_IS_WEB = typeof window === 'object';
    var ENVIRONMENT_IS_WORKER = typeof importScripts === 'function';
    var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;
    if (ENVIRONMENT_IS_WEB) {
        FS.init(get_line_char, put_char, put_char);
    } else if (ENVIRONMENT_IS_NODE) {
        FS.init(nothing, put_char, put_char);
    }
}

var Module = {
    noInitialRun: true,
    preInit: [ set_up_fs ],
    //preRun: [ set_up_fs ],
};

function assemble()
{
    outarea.value = "";
    get_line_char.i = 0;
    Module.run();
}

