# Subscription-System

I implemented a subscription system using TCP-UDP. The TCP clients can log in and subscribe to various topics which
are provided by the UDP clients. Each time something new comes up from a topic, clients will receive the information. If they are
offline, the information will be stocked and sent to them when they come back online. A user also has the posibility
to unsubscribe from a topic. Two clients with the same username can't be connected at the same time.

A TCP client can do the following:
1. connect to the server: ./subscriber <id> <server_ip> <port>
2. subscribe to a topic: subscribe <topic> <sf> (sf = 1 => the information received while the client is offline will be
stocked and sent to him when online, sf = 0 => no information will be stocked if offline)
3. unsubscribe from a topic: unsubscribe <topic>
4. exit (logout)
