### `qdecl`

A simple utility tool that we use to declare **queues** and **exchanges** in [RabbitMQ](https://www.rabbitmq.com/).

**Why?**
We want to declare queue and exchanges at deploy time, instead of waiting for application do it. As our consumers runs in background and sometimes exchange isn't declared yet, well, this approach lets us get rid of the fucking error when you attempt to bind to an inexistent exchange.

Why not use [rabbitmq_management](https://www.rabbitmq.com/management.html)? Simple: we don't have it enabled.
