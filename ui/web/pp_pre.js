var ppModule = Module = (function(){
    var Module = {
        noInitialRun: true,
        preInit: [ set_up_fs ],
        //preRun: [ set_up_fs ],
        arguments: [ "-Ilib" ],
    };

    function put_char(ch)
    {
        pparea.value += String.fromCharCode(ch);
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
            FS.init(get_in_line_char, put_char, put_char);
        } else if (ENVIRONMENT_IS_NODE) {
            FS.init(nothing, put_char, put_char);
        }
    }

    /*
    function assemble()
    {
        outarea.value = "";
        get_pp_line_char.i = 0;
        Module.run();
    }
    */

    return Module;
})();

function preprocess()
{
    pparea.value = "";
    get_in_line_char.i = 0;
    ppModule.run();
}

