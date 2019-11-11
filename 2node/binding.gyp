{
  "targets": [
    {
      "target_name": "recorder",
      "conditions": [
        ['OS=="win"',
          {
            'msvs_settings':
              {
                'VCLinkerTool':
                  {
                    'AdditionalDependencies': ['recorder.lib'],
                    'AdditionalLibraryDirectories': ['../platform/win32'],
                  },
              },
          },
        ],
        ['OS=="mac"',
          {
            'sources': [ './src/utils.mm' ],
            'defines': ['__MAC__'],
            'xcode_settings':
            {
              'MACOSX_DEPLOYMENT_TARGET': '10.10',
            },
            'link_settings':
            {
              'libraries': ['/Library/Frameworks/recorder.framework'],
            },
          },
        ],
      ],
      "include_dirs": [ "." ],
      "sources": [ "./src/main.cc" ],
    },
  ],
}
