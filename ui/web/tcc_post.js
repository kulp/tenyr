    return Module;
})((tasdata['tccModule'] = (function(){
        function put_char(ch)
        {
            tasdata['pparea']['value'] += String.fromCharCode(ch);
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
                mod['_FSref_init'](tasdata['get_in_line_char'], put_char, put_char);
            } else if (ENVIRONMENT_IS_NODE) {
                mod['_FSref_init'](nothing, put_char, put_char);
            }
        }

        var mod = {
            'noInitialRun': true,
            'preInit': [ function(){ set_up_fs(mod) } ],
            'arguments': [ "-Ilib" ],
            'noExitRuntime': true,
        };

        return mod;
    })()
));

tasdata['preprocess'] = function ()
{
    tasdata['pparea']['value'] = "";
    tasdata['get_in_line_char']['i'] = 0;
    tasdata['tccModule']['callMain'](tasdata['tccModule']['arguments']);
}

