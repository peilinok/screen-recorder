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
      ],
      "include_dirs": [ "." ],
      "sources": [ "./src/main.cc" ],
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/recorder.node" ],
          "destination": "./platform/win32"
        }
      ]
    }
  ],
}
