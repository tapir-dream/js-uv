var assert = require('assert');
var binding = require('./build/Release/binding');

console.log('detect file or directory', binding.detect("z:\\1"));