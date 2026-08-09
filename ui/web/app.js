// Browser UI glue for the emscripten-compiled tsim.
//
// tsim.js is an auto-running emscripten module (not MODULARIZE).  We set up
// a global Module config with noInitialRun=true BEFORE loading tsim.js so
// that main() is not called automatically; instead we call Module.callMain()
// ourselves after loading the program binary into the virtual filesystem.
//
// This adapts the original web demo's approach (which used MODULARIZE) for
// modern emscripten's auto-run mode.

var simModule = null;       // the emscripten Module instance
var inputQueue = [];        // pending stdin characters (bytes 0–255)

var outputEl   = document.getElementById('output');
var runBtn     = document.getElementById('run-btn');
var stopBtn    = document.getElementById('stop-btn');
var progSelect = document.getElementById('select-program');
var speedInput = document.getElementById('speed');
var stdinForm  = document.getElementById('input-form');
var stdinInput = document.getElementById('stdin');

// ---------------------------------------------------------------------------
// Module configuration — set as a global BEFORE tsim.js is loaded.
// tsim.js reads this via `var Module = typeof Module != 'undefined' ? Module : {};`
// ---------------------------------------------------------------------------
var Module = {
  noInitialRun: true,    // we call main() ourselves via callMain()
  noExitRuntime: true,   // keep the runtime alive between runs
  locateFile: function (path) {
    return path;          // tsim.wasm sits alongside this HTML page
  },
  stdout: function (ch) {
    if (ch !== null && ch !== undefined) {
      outputEl.textContent += String.fromCharCode(ch);
      outputEl.scrollTop = outputEl.scrollHeight;
    }
  },
  stderr: function (ch) {
    if (ch !== null && ch !== undefined) {
      outputEl.textContent += String.fromCharCode(ch);
      outputEl.scrollTop = outputEl.scrollHeight;
    }
  },
  stdin: function () {
    if (inputQueue.length === 0) return null;  // EOF / no data available
    return inputQueue.shift();
  },
  onRuntimeInitialized: function () {
    simModule = Module;
    runBtn.disabled = false;
    outputEl.textContent += '[ready — select a program and click Run]\n';
  }
};

// ---------------------------------------------------------------------------
// Dynamically load tsim.js AFTER the config above is in place.
// ---------------------------------------------------------------------------
var script = document.createElement('script');
script.src = 'tsim.js';
script.onload = function () {
  // tsim.js auto-runs its preamble but not main() (noInitialRun is true).
  // onRuntimeInitialized fires when the module/WASM is ready for callMain().
};
document.head.appendChild(script);

// ---------------------------------------------------------------------------
// Arguments passed to tsim — match the recipe_emscript event-loop recipe
// so the browser stays responsive during simulation.
// ---------------------------------------------------------------------------
function buildArgs() {
  return [
    '-n',                              // skip default recipes (tsimrc, plugin)
    '-r', 'sparse',                    // sparse memory backend
    '-r', 'serial',                    // serial device ← stdin/stdout
    '-r', 'top_page',                  // map a page at the highest addresses
    '-r', 'emscript',                  // event-loop recipe (non-blocking)
    '-p', 'emscripten.insns_per_anim_frame=' + speedInput.value,
    'prog.bin'                         // relative path — resolves in MEMFS
  ];
}

// ---------------------------------------------------------------------------
// UI event handlers
// ---------------------------------------------------------------------------
stdinForm.addEventListener('submit', function (e) {
  e.preventDefault();
  var text = stdinInput.value;
  if (text) {
    for (var i = 0; i < text.length; i++) {
      inputQueue.push(text.charCodeAt(i));
    }
    inputQueue.push('\n'.charCodeAt(0));  // newline terminator
    stdinInput.value = '';
  }
});

runBtn.addEventListener('click', function () {
  outputEl.textContent = '';
  runBtn.disabled = true;
  stopBtn.disabled = false;

  fetch(progSelect.value)
    .then(function (r) { return r.arrayBuffer(); })
    .then(function (buf) {
      // Load the binary into the virtual filesystem at /prog.bin
      var binary = new Uint8Array(buf);

      // Remove any previous file so createDataFile doesn't fail.
      try { simModule.FS.unlink('/prog.bin'); } catch (e) { /* not there yet */ }
      simModule.FS.createDataFile('/', 'prog.bin', binary, true, false);

      // Run the simulator — this calls main() which processes recipes,
      // loads the image, and runs the event-loop recipe (recipe_emscript).
      simModule.callMain(buildArgs());
    })
    .catch(function (err) {
      console.error('Failed to load program:', err);
      outputEl.textContent = 'Error: failed to load program: ' + err;
    });
});

stopBtn.addEventListener('click', function () {
  if (simModule) {
    var cancel = simModule._emscripten_cancel_main_loop ||
                 simModule.emscripten_cancel_main_loop;
    if (cancel) cancel();
  }
  outputEl.textContent += '\n[simulator stopped]\n';
  runBtn.disabled = false;
  stopBtn.disabled = true;
});
