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
          "<!@(pkg-config --cflags-only-I dbus-1 | sed s/-I//g)"
      ],
      "sources": [ "./src/dbus.cpp" ],
      "libraries": [
          "<!@(pkg-config --libs dbus-1)"
      ],
    },
  ]
}