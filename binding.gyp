{
  "targets": [
    {
      "target_name": "sharedMemory",
      "include_dirs": [
          "<!(node -e \"require('napi-macros')\")"
      ],
      "sources": [ "./src/sharedMemory.cpp" ],
      "libraries": [],
    },
    {
      "target_name": "dbus",
      "include_dirs": [
          "<!(node -e \"require('napi-macros')\")",
          "/usr/include/dbus-1.0",
          "/usr/lib/x86_64-linux-gnu/dbus-1.0/include"
      ],
      "sources": [ "./src/dbus.cpp" ],
      "libraries": [
          "-ldbus-1"
      ],
    },
  ]
}