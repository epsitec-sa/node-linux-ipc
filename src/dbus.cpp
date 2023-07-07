#include <node_api.h>
#include <napi-macros.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dbus/dbus.h>

#define DBUS_MESSAGE_MAX_LENGTH 4096

struct DBusConnectionHandle
{
    DBusConnection *conn;
};

// string serverName, DBusConnectionHandle* connHandle -> int
NAPI_METHOD(InitializeDBusConnection)
{
    NAPI_ARGV(2)

    NAPI_ARGV_UTF8(serverName, 1000, 0)
    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 1)

    DBusError err;
    DBusConnection *conn;
    int ret;

    // initialise the error
    dbus_error_init(&err);

    // connect to the bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err))
    {
        printf("DBUS: Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
        NAPI_RETURN_INT32(1);
    }
    if (NULL == conn)
    {
        printf("DBUS: Connection Null\n");
        NAPI_RETURN_INT32(2);
    }

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(conn, serverName, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err))
    {
        printf("DBUS: Name Error (%s)\n", err.message);
        dbus_error_free(&err);
        NAPI_RETURN_INT32(3);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        printf("DBUS: Not Primary Owner (%d)\n", ret);
        NAPI_RETURN_INT32(4);
    }

    *pConn = conn;

    NAPI_RETURN_INT32(0)
}

/*
// MachPortHandle* machPortHandle, int msgType, const byte* content, int contentLength, int timeout -> int
NAPI_METHOD(SendMachPortMessage)
{
    NAPI_ARGV(5)

    NAPI_ARGV_BUFFER_CAST(struct MachPortHandle *, machPortHandle, 0)
    NAPI_ARGV_INT32(msgType, 1)
    NAPI_ARGV_BUFFER_CAST(const char *, content, 2)
    NAPI_ARGV_INT32(contentLength, 3)
    NAPI_ARGV_INT32(timeout, 4)

    mach_message_send message;

    if (contentLength > MACH_MESSAGE_CONTENT_LENGTH)
    {
        NAPI_RETURN_INT32(-1)
    }

    message.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    message.header.msgh_remote_port = machPortHandle->machPort;
    message.header.msgh_local_port = MACH_PORT_NULL;

    message.msgType = msgType;

    memset(message.content, 0, MACH_MESSAGE_CONTENT_LENGTH);
    strncpy(message.content, content, contentLength);

    // Send the message.
    kern_return_t kr = mach_msg(
        &message.header,                                       // Same as (mach_msg_header_t *) &message.
        MACH_SEND_MSG | (timeout > 0 ? MACH_SEND_TIMEOUT : 0), // Options. We're sending a message.
        sizeof(message),                                       // Size of the message being sent.
        0,                                                     // Size of the buffer for receiving.
        MACH_PORT_NULL,                                        // A port to receive a message on, if receiving.
        timeout > 0 ? timeout : MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL // Port for the kernel to send notifications about this message to.
    );
    if (kr != KERN_SUCCESS)
    {
        NAPI_RETURN_INT32(kr)
    }

    NAPI_RETURN_INT32(0)
}

// MachPortHandle* machPortHandle, MsgTypeHandle* msgTypeHandle, char* msgBuffer, int msgBufferLength, int timeout -> int
NAPI_METHOD(WaitMachPortMessage)
{
    NAPI_ARGV(5)

    NAPI_ARGV_BUFFER_CAST(struct MachPortHandle *, machPortHandle, 0)
    NAPI_ARGV_BUFFER_CAST(struct MsgTypeHandle *, msgTypeHandle, 1)
    NAPI_ARGV_BUFFER_CAST(char *, msgBuffer, 2)
    NAPI_ARGV_INT32(msgBufferLength, 3)
    NAPI_ARGV_INT32(timeout, 4)
    mach_message_receive message;

    if (msgBufferLength < MACH_MESSAGE_CONTENT_LENGTH)
    {
        NAPI_RETURN_INT32(-1)
    }

    kern_return_t kr = mach_msg(
        &message.header,                                     // Same as (mach_msg_header_t *) &message.
        MACH_RCV_MSG | (timeout > 0 ? MACH_RCV_TIMEOUT : 0), // Options. We're receiving a message.
        0,                                                   // Size of the message being sent, if sending.
        sizeof(message),                                     // Size of the buffer for receiving.
        machPortHandle->machPort,                            // The port to receive a message on.
        timeout > 0 ? timeout : MACH_MSG_TIMEOUT_NONE,
        MACH_PORT_NULL // Port for the kernel to send notifications about this message to.
    );
    if (kr != KERN_SUCCESS)
    {
        NAPI_RETURN_INT32(kr)
    }

    strncpy(msgBuffer, message.content, MACH_MESSAGE_CONTENT_LENGTH);
    msgTypeHandle->msgType = message.msgType;

    NAPI_RETURN_INT32(0)
}
*/

// DBusConnectionHandle* connHandle -> int
NAPI_METHOD(CloseDBusConnection)
{
    NAPI_ARGV(1)

    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 0)

    dbus_connection_flush(connHandle->conn);

    NAPI_RETURN_INT32(0)
}

NAPI_INIT()
{
    NAPI_EXPORT_FUNCTION(InitializeDBusConnection)
    // NAPI_EXPORT_FUNCTION(SendMachPortMessage)
    // NAPI_EXPORT_FUNCTION(WaitMachPortMessage)
    NAPI_EXPORT_FUNCTION(CloseDBusConnection)

    NAPI_EXPORT_SIZEOF_STRUCT(DBusConnectionHandle)
    NAPI_EXPORT_ALIGNMENTOF(DBusConnectionHandle)
}