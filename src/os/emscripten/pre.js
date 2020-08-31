// --pre-js inserts this code before ENVIRONMENT_IS_NODE is available, so approximate it here
if (typeof process === 'object' && typeof process.versions === 'object' && typeof process.versions.node === 'string') {
  Module['stdout'] = function(x) { process.stdout.write(String.fromCharCode(x & 255), 'binary'); };
  Module['stdin'] = function stdin_reader () {
    var me = stdin_reader;
    if (!me.input || me.input.length == 0) {
      me.fs = me.fs || require('fs');
      var result = null;
      var BUFSIZE = 256;
      me.buf = me.buf || Buffer.alloc(BUFSIZE, 0, 'binary');
      me.fd = me.fd || me.fs.openSync('/dev/stdin', 'r');

      var again = false;
      var bytesRead = 0;
      do {
        try {
          bytesRead = me.fs.readSync(me.fd, me.buf, 0, BUFSIZE, null);
          again = false;
        } catch (e) {
          again = e.toString().indexOf('EAGAIN') != -1;
        }
      } while (again);

      if (bytesRead > 0) {
        result = me.buf.slice(0, bytesRead);
      } else {
        result = null;
      }

      me.input = result;
    }

    if (me.input && me.input.length > 0) {
      var result = me.input[0];
      me.input = me.input.slice(1);
      return result;
    } else {
      return null;
    }
  };
}

