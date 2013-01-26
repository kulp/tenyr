    return Module;
})((tasdata.tasModule = (function(){
        function put_char(ch)
        {
            tasdata.outarea.value += String.fromCharCode(ch);
        }

        function nothing()
        {
            return null;
        }

        function set_up_fs(mod)
        {
            var ENVIRONMENT_IS_NODE = typeof process === 'object';
            var ENVIRONMENT_IS_WEB = typeof window === 'object';
            var ENVIRONMENT_IS_WORKER = typeof importScripts === 'function';
            var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;
            if (ENVIRONMENT_IS_WEB) {
                mod._FSref.init(tasdata.get_pp_line_char, put_char, put_char);
            } else if (ENVIRONMENT_IS_NODE) {
                mod._FSref.init(nothing, put_char, put_char);
            }
        }

        var mod = {
            noInitialRun: true,
            preInit: [ function(){ set_up_fs(mod) } ],
        };

        return mod;
    })()
));

function assemble()
{
    tasdata.outarea.value = "";
    tasdata.get_pp_line_char.i = 0;
    tasdata.tasModule.run();
}

