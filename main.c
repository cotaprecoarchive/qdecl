/*
 * Copyright (c) 2015 Cota Preço
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @author Andrey K. Vital <andreykvital@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

/**
 * @param  argc
 * @param  argv
 * @param  args
 * @return int (1 if requirements wasn't fulfilled 0 otherwise)
 */
int ensure_exchange_requirements(int argc, char *argv[], int args)
{
    if (args == 0) {
        printf("You must specify exchange name and type\n");

        return 1;
    }

    if (args == 1) {
        printf(
            "You must specify exchange type: direct, topic, fanout or headers for exchange `%s`\n",
            argv[argc - args]
        );

        return 1;
    }

    char *exchange_name = argv[argc - args + 0];
    char *exchange_type = argv[argc - args + 1];

    int invalid_ex_type = (
        strcmp(exchange_type, "direct")  != 0 &&
        strcmp(exchange_type, "topic")   != 0 &&
        strcmp(exchange_type, "fanout")  != 0 &&
        strcmp(exchange_type, "headers") != 0
    );

    if (invalid_ex_type) {
        printf(
            "%s%s%s. Got `%s`\n",
            "You've provided an invalid exchange type, only `direct`, ",
            "`topic`, `fanout`, `headers` ",
            "are acceptable",
            exchange_type
        );

        return 1;
    }

    return 0;
}

/**
 * @param connection
 * @param channel
 */
void close_connection_and_channel(amqp_connection_state_t connection, int channel)
{
    amqp_channel_close(connection, channel, AMQP_REPLY_SUCCESS);

    amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
}

/**
 * @param  connection
 * @return int (AMQP_RESPONSE_NORMAL|AMQP_RESPONSE_SERVER_EXCEPTION|AMQP_RESPONSE_LIBRARY_EXCEPTION)
 */
int get_reply_type(amqp_connection_state_t connection)
{
    amqp_rpc_reply_t reply;

    reply = amqp_get_rpc_reply(connection);

    return reply.reply_type;
}

int main(int argc, char *argv[])
{
    static int
        durable,
        internal,
        auto_delete,
        exclusive,
        declare_exchange,
        declare_queue;

    enum {
        ARG_AMQP_HOST = 255,
        ARG_AMQP_PORT,
        ARG_AMQP_USER,
        ARG_AMQP_PASSWORD
    };

    char const
        *amqp_host     = "localhost",
        *amqp_user     = "guest",
        *amqp_password = "guest";

    int amqp_port = 5672;

    for (;;) {
        int c, opt_index = 0;

        static struct option long_opts[] = {
            {"help",        no_argument,       0,                 'h'},
            {"version",     no_argument,       0,                 'v'},
            {"durable",     no_argument,       &durable,          'd'},
            {"internal",    no_argument,       &internal,         'i'},
            {"auto-delete", no_argument,       &auto_delete,      'a'},
            {"exclusive",   no_argument,       &exclusive,        'x'},
            {"exchange",    no_argument,       &declare_exchange, 'e'},
            {"queue",       no_argument,       &declare_queue,    'q'},
            {"host",        required_argument, 0,                 ARG_AMQP_HOST},
            {"port",        required_argument, 0,                 ARG_AMQP_PORT},
            {"user",        required_argument, 0,                 ARG_AMQP_USER},
            {"password",    required_argument, 0,                 ARG_AMQP_PASSWORD},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, ":hvdiaxeq", long_opts, &opt_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                // TODO: display available [OPTIONS]!
                printf("Usage: qdecl [...OPTIONS] [exchange-name exchange-type | queue-name]\n");
                return 0;

            case 'v':
                printf("Version %s, build %s\n", VERSION, GIT_COMMIT);
                return 0;

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

    if (! (declare_queue || declare_exchange)) {
        printf(
            "%s. %s\n",
            "You must choose declare a queue or an exchange",
            "See `qdecl -h` for more information!"
        );
    }

    // printf("%s %s %s %d\n", amqp_host, amqp_user, amqp_password, amqp_port);

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

        return 1;
    }

    amqp_channel_open(amqp_connection, 1);

    if (get_reply_type(amqp_connection) != AMQP_RESPONSE_NORMAL) {
        printf("Unable to open channel :(\n");

        return 1;
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
            0, /* passive */
            durable,
            auto_delete,
            internal,
            amqp_empty_table
        );

        int result = get_reply_type(amqp_connection) == AMQP_RESPONSE_NORMAL ? 0 : 1;

        close_connection_and_channel(amqp_connection, 1);

        return result;
    }

    if (! declare_queue) {
        return 0;
    }

    char *queue_name = argv[argc - args + 0];

    if (! queue_name) {
        printf("You haven't provided the queue name to declare\n");

        return 1;
    }

    // @link https://github.com/alanxz/rabbitmq-c/blob/9626dd5cd5f78894f1416a1afd2d624ddd4904ae/librabbitmq/amqp_framing.h#L842-L850
    amqp_queue_declare(
        amqp_connection,
        1, /* channel */
        amqp_cstring_bytes(queue_name),
        0, /* passive */
        durable,
        0, /* exclusive */
        auto_delete,
        amqp_empty_table
    );

    int result = get_reply_type(amqp_connection) == AMQP_RESPONSE_NORMAL ? 0 : 1;

    close_connection_and_channel(amqp_connection, 1);

    return result;
}
