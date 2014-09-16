    return Module;
})((tenyr_state[prefix + '_module'] = (function(){
        var mod = {
            'noInitialRun': !ENVIRONMENT_IS_NODE,
            'noExitRuntime': !ENVIRONMENT_IS_NODE,
			/* hack around invisibility of FS due to wrapping all code in an
			 * extra function scope to permit multiple emscripten scripts in
			 * one HTML page */
			'_set_up_fs':
				function(FS) {
					if (ENVIRONMENT_IS_WEB) {
						function put_char(ch) {
							tenyr_state[prefix + '_out_area']['value'] += String.fromCharCode(ch);
						}

						FS.init(tenyr_state[prefix + '_get_line_char'], put_char, put_char);
					}
				},
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
