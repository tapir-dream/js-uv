{
  'variables': {
    'v8_use_snapshot%': 'true',
    'werror': '',
    'shared_v8%': 'false',
    'shared_libuv%': 'false',

    
  },

  'targets': [
    {
      'target_name': 'js2uv',
      'type': 'executable',

      'dependencies': [
  
      ],

      'include_dirs': [
        'src',
        'deps/uv/src/ares',
      ],

      'sources': [
        'src/init.cc',
        'src/init.h',
        'src/init.js',
        'src/util.h',
        'src/main.cc',
        'src/uvrun.cc',
        'src/uvrun.h',

        #added to the project by default.
        'common.gypi',
      ],

      'defines': [
        'NODE_WANT_INTERNALS=1',
        'ARCH="<(target_arch)"',
        'PLATFORM="<(OS)"',
      ],

      'conditions': [
        [ 'shared_v8=="false"', {
          'sources': [
            'deps/v8/include/v8.h',
            'deps/v8/include/v8-debug.h',
          ],
          'dependencies': [ 'deps/v8/tools/gyp/v8.gyp:v8' ],
        }],


        [ 'shared_libuv=="false"', {
          'dependencies': [ 'deps/uv/uv.gyp:libuv' ],
        }],

        [ 'OS=="win"', {
          'sources': [

          ],
          'defines': [
            'FD_SETSIZE=1024',
            # we need to use node's preferred "win32" rather than gyp's preferred "win"
            'PLATFORM="win32"',
            '_UNICODE=1',
          ],
          'libraries': [ '-lpsapi.lib' ]
        }, { # POSIX
          'defines': [ '__POSIX__' ],
        }],
        [ 'OS=="mac"', {
          'libraries': [ '-framework Carbon' ],
          'defines!': [
            'PLATFORM="mac"',
          ],
          'defines': [
            # we need to use node's preferred "darwin" rather than gyp's preferred "mac"
            'PLATFORM="darwin"',
          ],
        }],
        [ 'OS=="freebsd"', {
          'libraries': [
            '-lutil',
            '-lkvm',
          ],
        }],
        [ 'OS=="solaris"', {
          'libraries': [
            '-lkstat',
            '-lumem',
          ],
          'defines!': [
            'PLATFORM="solaris"',
          ],
          'defines': [
            # we need to use node's preferred "sunos"
            # rather than gyp's preferred "solaris"
            'PLATFORM="sunos"',
          ],
        }],
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'SubSystem': 1, # /subsystem:console
        },
      },
    },

    {
      'target_name': 'tests',
      'type': 'executable',
      'dependencies': [ 'js2uv' ],
      'sources': [
        'test/main.cc',
        'test/uv_run.js',
      ],
      'conditions': [

      ],
      'msvs-settings': {
        'VCLinkerTool': {
          'SubSystem': 1, # /subsystem:console
        },
      },
    },

 
    
    
  ] # end targets
}
