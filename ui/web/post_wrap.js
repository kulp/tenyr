    return Module;
})((tenyr_state[prefix + '_module'] = (function(){
        function put_char(ch)
        {
            tenyr_state[prefix + '_out_area']['value'] += String.fromCharCode(ch);
        }

        function set_up_fs(mod)
        {
            if (ENVIRONMENT_IS_WEB) {
                mod['_FSref_init'](tenyr_state[prefix + '_get_line_char'], put_char, put_char);
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

tenyr_state[prefix + '_entry'] = function ()
{
    if (ENVIRONMENT_IS_WEB) {
        tenyr_state[prefix + '_out_area']['value'] = "";
        tenyr_state[prefix + '_get_line_char']['i'] = 0;
    }
    tenyr_state[prefix + '_module']['callMain'](tenyr_state[prefix + '_module']['arguments']);
}

})(TENYR_BINARY_NAME, tenyr_state);
