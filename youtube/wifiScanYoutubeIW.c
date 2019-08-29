/*
step 1: find all network interfaces in our system. ifaddrs.h
step 2: get only the wireless network interfaces. iwlib.h 'wireless tools library'
step 3: launch a scan function for each one.

*/

#include <stdio.h>
#include <ifaddrs.h>
#include <iwlib.h>

int main(){

    wireless_scan_head myScanHead;
    wireless_scan *myScanInterator;
    iwrange myWirelessRange;

    int mySocket;
    mySocket = iw_sockets_open();

    struct ifaddrs *myInterfaces;
    if(getifaddrs(&myInterfaces) == -1){
        printf("Error in get interfaces.\n");
        return -1;
    }

    struct ifaddrs *myInterfacesIterator = myInterfaces;
    while(myInterfacesIterator != NULL){

        // print the name and the family of the interface
        //printf("Interface name is %s, %u \n", myInterfacesIterator->ifa_name, myInterfacesIterator->ifa_addr->sa_family);

        // use only interfaces with address family equal to AF_INET which is the IP version 4
        if(myInterfacesIterator->ifa_addr->sa_family == AF_INET){
            // use the iw_get_basic_config or the print_iface_version_into function to check if this interface is wireless or not.
            wireless_config myWirelessConfig;
            if(iw_get_basic_config(mySocket, myInterfacesIterator->ifa_name, &myWirelessConfig) != -1){
                // get range info
                if(iw_get_range_info(mySocket, myInterfacesIterator->ifa_name, &myWirelessRange) != -1){
                    // launch scan function
                    if(iw_scan(mySocket, myInterfacesIterator->ifa_name, myWirelessRange.we_version_compiled, &myScanHead) != -1){
                        // print the scan result
                        printf("Scan result for %s:\n", myInterfacesIterator->ifa_name);
                        myScanInterator = myScanHead.result;
                        while(myScanInterator != NULL){
                            printf("    %s \n", myScanInterator->b.essid);
                            myScanInterator = myScanInterator->next;
                        }
                    }
                }
            }
        }

        myInterfacesIterator = myInterfacesIterator->ifa_next;
    }

    return 0;
}
