var tenyr_state; if (!tenyr_state) tenyr_state = {};
(function(prefix, tenyr_state){
var ENVIRONMENT_IS_NODE = typeof process === 'object';
var ENVIRONMENT_IS_WEB = typeof window === 'object';
var ENVIRONMENT_IS_WORKER = typeof importScripts === 'function';
var ENVIRONMENT_IS_SHELL = !ENVIRONMENT_IS_WEB && !ENVIRONMENT_IS_NODE && !ENVIRONMENT_IS_WORKER;

//if (typeof tenyr_state == 'undefined') { tenyr_state = { } }
(function(Module){
