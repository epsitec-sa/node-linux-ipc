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
      "sources": [ "./src/dbus.cpp" ],
      "include_dirs": [
          "<!(node -e \"require('napi-macros')\")"
      ],
      "cflags": [
          "<!@(pkg-config --cflags dbus-1)"
      ],
      "libraries": [
          "<!@(pkg-config --libs dbus-1)"
      ],
    },
  ]
}