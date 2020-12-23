{
  "targets": [
    {
      "target_name": "screen-recorder",
      "conditions": [
        ['OS=="win"',
          {
            'msvs_settings':
              {
                'VCLinkerTool':
                  {
                    'AdditionalDependencies': ['screen-recorder.lib'],
                    'AdditionalLibraryDirectories': ['../platform/win32'],
                  },
              },
          },
        ],
      ],
      "include_dirs": [ ".","<!(node -e \"require('nan')\")" ],
      "sources": [ "./src/main.cc" ],
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/screen-recorder.node" ],
          "destination": "./platform/win32"
        }
      ]
    }
  ],
}
