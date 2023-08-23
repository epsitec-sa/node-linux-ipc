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

int GetBus(int busType, DBusConnection **conn)
{
    DBusError err;

    // initialise the error
    dbus_error_init(&err);

    // connect to the bus and check for errors
    DBusBusType bType = DBUS_BUS_SESSION;

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

    *conn = dbus_bus_get(bType, &err);
    if (dbus_error_is_set(&err))
    {
        printf("DBUS: Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    if (nullptr == conn)
    {
        printf("DBUS: Connection Null\n");
        return 2;
    }

    return 0;
}

// string serverName, int busType, int busFlags, DBusConnectionHandle* connHandle -> int
NAPI_METHOD(InitializeDBusConnection)
{
    NAPI_ARGV(4)

    NAPI_ARGV_UTF8(serverName, 1000, 0)
    NAPI_ARGV_INT32(busType, 1)
    NAPI_ARGV_INT32(busFlags, 2)
    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 3)

    DBusConnection *conn;
    DBusError err;
    int ret;

    ret = GetBus(busType, &conn);
    if (ret != 0)
    {
        NAPI_RETURN_INT32(ret)
    }

    // initialise the error
    dbus_error_init(&err);

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(conn, serverName, busFlags, &err);
    if (dbus_error_is_set(&err))
    {
        printf("DBUS: Name Error (%s)\n", err.message);
        dbus_error_free(&err);
        NAPI_RETURN_INT32(3)
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        printf("DBUS: Not Primary Owner (%d)\n", ret);
        NAPI_RETURN_INT32(4)
    }

    connHandle->conn = conn;

    NAPI_RETURN_INT32(0)
}

// int busType, DBusConnectionHandle* connHandle -> int
NAPI_METHOD(OpenDBusConnection)
{
    NAPI_ARGV(2)

    NAPI_ARGV_INT32(busType, 0)
    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 1)

    DBusConnection *conn;

    int ret = GetBus(busType, &conn);
    if (ret != 0)
    {
        NAPI_RETURN_INT32(ret)
    }

    connHandle->conn = conn;

    NAPI_RETURN_INT32(0)
}

// DBusConnectionHandle* connHandle, char *targetName, char *objectName, char *interfaceName, char *methodName, int cmdType, const byte* content, int contentLength -> int
NAPI_METHOD(EnqueueDBusMethodCall)
{
    NAPI_ARGV(8)

    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 0)
    NAPI_ARGV_UTF8(targetName, 1000, 1)
    NAPI_ARGV_UTF8(objectName, 1000, 2)
    NAPI_ARGV_UTF8(interfaceName, 1000, 3)
    NAPI_ARGV_UTF8(methodName, 1000, 4)
    NAPI_ARGV_INT32(cmdType, 5)
    NAPI_ARGV_BUFFER_CAST(const char *, content, 6)
    NAPI_ARGV_INT32(contentLength, 7)

    DBusMessage *msg;
    DBusMessageIter args;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(targetName,    // target for the method call
                                       objectName,    // object to call on
                                       interfaceName, // interface to call on
                                       methodName);   // method name
    if (nullptr == msg)
    {
        printf("DBUS: Message Null\n");
        NAPI_RETURN_INT32(1)
    }

    dbus_message_set_auto_start(msg, TRUE);

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &cmdType))
    {
        printf("DBUS: cannot set first argument!\n");
        dbus_message_unref(msg);
        NAPI_RETURN_INT32(2)
    }

    if (contentLength > 0)
    {
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &content))
        {
            printf("DBUS: cannot set second argument!\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(3)
        }
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send(connHandle->conn, msg, nullptr))
    {
        printf("DBUS: Out of memory!\n");
        dbus_message_unref(msg);
        NAPI_RETURN_INT32(4)
    }

    dbus_connection_flush(connHandle->conn);

    dbus_message_unref(msg);
    NAPI_RETURN_INT32(0)
}

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
    int *cmdType = nullptr;

    if (argsBufferLength < DBUS_MESSAGE_MAX_LENGTH)
    {
        printf("the receive buffer length if less than %d\n", DBUS_MESSAGE_MAX_LENGTH);
        NAPI_RETURN_INT32(1)
    }

    while (true)
    {
        // non blocking read of the next available message
        if (!dbus_connection_read_write(connHandle->conn, 0))
        {
            // not connected anymore
            NAPI_RETURN_INT32(2)
        }

        msg = dbus_connection_pop_message(connHandle->conn);
        // loop again if we haven't got a message
        if (nullptr == msg)
        {
            usleep(200);
            continue;
        }

        if (interfaceName == nullptr)
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
            NAPI_RETURN_INT32(3)
        }

        // first argument
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32)
        {
            printf("type of first argument is wrong (not int)\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(4)
        }
        dbus_message_iter_get_basic(&args, cmdType);

        const char *param = "";
        if (dbus_message_iter_next(&args))
        {
            // second argument is available
            if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
            {
                printf("type of second argument is wrong (not string)\n");
                dbus_message_unref(msg);
                NAPI_RETURN_INT32(5)
            }
            dbus_message_iter_get_basic(&args, &param);
        }

        int paramLen = strlen(param);
        if (paramLen > argsBufferLength)
        {
            printf("second argument is longer than available buffer\n");
            dbus_message_unref(msg);
            NAPI_RETURN_INT32(6)
        }

        strncpy(argsBuffer, param, argsBufferLength);
        cmdTypeHandle->cmdType = *cmdType;

        dbus_message_unref(msg);
        NAPI_RETURN_INT32(0)
    }

    NAPI_RETURN_INT32(1000) // should never happen
}

// DBusConnectionHandle* connHandle -> int
NAPI_METHOD(CloseDBusConnection)
{
    NAPI_ARGV(1)

    NAPI_ARGV_BUFFER_CAST(struct DBusConnectionHandle *, connHandle, 0)

    dbus_connection_flush(connHandle->conn);
    dbus_connection_unref(connHandle->conn);

    NAPI_RETURN_INT32(0)
}

NAPI_INIT()
{
    NAPI_EXPORT_FUNCTION(InitializeDBusConnection)
    NAPI_EXPORT_FUNCTION(OpenDBusConnection)
    NAPI_EXPORT_FUNCTION(EnqueueDBusMethodCall)
    NAPI_EXPORT_FUNCTION(ListenDBusMethodCall)
    NAPI_EXPORT_FUNCTION(CloseDBusConnection)

    NAPI_EXPORT_SIZEOF_STRUCT(DBusConnectionHandle)
    NAPI_EXPORT_ALIGNMENTOF(DBusConnectionHandle)
    NAPI_EXPORT_SIZEOF_STRUCT(CmdTypeHandle)
    NAPI_EXPORT_ALIGNMENTOF(CmdTypeHandle)
}