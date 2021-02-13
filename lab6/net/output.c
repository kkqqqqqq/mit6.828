#include "ns.h"
#include <inc/lib.h>
extern union Nsipc nsipcbuf;


void
output(envid_t ns_envid)
{  
        binaryname = "ns_output";
        int r;          
        int perm;
        // LAB 6: Your code here:
        //      - read a packet from the network server
        //      - send the packet to the device driver
        envid_t from_env;
        while(1){       
                if( ipc_recv(&from_env, &nsipcbuf, &perm) != NSREQ_OUTPUT)
                        continue;
                while((r = sys_packet_try_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)<0))
                        sys_yield();
        }       
}
