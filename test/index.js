/*global describe,it*/

const assert = require("assert");
const lib = require("../");
//const synchronize = require("node-windows-synchronize");

describe("CreateSharedMemory", function () {
  it("should create shared memory", function () {
    const handle = lib.createSharedMemory(
      "/TestSharedMemory",
      lib.sharedMemoryFileMode.S_IRUSR | lib.sharedMemoryFileMode.S_IWUSR,
      4096
    );

    assert.ok(handle);

    lib.closeSharedMemory(handle);
  });
});

describe("OpenSharedMemory", function () {
  it("should create and open shared memory", function () {
    const cHandle = lib.createSharedMemory(
      "/TestSharedMemory",
      lib.sharedMemoryFileMode.S_IRUSR | lib.sharedMemoryFileMode.S_IWUSR,
      4096
    );
    const oHandle = lib.openSharedMemory("/TestSharedMemory", 4096);

    assert.ok(cHandle);
    assert.ok(oHandle);

    lib.closeSharedMemory(oHandle);
    lib.closeSharedMemory(cHandle);
  });
});

describe("WriteSharedMemory", function () {
  it("should create and write into shared memory", function () {
    const handle = lib.createSharedMemory(
      "/TestSharedMemory",
      lib.sharedMemoryFileMode.S_IRUSR | lib.sharedMemoryFileMode.S_IWUSR,
      4096
    );

    lib.writeSharedData(handle, "hello world!!");

    assert.ok(handle);

    lib.closeSharedMemory(handle);
  });
});

describe("ReadSharedMemory", function () {
  it("should create, open, write and read from shared memory", function () {
    const cHandle = lib.createSharedMemory(
      "/TestSharedMemory",
      lib.sharedMemoryFileMode.S_IRUSR | lib.sharedMemoryFileMode.S_IWUSR,
      4096
    );
    const oHandle = lib.openSharedMemory("/TestSharedMemory", 4096);

    assert.ok(cHandle);
    assert.ok(oHandle);

    lib.writeSharedData(cHandle, "hello wòrld!!");
    const text = lib.readSharedData(oHandle, "utf8");

    assert.strictEqual("hello wòrld!!", text);

    lib.closeSharedMemory(oHandle);
    lib.closeSharedMemory(cHandle);
  });
});
/*
describe("WriteSharedMemorySerial", function () {
  it("should create and write in a serial way to shared memory", function () {
    const smHandle = lib.createSharedMemory(
      "Local\\TestSharedMemory",
      lib.sharedMemoryPageAccess.ReadWrite,
      lib.sharedMemoryFileMapAccess.AllAccess,
      4096
    );
    const mHandle = synchronize.createMutex("Local\\TestMutex");

    assert.ok(smHandle);
    assert.ok(mHandle);

    for (let i = 0; i < 100000; i++) {
      synchronize.waitMutex(mHandle, synchronize.waitTime.Infinite);

      lib.writeSharedData(smHandle, "hello world: " + i);

      synchronize.releaseMutex(mHandle);
    }

    const text = lib.readSharedData(smHandle, "utf8");
    assert.strictEqual("hello world: 99999", text);

    synchronize.closeMutex(mHandle);
    lib.closeSharedMemory(smHandle);
  });
});
*/

describe("CreateDBusConnection", function () {
  it("should create a dbus connection", function () {
    const handle = lib.initializeDBusConnection(
      "epsitec.monolith.Test",
      lib.dbusBusType.DBUS_BUS_SESSION
    );

    assert.ok(handle);

    lib.closeDBusConnection(handle);
  });
});

describe("CallDBUSMethod", function () {
  it("should call a method through dbus", function () {
    const rHandle = lib.initializeDBusConnection(
      "epsitec.monolith.Test3.receiver",
      lib.dbusBusType.DBUS_BUS_SESSION
    );
    const sHandle = lib.openDBusConnection(lib.dbusBusType.DBUS_BUS_SESSION);

    assert.ok(rHandle);
    assert.ok(sHandle);

    lib.callDBusMethodAsync(
      sHandle,
      "epsitec.monolith.Test3.receiver", // target for the method call
      "/epsitec/monolith/Test", // object to call on
      "epsitec.monolith.Test", // interface to call on
      "Method",
      1,
      "hello world"
    );

    lib.closeDBusConnection(sHandle);
    lib.closeDBusConnection(rHandle);
  });
});

describe("CallUpdaterDBUSMethod", function () {
  it("should call a method on the updater through dbus", function () {
    const sHandle = lib.openDBusConnection(lib.dbusBusType.DBUS_BUS_SYSTEM);

    assert.ok(sHandle);

    lib.callDBusMethodAsync(
      sHandle,
      "epsitec.monolith.CresusUpdater", // target for the method call
      null, // object to call on
      null, // interface to call on
      "ExecuteCommand",
      202, // ping
      null
    );

    lib.callDBusMethodAsync(
      sHandle,
      "epsitec.monolith.CresusUpdater", // target for the method call
      null, // object to call on
      null, // interface to call on
      "ExecuteCommand",
      233, // setPortfolio
      '{"Pid": "pid.12345"}'
    );

    lib.closeDBusConnection(sHandle);
  });
});
/*
describe("CallAndListenDBusMethod", function () {
  it("should call and listen to a method through dbus", function (done) {
    const rHandle = lib.initializeDBusConnection(
      "epsitec.monolith.Test4.receiver",
      lib.dbusBusType.DBUS_BUS_SESSION,
      lib.dbusBusFlag.DBUS_NAME_FLAG_REPLACE_EXISTING
    );
    const sHandle = lib.openDBusConnection(lib.dbusBusType.DBUS_BUS_SESSION);

    assert.ok(rHandle);
    assert.ok(sHandle);

    setImmediate(() => {
      const msg = lib.listenDBusMethodCall(rHandle, null, "Method", "utf8");

      lib.closeDBusConnection(sHandle);
      lib.closeDBusConnection(rHandle);

      if (1 !== msg.msgType) {
        done(`msgType should be 1 but is ${msg.msgType}`);
      }
      if ("hello wòrld!!" !== msg.content) {
        done(`content should be 'hello wòlrd!!' but is '${msg.content}'`);
      }

      done();
    });

    lib.callDBusMethodAsync(
      sHandle,
      "epsitec.monolith.Test4.receiver", // target for the method call
      "/epsitec/monolith/Test", // object to call on
      "epsitec.monolith.Test", // interface to call on
      "Method",
      1,
      "hello wòrld!!!"
    );
  });
});
*/
