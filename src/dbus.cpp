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

struct CmdTypeHandle
{
    int cmdType;
};

// string serverName, int busType, int busFlags, DBusConnectionHandle* connHandle -> int
NAPI_METHOD(InitializeDBusConnection)
{
    NAPI_ARGV(4)

    NAPI_ARGV_UTF8(serverName, 1000, 0)
    NAPI_ARGV_INT32(busType, 1)
    NAPI_ARGV_INT32(busFlags, 2)
    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 3)

    DBusError err;
    DBusConnection *conn;
    int ret;

    // initialise the error
    dbus_error_init(&err);

    // connect to the bus and check for errors
    DBusBusType bType = DBUS_BUS_SYSTEM;

    if (busType == 0) // session
    {
        bType = DBUS_BUS_SESSION;
    }
    else if (busType == 1) // system
    {
        bType = DBUS_BUS_SYSTEM;
    }
    else if (busType == 2) // starter
    {
        bType = DBUS_BUS_STARTER;
    }

    conn = dbus_bus_get(bType, &err);
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
    ret = dbus_bus_request_name(conn, serverName, busFlags, &err);
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

    connHandle->conn = conn;

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
}*/

// DBusConnectionHandle* connHandle, char *interfaceName, char *methodName, CmdTypeHandle *cmdTypeHandle, char *argsBuffer, int argsBufferLength -> int
NAPI_METHOD(ListenDBusMethodCall)
{
    NAPI_ARGV(6)

    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 0)
    NAPI_ARGV_UTF8(interfaceName, 1000, 1)
    NAPI_ARGV_UTF8(methodName, 1000, 2)
    NAPI_ARGV_BUFFER_CAST(struct CmdTypeHandle *, cmdTypeHandle, 3)
    NAPI_ARGV_BUFFER_CAST(char *, argsBuffer, 4)
    NAPI_ARGV_INT32(argsBufferLength, 5)
    DBusMessage *msg;
    DBusMessageIter args;
    int *cmdType;

    if (argsBufferLength < DBUS_MESSAGE_MAX_LENGTH)
    {
        printf("the receive buffer length if less than %d\n", DBUS_MESSAGE_MAX_LENGTH);
        NAPI_RETURN_INT32(1);
    }

    while (true)
    {
        // non blocking read of the next available message
        if (!dbus_connection_read_write(connHandle->conn, 0))
        {
            // not connected anymore
            NAPI_RETURN_INT32(2);
        }
        msg = dbus_connection_pop_message(connHandle->conn);
        // loop again if we haven't got a message
        if (NULL == msg)
        {
            usleep(200);
            continue;
        }

        if (interfaceName == NULL)
        {
            // only check method
            char *method = (char *)dbus_message_get_member(msg);
            if (strcmp(method, methodName) != 0)
            {
                // free the message
                dbus_message_unref(msg);
                continue;
            }
        }
        else
        {
            // check interface and method
            if (!dbus_message_is_method_call(msg, interfaceName, methodName))
            {
                // free the message
                dbus_message_unref(msg);
                continue;
            }
        }

        // read the arguments
        if (!dbus_message_iter_init(msg, &args))
        {
            // No arguments
            printf("no arguments are provided\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(3);
        }

        // first argument
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32)
        {
            printf("type of first argument is wrong (not int)\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(4);
        }
        dbus_message_iter_get_basic(&args, cmdType);

        char *param = "";
        if (dbus_message_iter_next(&args))
        {
            // second argument is available
            if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
            {
                printf("type of second argument is wrong (not string)\n");
                dbus_message_unref(msg);
                NAPI_RETURN_INT32(5);
            }
            dbus_message_iter_get_basic(&args, &param);
        }

        int paramLen = strlen(param);
        if (paramLen > argsBufferLength)
        {
            printf("second argument is longer than available buffer\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(6);
        }

        strncpy(argsBuffer, param, paramLen);
        cmdTypeHandle->cmdType = *cmdType;

        dbus_message_unref(msg);
        NAPI_RETURN_INT32(0);
    }

    NAPI_RETURN_INT32(1000); // should never happen
}

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
    NAPI_EXPORT_FUNCTION(ListenDBusMethodCall)
    NAPI_EXPORT_FUNCTION(CloseDBusConnection)

    NAPI_EXPORT_SIZEOF_STRUCT(DBusConnectionHandle)
    NAPI_EXPORT_ALIGNMENTOF(DBusConnectionHandle)
    NAPI_EXPORT_SIZEOF_STRUCT(CmdTypeHandle)
    NAPI_EXPORT_ALIGNMENTOF(CmdTypeHandle)
}