Module['preInit'] = Module['preInit'] || [];
Module['preInit'].push(function () {
  if (ENVIRONMENT_IS_NODE) {
    Module['stdout'] = function(x) { process.stdout.write(String.fromCharCode(x & 255), 'binary'); };
    Module['stdin'] = function stdin_reader () {
      if (!stdin_reader.input || stdin_reader.input.length == 0) {
        var fs = require('fs');
        var result = null;
        var BUFSIZE = 256;
        var buf = new Buffer(BUFSIZE, 'binary');
        var fd = process.stdin.fd;

        var bytesRead = fs.readSync(fd, buf, 0, BUFSIZE, null);

        if (bytesRead > 0) {
          result = buf.slice(0, bytesRead);
        } else {
          result = null;
        }

        stdin_reader.input = result;
      }

      if (stdin_reader.input && stdin_reader.input.length > 0) {
        var result = stdin_reader.input[0];
        stdin_reader.input = stdin_reader.input.slice(1);
        return result;
      } else {
        return null;
      }
    };
  }
});

