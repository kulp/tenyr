    return Module;
})((tasdata[prefix + '_module'] = (function(){
        function put_char(ch)
        {
            tasdata[prefix + '_out_area']['value'] += String.fromCharCode(ch);
        }

        function set_up_fs(mod)
        {
            if (ENVIRONMENT_IS_WEB) {
                mod['_FSref_init'](tasdata[prefix + '_get_line_char'], put_char, put_char);
            }
        }

        var mod = {
            'noInitialRun': !ENVIRONMENT_IS_NODE,
            'preRun': [ function(){ set_up_fs(mod) } ],
            'noExitRuntime': !ENVIRONMENT_IS_NODE,
        };

        return mod;
    })()
));

tasdata[prefix + '_entry'] = function ()
{
    if (ENVIRONMENT_IS_WEB) {
        tasdata[prefix + '_out_area']['value'] = "";
        tasdata[prefix + '_get_line_char']['i'] = 0;
    }
    tasdata[prefix + '_module']['callMain'](tasdata[prefix + '_module']['arguments']);
}

})(TENYR_BINARY_NAME, tasdata);
