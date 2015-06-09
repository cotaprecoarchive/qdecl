#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

int ensure_exchange_requirements(int argc, char *argv[], int args)
{
    if (args == 1) {
        printf(
            "You must specify exchange type: direct, topic, fanout or headers for exchange `%s`\n",
            argv[argc - args]
        );

        return 1;
    }

    if (args == 0) {
        printf("You must specify exchange name and type\n");

        return 1;
    }

    if (args == 2) {
        char *type = argv[argc - args + 1];

        int invalid_ex_type = (
            strcmp(type, "direct")  != 0 &&
            strcmp(type, "topic")   != 0 &&
            strcmp(type, "fanout")  != 0 &&
            strcmp(type, "headers") != 0
        );

        if (invalid_ex_type) {
            printf(
                "%s%s%s. Got `%s`\n",
                "You've provided an invalid exchange type, only `direct`, ",
                "`topic`, `fanout`, `headers` ",
                "are acceptable",
                type
            );

            return 1;
        }
    }

    return 0;
}

int get_reply_type(amqp_connection_state_t connection)
{
    amqp_rpc_reply_t reply;

    reply = amqp_get_rpc_reply(connection);

    return reply.reply_type;
}

int main (int argc, char *argv[])
{
    static int
        durable,
        internal,
        auto_delete,
        declare_exchange,
        declare_queue;

    enum {
        ARG_AMQP_HOST     = 255,
        ARG_AMQP_PORT     = 256,
        ARG_AMQP_USER     = 257,
        ARG_AMQP_PASSWORD = 258
    };

    char const
        *amqp_host,
        *amqp_user,
        *amqp_password;

    int amqp_port;

    for (;;) {
        int c, opt_index = 0;

        static struct option long_opts[] = {
            {"help",        no_argument,       0,                 'h'},
            {"version",     no_argument,       0,                 'v'},
            {"durable",     no_argument,       &durable,          'd'},
            {"internal",    no_argument,       &internal,         'i'},
            {"auto-delete", no_argument,       &auto_delete,      'a'},
            {"exchange",    no_argument,       &declare_exchange, 'e'},
            {"queue",       no_argument,       &declare_queue,    'q'},
            {"host",        required_argument, 0,                 ARG_AMQP_HOST},
            {"port",        required_argument, 0,                 ARG_AMQP_PORT},
            {"user",        required_argument, 0,                 ARG_AMQP_USER},
            {"password",    required_argument, 0,                 ARG_AMQP_PASSWORD},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, ":vdiaeqh", long_opts, &opt_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                printf("Usage!\n");
                return 0;

            case 'v':
                printf("Version %s, build %s\n", VERSION, GIT_COMMIT);
                break;

            case ARG_AMQP_HOST:
                amqp_host = optarg;
                break;

            case ARG_AMQP_PORT:
                amqp_port = atoi(optarg);
                break;

            case ARG_AMQP_USER:
                amqp_user = optarg;
                break;

            case ARG_AMQP_PASSWORD:
                amqp_password = optarg;
                break;

            case '?':
            default:
                break;
        }
    }

    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t amqp_connection;

    amqp_connection = amqp_new_connection();
    socket = amqp_tcp_socket_new(amqp_connection);
    status = amqp_socket_open(socket, amqp_host, amqp_port);

    // @link http://alanxz.github.io/rabbitmq-c/docs/0.4.0/amqp_8h.html#a05dadc32b3a59918206ac38a53606723
    if (status == AMQP_STATUS_SOCKET_ERROR) {
        printf("Unable to connect :(\n");
        return 1;
    }

    amqp_rpc_reply_t login_reply;

    login_reply = amqp_login(
        amqp_connection,
        "/",
        0,      /* channel_max */
        131072, /* frame_max */
        0,      /* heartbeat */
        AMQP_SASL_METHOD_PLAIN,
        amqp_user,
        amqp_password
    );

    // @link http://alanxz.github.io/rabbitmq-c/docs/0.2/amqp_8h.html#ace098ed2a6aacbffd96cf9f7cd9f6465
    if (login_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("Unable to authenticate :(\n");
    }

    amqp_channel_open(amqp_connection, 1);

    if (get_reply_type(amqp_connection) != AMQP_RESPONSE_NORMAL) {
        printf("Unable to open channel :(\n");
    }

    if (! (declare_queue || declare_exchange)) {
        printf("You must choose declare a queue or an exchange!\n");
    }

    int args = (argc - optind);

    if (declare_exchange && ensure_exchange_requirements(argc, argv, args) != 0) {
        return 1;
    }

    if (declare_exchange) {
        char *exchange      = argv[argc - args + 0];
        char *exchange_type = argv[argc - args + 1];

        // @link https://github.com/alanxz/rabbitmq-c/blob/9626dd5cd5f78894f1416a1afd2d624ddd4904ae/librabbitmq/amqp_framing.h#L785-L793
        amqp_exchange_declare(
            amqp_connection,
            1, /* channel */
            amqp_cstring_bytes(exchange),
            amqp_cstring_bytes(exchange_type),
            0 /* passive */,
            durable,
            auto_delete,
            internal,
            amqp_empty_table
        );

        return get_reply_type(amqp_connection) == AMQP_RESPONSE_NORMAL
            ? 0
            : 1;
    }

    return 0;
}
