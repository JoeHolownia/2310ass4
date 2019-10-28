#include "network.h"
#include "linkedLists.h"
#include "channel.h"
#include "messaging.h"

#define MAX_BUFFER_LENGTH 50
#define DELIVER 0
#define WITHDRAW 1
#define BLANK_DEFER_LENGTH 7
#define MAX_CONNECTIONS 30

/**
 * Thread function for waiting on an execution key to instantiate a
 * deferred message. Once executed, passed the deferred message to
 * the corresponding message handler.
 *
 * @param arg: connection wrapper containing information for the key,
 *      executed status, deferred message and info for all other required
 *      subsequent message handlers
 * @return null unused value (required for thread function)
 */
void* defer_thread(void* arg) {

    struct ConnectionWrapper* connection = (struct ConnectionWrapper*)arg;

    pthread_mutex_lock(connection->dataLock);
    struct LinkedList* deferral =
            search_list_by_name("new", connection->firstDeferral);

    if (deferral == NULL) {
        return NULL;
    }

    deferral->name = "ready";
    pthread_mutex_unlock(connection->dataLock);

    while (!deferral->type.deferral.executed) {
        // wait for external boolean signal to break the loop
    }

    char* operation = deferral->type.deferral.operation;
    char firstLetter = operation[0];

    // call operation
    switch (firstLetter) {
        case 'D':
            handle_deliver_withdraw_message(operation,
                    connection->firstResource, connection->dataLock,
                    DELIVER);
            break;

        case 'W':
            handle_deliver_withdraw_message(operation,
                    connection->firstResource, connection->dataLock,
                    WITHDRAW);
            break;

        case 'T':
            handle_transfer_message(operation,
                    connection->firstResource,
                    connection->thisDepot, connection->dataLock);
            break;

        default:
            return NULL;
    }

    return NULL;
}

/**
 * Message handler for defer message, of the format Defer:k:operation,
 * where k is the key assigned to this deferred oeperation, and operation
 * is a sub-message which is a command to be performed upon execution
 * of this deferral (i.e. Deliver:q:t). First checks message, then
 * locates the operation sub-message, creates a new deferral struct
 * (added to the linked list of this depot) and starts a thread. This thread
 * then waits for the execution of the given key, as given in defer_thread().
 *
 * @param message: the defer message to handle
 * @param connection: a connection wrapper struct, containing all depot info
 *      including existing deferrals, resources and connected depots.
 */
void handle_defer_message(char* message,
        struct ConnectionWrapper* connection) {

    // ignore faulty message
    if (!check_defer_message(message)) {
        return;
    }

    // breakdown defer message to find position of 'operation'
    // mesage (i.e. Deliver...)
    char* messageCopy = malloc(sizeof(char) * strlen(message));
    strcpy(messageCopy, message);
    char* key;
    int splitPosition = BLANK_DEFER_LENGTH; // i.e. Defer::

    strtok_r(messageCopy, ":", &messageCopy);
    key = strtok_r(messageCopy, ":", &messageCopy);
    splitPosition += strlen(key);
    message += splitPosition;

    // create new deferral
    pthread_mutex_lock(connection->dataLock);
    struct LinkedList* newDeferral = add_item(connection->firstDeferral);
    newDeferral->type.deferral.executed = false;
    newDeferral->type.deferral.key = atoi(key);
    newDeferral->name = "new";
    newDeferral->type.deferral.operation = message;

    pthread_mutex_unlock(connection->dataLock);

    // create defer thread
    pthread_t tid;
    pthread_create(&tid, 0, defer_thread, connection);
}

/**
 * Mesage handler for IM message, of the format IM:port:name,
 * where port is the port of the connecting depot, and name is
 * the name of the connecting depot. Adds a record of the
 * connecting depot (with port and name) in this depot's list
 * of all depots, if the IM message is received correctly. If
 * the IM message is not the first thing sent, or is incorrect, when
 * two depots connect - the connection thread is removed and
 * communication between the depots ceases.
 *
 * @param message: the IM message to handle
 * @param connection: a connectionw wrapper containing all information
 *      for this connection between depots.
 * @return a boolean status to determine the validity of the new connection.
 *      If the IM message is valid, return true and handle the connection
 *      normally, however if it is invalid in anyway, cancel the thread
 *      waiting on new input from this depot, effectively ceasing
 *      communications.
 */
bool handle_im_message(char* message,
        struct ConnectionWrapper* connection) {

    // if message is faulty, we close connection
    if(!check_im_message(message)) {
        return false;
    }

    char* port;
    char* name;
    strtok_r(message, ":", &message);
    port = strtok_r(message, ":", &message);
    name = strtok_r(message, ":", &message);

    pthread_mutex_lock(connection->dataLock);

    char* new = "new";
    struct LinkedList* newDepot =
            search_list_by_name(new, connection->thisDepot);

    if (newDepot != NULL) {
        newDepot->name = name;
        newDepot->type.depot.port = port;
    }

    pthread_mutex_unlock(connection->dataLock);

    return true;
}

/**
 * Message handler for connect message, of the format Connect:port,
 * where port is the port number to try and connect to. Facilitates
 * connection to a new port, given by its port number, unless the port
 * has already been connected to or closed.
 *
 * @param message: the connect message to handle
 * @param connection: a connection wrapper containing information, with
 *      which to create the new connection
 */
void handle_connect_message(char* message,
        struct ConnectionWrapper* connection) {

    // silently ignore faulty connect message
    if(!check_connect_message(message)) {
        return;
    }

    char* port;
    strtok_r(message, ":", &message);
    port = strtok_r(message, ":", &message);

    // check for duplicate port nums (if it is already connected)...
    struct LinkedList* node = connection->thisDepot;
    while (node != NULL) { // process all nodes

        if (node->type.depot.port == NULL) {
            continue;
        }

        if (strcmp(node->type.depot.port, port) == 0) {
            return;
        }
        node = node->next;
    }

    connect_to_depot(port, connection);
}

/**
 * Message handler for all received messages from the channel.
 * Detects first letter/part of new received message on a channel, and
 * decides how to deal with it by calling sub-handler functions.
 *
 * @param message: the message to handle
 * @param connection: wrapper struct containing information about this
 *      connection
 */
void handle_messages(char* message, struct ConnectionWrapper* connection) {

    char firstLetter = message[0];
    char* firstThreeLetters;

    switch (firstLetter) {
        case 'C':
            // Connect
            handle_connect_message(message, connection);
            break;

        case 'D':
            // Deliver or defer
            firstThreeLetters = malloc(sizeof(char) * 3);
            strncpy(firstThreeLetters, message, 3);

            if (strcmp(firstThreeLetters, "Del") == 0) {
                handle_deliver_withdraw_message(message,
                        connection->firstResource, connection->dataLock,
                        DELIVER);
            } else {
                handle_defer_message(message, connection);
            }
            free(firstThreeLetters);
            break;

        case 'W':
            // Withdraw
            handle_deliver_withdraw_message(message,
                    connection->firstResource, connection->dataLock,
                    WITHDRAW);
            break;

        case 'T':
            // Transfer
            handle_transfer_message(message,
                    connection->firstResource,
                    connection->thisDepot, connection->dataLock);
            break;

        case 'E':
            // Execute
            handle_execute_message(message, connection->firstDeferral,
                    connection->dataLock);
            break;

        default:
            return;
    }
}

/**
 * Thread function for reading side of each connection, reads from connection
 * FILE* and places input into a threadsafe channel. There is one reader
 * thread per connection between depots. A corresponding action thread will
 * take input from the channel and perform actions upon it.
 *
 * @param arg: connection wrapper struct containing all info relevant to a
 *      single connection
 * @return NULL (just for thread function requirement)
 */
void* reader_thread(void* arg) {

    struct ConnectionWrapper* connection = (struct ConnectionWrapper*)arg;
    char buffer[MAX_BUFFER_LENGTH];
    char* string;

    while (1) {
        // break on EOF
        while ((string = fgets(buffer, MAX_BUFFER_LENGTH,
                connection->from))) {
            if (string[strlen(string) - 1] == '\n') {
                string[strlen(string) - 1] = '\0';
            }

            // write to queue
            write_channel(connection->channel, (void*) strdup(string));
        }
    }

    return NULL;
}

/**
 * Thread function which reads from a threadsafe channel (by which data
 * is input through the reader_thread using the current connection FILE*s).
 * Messages are read from the channel, handled, and sent to a message handler
 * to determine how to treat it, and to perform actions.
 *
 * @param arg: connection wrapper struct containing all info relevant to a
 *      single connection
 * @return NULL (just for thread function requirement)
 */
void* action_thread(void* arg) {

    struct ConnectionWrapper* connection = (struct ConnectionWrapper*)arg;
    char* string;

    bool expectedFirst = true;
    bool connectionOpen = true;

    // wait to check IM message before handling anything else
    while (expectedFirst) {
        if (read_channel(connection->channel, (void**) &string)) {

            if (!handle_im_message(string, connection)) {
                connectionOpen = false;
            }
            expectedFirst = false;
        }
    }

    // if IM message not received, connection never opens
    while (connectionOpen) {
        if (read_channel(connection->channel, (void**) &string)) {

            handle_messages(string, connection);
        }
    }

    return NULL;
}

/**
 * Called by the listening connection_thread, and when connect_to_depot
 * is called, sets up essential information for a new connection threads
 * to be created (in the connection wrapper struct), starts a reader_thread
 * and a writer_thread, then sends and IM connect message to the depot
 * at the other end of the connection.
 *
 * @param connection: the connection wrapper struct containing all info
 *      to set up new reader/action threads
 * @param to: a FILE* for sending messages to the other connected depot
 * @param from: a FILE* for receiving messages from the other connected depot
 */
void start_communication_threads(struct ConnectionWrapper* connection,
        int to, int from) {

    // create depot object and assign streams
    pthread_mutex_lock(connection->dataLock);

    struct LinkedList* newDepot = add_item(connection->thisDepot);
    newDepot->type.depot.to = fdopen(to, "w");
    newDepot->type.depot.from = fdopen(from, "r");
    newDepot->name = "new";
    connection->connectedDepot = newDepot;

    struct Channel* channel = new_channel(); // set up new channel
    connection->channel = channel;

    connection->to = newDepot->type.depot.to;
    connection->from = newDepot->type.depot.from;

    pthread_mutex_unlock(connection->dataLock);

    // start threads (read and action)
    pthread_create(&newDepot->type.depot.readerId, 0, reader_thread,
            connection); // reader thread
    pthread_create(&newDepot->type.depot.writerId, 0, action_thread,
            connection); // action thread

    // send IM connect message to new connected depot
    fprintf(connection->connectedDepot->type.depot.to, "IM:%s:%s\n",
            connection->thisDepot->type.depot.port,
            connection->thisDepot->name);
    fflush(connection->connectedDepot->type.depot.to);
}

/**
 * Thread function which acts as a server by listening for new connections.
 * Once a connection is received, a new connection wrapper is created for it
 * and then passed to start_communication_threads to start threads for the
 * new connection.
 *
 * @param arg: a blank wrapper for a connection struct, to be overridden
 *      when a new connection is accepted
 * @return NULL (for thread function definition)
 */
void* connection_thread(void* arg) {

    struct ConnectionWrapper* wrapper = (struct ConnectionWrapper*)arg;
    struct ConnectionWrapper* connection;
    int server = wrapper->serverSocket;
    int connFd;
    int connFd2;

    while (1) {
        // accept connection request
        connFd = accept(server, 0, 0);
        connFd2 = dup(connFd);

        // create unique connection wrapper for each new connection
        connection = malloc(sizeof(struct ConnectionWrapper));
        connection->thisDepot = wrapper->thisDepot;
        connection->firstDeferral = wrapper->firstDeferral;
        connection->dataLock = wrapper->dataLock;
        connection->firstResource = wrapper->firstResource;

        // start threads for communication between depots
        start_communication_threads(connection, connFd, connFd2);
    }
    free(connection);
    return NULL;
}

/**
 * Creates a new connection wrapper which contains all relevant information
 * for an individual connection. This is to be passed to threads.
 *
 * @param thisDepot: this depot, in a list of all connected depots
 * @param firstResource: first resource in this depots resource list
 * @param firstDeferral:  first deferral message, for linked list of potential
 *      deferred message operations
 * @param dataLock: mutex protecting this depots structs and lists
 * @return a pointer to the newly created connection wrapper, to be passed to
 *      threads
 */
struct ConnectionWrapper* new_connection_wrapper(struct LinkedList* thisDepot,
        struct LinkedList* firstResource, struct LinkedList* firstDeferral,
        pthread_mutex_t* dataLock) {

    struct ConnectionWrapper* connection =
            malloc(sizeof(struct ConnectionWrapper));

    connection->thisDepot = thisDepot;
    connection->firstResource = firstResource;
    connection->firstDeferral = firstDeferral;
    connection->dataLock = dataLock;

    return connection;
}

/**
 * Starts the server for this specific depot, prints the port number,
 * and creates a thread to handle incoming connection requests from
 * other depots.
 *
 * @param thisDepot: this depot, in a list of all connected depots
 * @param firstResource: first resource in this depots resource list
 * @param firstDeferral:  first deferral message, for linked list of potential
 *      deferred message operations
 * @param dataLock: mutex protecting this depots structs and lists
 * @return the thread id of the server (or -1 if an error occurred)
 */
pthread_t start_server(struct LinkedList* thisDepot,
        struct LinkedList* firstResource, struct LinkedList* firstDeferral,
        pthread_mutex_t* dataLock) {

    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv6  for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // Because we want to bind with it

    int err; // start server on any ephemeral port
    if ((err = getaddrinfo("localhost", 0, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return -1;  // could not work out the address
    }

    int server = socket(AF_INET, SOCK_STREAM, 0); // socket - default protocol

    // bind server socket to port
    if (bind(server, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
        return -1;
    }

    struct sockaddr_in ad; // Which port did we get?
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(server, (struct sockaddr*)&ad, &len)) {
        return -1;
    }

    // assign port number to this depot and print
    unsigned int port = ntohs(ad.sin_port);
    printf("%u\n", port);
    fflush(stdout);

    char portBuffer[6];
    snprintf(portBuffer, 6, "%u", port);
    thisDepot->type.depot.port = portBuffer;

    if (listen(server, MAX_CONNECTIONS)) { // listen for connections
        return -1;
    }

    // handle connection requests with a thread
    struct ConnectionWrapper* connection = new_connection_wrapper(thisDepot,
            firstResource, firstDeferral, dataLock);
    connection->serverSocket = server;
    pthread_t tid;
    pthread_create(&tid, 0, connection_thread, connection);
    return tid;
}

/**
 * Connects to another depot given by the specified port number.
 * @param port: the port number to connect to
 * @param wrapper: the connection wrapper containing information to start
 *      a new connection
 * @return integer error status (0 if none detected)
 */
int connect_to_depot(const char* port,
        struct ConnectionWrapper* wrapper) {

    struct ConnectionWrapper* connection;
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;  // IPv6  for generic could use AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;

    if ((getaddrinfo("localhost", port, &hints, &ai))) {
        freeaddrinfo(ai);
        return 1;   // could not work out the address
    }

    int depotFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(depotFd, (struct sockaddr*)ai->ai_addr,
            sizeof(struct sockaddr))) {
        return 3;
    }
    int depotFd2 = dup(depotFd);

    // create new connection struct
    connection = malloc(sizeof(struct ConnectionWrapper));
    connection->thisDepot = wrapper->thisDepot;
    connection->firstDeferral = wrapper->firstDeferral;
    connection->dataLock = wrapper->dataLock;
    connection->firstResource = wrapper->firstResource;

    start_communication_threads(connection, depotFd, depotFd2);

    return 0;
}