const sharedMemoryAddon = require("./build/Release/sharedMemory");
const dbusAddon = require("./build/Release/dbus");

const sharedMemoryNameMaxLength = 32;
const dBusMessageMaxContentLength = 4096;

const dbusBusFlag = {
  DBUS_NAME_FLAG_ALLOW_REPLACEMENT: 0x1,
  DBUS_NAME_FLAG_REPLACE_EXISTING: 0x2,
  DBUS_NAME_FLAG_DO_NOT_QUEUE: 0x4,
};

function isBuffer(value) {
  return (
    value &&
    value.buffer instanceof ArrayBuffer &&
    value.byteLength !== undefined
  );
}

function strEncodeUTF16(str) {
  var buf = new ArrayBuffer(str.length * 2);
  var bufView = new Uint16Array(buf);
  for (var i = 0, strLen = str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return bufView;
}

function bufferFromData(data, encoding) {
  if (isBuffer(data)) {
    return data;
  } else if (data && typeof data === "string") {
    if (encoding === "utf16") {
      return strEncodeUTF16(data);
    } else {
      return Buffer.from(data, encoding || "utf8");
    }
  }

  return Buffer.from(data);
}

// shared memory
function createSharedMemory(name, fileMode, memorySize) {
  if (name.length > sharedMemoryNameMaxLength) {
    throw `shared memory name length cannot be greater than ${sharedMemoryNameMaxLength}`;
  }

  const handle = Buffer.alloc(sharedMemoryAddon.sizeof_SharedMemoryHandle);

  const res = sharedMemoryAddon.CreateSharedMemory(
    name,
    fileMode,
    memorySize,
    handle
  );

  if (res !== 0) {
    throw `could not create shared memory ${name}: ${res}`;
  }

  return handle;
}

function openSharedMemory(name, memorySize) {
  if (name.length > sharedMemoryNameMaxLength) {
    throw `shared memory name length cannot be greater than ${sharedMemoryNameMaxLength}`;
  }

  const handle = Buffer.alloc(sharedMemoryAddon.sizeof_SharedMemoryHandle);

  const res = sharedMemoryAddon.OpenSharedMemory(name, memorySize, handle);

  if (res !== 0) {
    throw `could not open shared memory ${name}: ${res}`;
  }

  return handle;
}

function writeSharedData(handle, data, encoding) {
  const buf = bufferFromData(data, encoding);
  const res = sharedMemoryAddon.WriteSharedData(handle, buf, buf.byteLength);

  if (res === -1) {
    throw `data size (${data.length()}) exceeded maximum shared memory size`;
  }
}

function readSharedData(handle, encoding, bufferSize) {
  const dataSize = bufferSize || sharedMemoryAddon.GetSharedMemorySize(handle);
  const buf = Buffer.alloc(dataSize);

  const res = sharedMemoryAddon.ReadSharedData(handle, buf, dataSize);

  if (res === -1) {
    throw `data size (${data.length()}) exceeded maximum shared memory size`;
  }

  if (encoding) {
    // is a string
    return buf.toString(encoding).replace(/\0/g, ""); // remove trailing \0 characters
  }

  return buf;
}

function closeSharedMemory(handle) {
  sharedMemoryAddon.CloseSharedMemory(handle);
}

// dbus
function initializeDBusConnection(serverName, busType, busFlags) {
  const handle = Buffer.alloc(dbusAddon.sizeof_DBusConnectionHandle);
  const res = dbusAddon.InitializeDBusConnection(
    serverName,
    busType,
    busFlags || dbusBusFlag.DBUS_NAME_FLAG_REPLACE_EXISTING,
    handle
  );

  if (res !== 0) {
    throw `could not initialize dbus connection ${serverName}: ${res}`;
  }

  return handle;
}

function openDBusConnection(busType) {
  const handle = Buffer.alloc(dbusAddon.sizeof_DBusConnectionHandle);
  const res = dbusAddon.OpenDBusConnection(busType, handle);

  if (res !== 0) {
    throw `could not open dbus connection: ${res}`;
  }

  return handle;
}

function enqueueDBusMethodCall(
  handle,
  targetName,
  objectName,
  interfaceName,
  methodName,
  cmdType,
  data,
  encoding
) {
  const fObject = objectName || `/${targetName.replaceAll(".", "/")}`;
  const fInterface = interfaceName || targetName;

  const buf = bufferFromData(data || "", encoding);
  const res = dbusAddon.EnqueueDBusMethodCall(
    handle,
    targetName,
    fObject,
    fInterface,
    methodName,
    cmdType,
    buf,
    buf.byteLength
  );

  if (res === 1) {
    throw `data size (${data.length()}) exceeded maximum msg content size (${dBusMessageMaxContentLength})`;
  } else if (res !== 0) {
    throw `could not call dbus method: ${res}`;
  }
}

function listenDBusMethodCall(handle, interfaceName, methodName, encoding) {
  const buf = Buffer.alloc(dBusMessageMaxContentLength);
  const cmdTypeHandle = Buffer.alloc(dbusAddon.sizeof_CmdTypeHandle);

  const res = dbusAddon.ListenDBusMethodCall(
    handle,
    interfaceName || "",
    methodName,
    cmdTypeHandle,
    buf,
    dBusMessageMaxContentLength
  );

  if (res === 1) {
    throw `data buffer size is less than maximum content size (${dBusMessageMaxContentLength})`;
  } else if (res === 2) {
    throw "not connected anymore";
  } else if (res !== 0) {
    throw `could not listen to dbus method call: ${res}`;
  }

  return {
    cmdType: cmdTypeHandle[0].valueOf(),
    content: encoding
      ? buf.toString(encoding).replace(/\0/g, "") // is a string, remove trailing \0 characters
      : buf,
  };
}

function closeDBusConnection(handle) {
  const res = dbusAddon.CloseDBusConnection(handle);

  if (res !== 0) {
    throw `could not close dbus connection: ${res}`;
  }
}

module.exports = {
  createSharedMemory,
  openSharedMemory,
  writeSharedData,
  readSharedData,
  closeSharedMemory,

  initializeDBusConnection,
  openDBusConnection,
  listenDBusMethodCall,
  enqueueDBusMethodCall,
  closeDBusConnection,

  sharedMemoryFileMode: {
    S_IRWXU: 0o700 /* [XSI] RWX mask for owner */,
    S_IRUSR: 0o400 /* [XSI] R for owner */,
    S_IWUSR: 0o200 /* [XSI] W for owner */,
    S_IXUSR: 0o100 /* [XSI] X for owner */,

    /* Read, write, execute/search by group */
    S_IRWXG: 0o70 /* [XSI] RWX mask for group */,
    S_IRGRP: 0o40 /* [XSI] R for group */,
    S_IWGRP: 0o20 /* [XSI] W for group */,
    S_IXGRP: 0o10 /* [XSI] X for group */,

    /* Read, write, execute/search by others */
    S_IRWXO: 0o7 /* [XSI] RWX mask for other */,
    S_IROTH: 0o4 /* [XSI] R for other */,
    S_IWOTH: 0o2 /* [XSI] W for other */,
    S_IXOTH: 0o1 /* [XSI] X for other */,

    S_ISUID: 0o4000 /* [XSI] set user id on execution */,
    S_ISGID: 0o2000 /* [XSI] set group id on execution */,
    S_ISVTX: 0o1000 /* [XSI] directory restrcted delete */,
  },
  dbusBusType: {
    DBUS_BUS_SESSION: 0,
    DBUS_BUS_SYSTEM: 1,
    DBUS_BUS_STARTER: 2,
  },
  dbusBusFlag,
};
