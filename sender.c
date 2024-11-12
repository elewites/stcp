/************************************************************************
 * Adapted from a course at Boston University for use in CPSC 317 at UBC
 *
 *
 * The interfaces for the STCP sender (you get to implement them), and a
 * simple application-level routine to drive the sender.
 *
 * This routine reads the data to be transferred over the connection
 * from a file specified and invokes the STCP send functionality to
 * deliver the packets as an ordered sequence of datagrams.
 *
 * Version 2.0
 *
 *
 *************************************************************************/


#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/file.h>

#include "stcp.h"

#define STCP_SUCCESS 1
#define STCP_ERROR -1

typedef struct {


    int socket;
    int state;
    unsigned int seqNo; // current sequence number for sending packets
    unsigned int ackNo; // current ack number for receiving packets
    int timeout;        // timeout duration for retranmission
    unsigned short windowSize; // receiver's window size
    
} stcp_send_ctrl_blk;
/* ADD ANY EXTRA FUNCTIONS HERE */

/*
 * Send STCP. This routine is to send all the data (len bytes).  If more
 * than MSS bytes are to be sent, the routine breaks the data into multiple
 * packets. It will keep sending data until the send window is full or all
 * the data has been sent. At which point it reads data from the network to,
 * hopefully, get the ACKs that open the window. You will need to be careful
 * about timing your packets and dealing with the last piece of data.
 *
 * Your sender program will spend almost all of its time in either this
 * function or in tcp_close().  All input processing (you can use the
 * function readWithTimeout() defined in stcp.c to receive segments) is done
 * as a side effect of the work of this function (and stcp_close()).
 *
 * The function returns STCP_SUCCESS on success, or STCP_ERROR on error.
 */
int stcp_send(stcp_send_ctrl_blk *stcp_CB, unsigned char* data, int length) {

    /* YOUR CODE HERE */
    return STCP_SUCCESS;
}



/*
 * Open the sender side of the STCP connection. Returns the pointer to
 * a newly allocated control block containing the basic information
 * about the connection. Returns NULL if an error happened.
 *
 * If you use udp_open() it will use connect() on the UDP socket
 * then all packets then sent and received on the given file
 * descriptor go to and are received from the specified host. Reads
 * and writes are still completed in a datagram unit size, but the
 * application does not have to do the multiplexing and
 * demultiplexing. This greatly simplifies things but restricts the
 * number of "connections" to the number of file descriptors and isn't
 * very good for a pure request response protocol like DNS where there
 * is no long term relationship between the client and server.
 */
// stcp_send_ctrl_blk * stcp_open(char *destination, int sendersPort,
//                              int receiversPort) {

//     logLog("init", "Sending from port %d to <%s, %d>", sendersPort, destination, receiversPort);
//     // Step 1 open upd connection
//     int fd = udp_open(destination, receiversPort, sendersPort);
//     // Step 2 create control block
//     stcp_send_ctrl_blk *cb = malloc(sizeof(stcp_send_ctrl_blk));
//     if (!cb) {
//         logLog("failure", "Failed to allocate memory for control block");
//         return NULL;
//     }
//     // Step 3 initialize control block
//     cb->socket = fd;
//     cb->state = STCP_SENDER_SYN_SENT;
//     cb->seqNo = random() % STCP_MAXWIN;
//     cb->ackNo = 0;
//     // initialize timeout
//     cb->timeout = STCP_INITIAL_TIMEOUT;
//     // step 4 prepare and send syn packet
//     packet syn_packet;
//     createSegment(&syn_packet, SYN, STCP_MAXWIN, cb->seqNo, cb->ackNo, NULL, 0);
//     setSyn(syn_packet->hdr);
//     syn_packet.hdr->checksum = ipchecksum(&syn_packet.data, sizeof(tcpheader));
//     // send the packet
//     if (send(cb->socket, &syn_packet, syn_packet.len, 0) < 0) {
//         logLog("failure", "Failed to send SYN packet");
//         free(cb);
//         close(fd);
//         return NULL;
//     }
//     // readWithTimeout(cb->socket, &syn_packet, STCP_INITIAL_TIMEOUT);
//     return NULL;
// }
stcp_send_ctrl_blk *stcp_open(char *destination, int sendersPort, int receiversPort) {
    // Log the connection attempt
    logLog("init", "Sending from port %d to <%s, %d>", sendersPort, destination, receiversPort);

    // Step 1: Open a UDP connection
    int fd = udp_open(destination, receiversPort, sendersPort);
    if (fd < 0) {
        logLog("failure", "Failed to open UDP connection");
        return NULL;
    }

    // Step 2: Allocate and initialize control block
    stcp_send_ctrl_blk *cb = malloc(sizeof(stcp_send_ctrl_blk));
    if (!cb) {
        logLog("failure", "Failed to allocate memory for control block");
        close(fd);
        return NULL;
    }

    cb->socket = fd;
    cb->state = STCP_SENDER_SYN_SENT;
    cb->seqNo = random() % STCP_MAXWIN;  // Generate initial sequence number
    cb->ackNo = 0;
    cb->timeout = STCP_INITIAL_TIMEOUT;

    // Step 3: Prepare and send SYN packet
    packet syn_packet;
    createSegment(&syn_packet, SYN, STCP_MAXWIN, cb->seqNo, cb->ackNo, NULL, 0);
    setSyn(syn_packet.hdr); // Set SYN flag
    dump('s', syn_packet.data, syn_packet.len);
    htonHdr(syn_packet.hdr);  // Convert header to network byte order
    syn_packet.hdr->checksum = ipchecksum(syn_packet.data, syn_packet.len);
    logLog("init", "Sending SYN packet to %s:%d", destination, receiversPort);
    if (send(cb->socket, syn_packet.data, syn_packet.len, 0) < 0) {
        logLog("failure", "Failed to send SYN packet");
        free(cb);
        close(fd);
        return NULL;
    }

    // Step 4: Wait for SYN-ACK
    packet recvPacket;
    int readStatus = readWithTimeout(cb->socket, recvPacket.data, STCP_INITIAL_TIMEOUT);
    if (readStatus <= 0) {
        logLog("failure", "Timed out or failed to receive SYN-ACK");
        free(cb);
        close(fd);
        return NULL;
    }
    // Step 5: Validate SYN-ACK
    recvPacket.hdr = (tcpheader *)recvPacket.data;
    ntohHdr(recvPacket.hdr); // Convert header to host byte order
    if (!(getAck(recvPacket.hdr) && recvPacket.hdr->ackNo == cb->seqNo + 1 && getSyn(recvPacket.hdr))) {
        logLog("failure", "Invalid SYN-ACK received");
        free(cb);
        close(fd);
        return NULL;
    }
    cb->state = STCP_SENDER_ESTABLISHED;
    cb->ackNo = recvPacket.hdr->seqNo + 1;
    cb->seqNo = cb->seqNo + 1;
    logLog("init", "Connection established with %s:%d", destination, receiversPort);

    // Step 6: Send ACK to complete handshake
    packet ack_packet;
    createSegment(&ack_packet, ACK, STCP_MAXWIN, cb->seqNo, cb->ackNo, NULL, 0);
    setAck(ack_packet.hdr); // Set ACK flag
    htonHdr(ack_packet.hdr); // Convert header to network byte order
    ack_packet.hdr->checksum = ipchecksum(ack_packet.data, ack_packet.len);
    if (send(cb->socket, ack_packet.data, ack_packet.len, 0) < 0) {
        logLog("failure", "Failed to send final ACK packet");
        free(cb);
        close(fd);
        return NULL;
    }
    return cb;
}


/*
 * Make sure all the outstanding data has been transmitted and
 * acknowledged, and then initiate closing the connection. This
 * function is also responsible for freeing and closing all necessary
 * structures that were not previously freed, including the control
 * block itself.
 *
 * Returns STCP_SUCCESS on success or STCP_ERROR on error.
 */
int stcp_close(stcp_send_ctrl_blk *cb) {
    /* YOUR CODE HERE */
    return STCP_SUCCESS;
}
/*
 * Return a port number based on the uid of the caller.  This will
 * with reasonably high probability return a port number different from
 * that chosen for other uses on the undergraduate Linux systems.
 *
 * This port is used if ports are not specified on the command line.
 */
int getDefaultPort() {
    uid_t uid = getuid();
    int port = (uid % (32768 - 512) * 2) + 1024;
    assert(port >= 1024 && port <= 65535 - 1);
    return port;
}

/*
 * This application is to invoke the send-side functionality.
 */
int main(int argc, char **argv) {
    stcp_send_ctrl_blk *cb;

    char *destinationHost;
    int receiversPort, sendersPort;
    char *filename = NULL;
    int file;
    /* You might want to change the size of this buffer to test how your
     * code deals with different packet sizes.
     */
    unsigned char buffer[STCP_MSS];
    int num_read_bytes;

    logConfig("sender", "init,segment,packet,error,failure");
    /* Verify that the arguments are right */
    if (argc > 5 || argc == 1) {
        fprintf(stderr, "usage: sender DestinationIPAddress/Name receiveDataOnPort sendDataToPort filename\n");
        fprintf(stderr, "or   : sender filename\n");
        exit(1);
    }
    if (argc == 2) {
        filename = argv[1];
        argc--;
    }

    // Extract the arguments
    destinationHost = argc > 1 ? argv[1] : "localhost";
    receiversPort = argc > 2 ? atoi(argv[2]) : getDefaultPort();
    sendersPort = argc > 3 ? atoi(argv[3]) : getDefaultPort() + 1;
    if (argc > 4) filename = argv[4];

    /* Open file for transfer */
    file = open(filename, O_RDONLY);
    if (file < 0) {
        logPerror(filename);
        exit(1);
    }

    /*
     * Open connection to destination.  If stcp_open succeeds the
     * control block should be correctly initialized.
     */
    cb = stcp_open(destinationHost, sendersPort, receiversPort);
    if (cb == NULL) {
        logLog("failure", "Could not open connection to %s:%d", destinationHost, receiversPort);
        fprintf(stderr, "Could not open connection to %s:%d\n", destinationHost, receiversPort);
        close(file);
        exit(1);
    }

    /* Start to send data in file via STCP to remote receiver. Chop up
     * the file into pieces as large as max packet size and transmit
     * those pieces.
     */
    while (1) {
        num_read_bytes = read(file, buffer, sizeof(buffer));

        /* Break when EOF is reached */
        if (num_read_bytes <= 0)
            break;

        if (stcp_send(cb, buffer, num_read_bytes) == STCP_ERROR) {
            /* YOUR CODE HERE */
        }
    }

    /* Close the connection to remote receiver */
    if (stcp_close(cb) == STCP_ERROR) {
        /* YOUR CODE HERE */
    }

    return 0;
}
