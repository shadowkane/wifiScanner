/*
    step 1: get all network interfaces in the system using ifaddrs library.
    step 2: check which one is wireless using the ioctl function
    step 3: for each wireless interface trigger and get the scan results (bunch of beacons and probe response)
    step 4: from each data (result) extract and process events
    step 5: for each event check which one has the essid or the mac address of a wireless device
    step 6: create a wireless device struction and store its informations from events
    step 7: do not add a device already exist based on the mac address
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for allocations
#include <string.h> // for string functions
#include <ifaddrs.h>
#include <linux/wireless.h> // for the typedef SIOCGIWNAME, SIOCGIWSCAN, SIOCSIWSCAN, etc... and for the iwreq struction
#include <sys/ioctl.h>

typedef struct linkedDevices{
    char *essid;
    char *macAddr;
    struct linkedDevices *nextDevices;
} device;

// append new device at the button of the list
device* appendDevice(device **listDevices){
    device *newDevice = (device*)malloc(sizeof(device));
    newDevice->essid = NULL;
    newDevice->macAddr = NULL;
    newDevice->nextDevices = NULL;

    if(*listDevices == NULL){
        *listDevices = newDevice;
    }
    else{
        device *deviceIterator = *listDevices;
        while(deviceIterator->nextDevices != NULL){
            deviceIterator = deviceIterator->nextDevices;
        }
        deviceIterator->nextDevices = newDevice;
    }
    return newDevice;
}

// get last device in the list
device* getLastDevice(device **listDevices){
    device *deviceIterator = *listDevices;
    if(deviceIterator != NULL){
        while(deviceIterator->nextDevices != NULL){
            deviceIterator = deviceIterator->nextDevices;
        }
    }
    return deviceIterator;
}

// remove doubled devices
device* revomeRedundancy(device *listDevices){
    if(listDevices == NULL){
        return NULL;
    }
    device *newListDevices = (device*)malloc(sizeof(device));
    newListDevices->essid = listDevices->essid;
    newListDevices->macAddr = listDevices->macAddr;
    newListDevices->nextDevices = NULL;

    device *deviceIterator = listDevices;
    while(deviceIterator != NULL){
        int deviceExist = 0;
        device *newListIterator = newListDevices;
        while(newListIterator->nextDevices != NULL){
            if(strcmp(newListIterator->macAddr, deviceIterator->macAddr) == 0){
                deviceExist = 1;
                break;
            }
            newListIterator = newListIterator->nextDevices;
        }
        // check the last device in new list
        if(strcmp(newListIterator->macAddr, deviceIterator->macAddr) == 0){
            deviceExist = 1;
        }

        if(deviceExist==0){
            newListIterator->nextDevices = (device*)malloc(sizeof(device));
            newListIterator->nextDevices->essid = deviceIterator->essid;
            newListIterator->nextDevices->macAddr = deviceIterator->macAddr;
            newListIterator->nextDevices->nextDevices = NULL;
        }
        deviceIterator = deviceIterator->nextDevices;
    }
    return newListDevices;
}

// extract the device mac address from event
void getMacAddr(struct iw_event *myEvent, char *dataBuffer, device *myDevice){
    // resize the event memory to store both the event header and the event data
    // IW_EV_ADDR_PK_LEN is the full length of a mac address event
    struct iw_event *newEvent = (struct iw_event*)realloc((struct iw_event*)myEvent, IW_EV_ADDR_PK_LEN);
    if(newEvent == NULL){
        return;
    }
    myEvent = newEvent;
    // copy the event data from data buffer the the event
    memcpy((char*)myEvent+IW_EV_LCP_PK_LEN, dataBuffer+IW_EV_LCP_PK_LEN, IW_EV_ADDR_PK_LEN-IW_EV_LCP_PK_LEN);

    myDevice->macAddr = myEvent->u.ap_addr.sa_data;

}

// extract the device name from event
void getEssid(struct iw_event *myEvent, char *dataBuffer, device *myDevice){
    // IW_EV_POINT_PK_LEN is length of event of type point without the pointer length. IW_EV_LCP_PK_LEN + sizeof(iw_pointer.length) + sizeof(iw_pointer.flag)
    // IW_EV_POINT_OFF is the length of the pointer (sizeof(iw_pointer.pointer)) and the offset where we should start copy data to.
    struct iw_event *newEvent = (struct iw_event*)realloc((struct iw_event*)myEvent, IW_EV_POINT_PK_LEN+IW_EV_POINT_OFF);
    if(newEvent == NULL){
        return;
    }
    myEvent = newEvent;
    // copy the essid (iw_pointer) length and flag from dataBuffer to the event
    memcpy((char*)myEvent+IW_EV_LCP_PK_LEN+IW_EV_POINT_OFF, dataBuffer+IW_EV_LCP_PK_LEN, IW_EV_POINT_LEN-IW_EV_LCP_PK_LEN);

    char *essid = (char*)malloc(myEvent->u.essid.length+1);
    memset(essid, '\0', myEvent->u.essid.length+1);
    memcpy((char*)essid, dataBuffer+IW_EV_POINT_PK_LEN, (unsigned short)myEvent->u.essid.length);
    myEvent->u.essid.pointer = essid;

    myDevice->essid = essid;
}

// event extraction function. return the next event position or in case of failure return -1
int eventExtrac(char *dataBuffer, device **listDevices){
    // create and allocat the minimum memory space for event
    // IW_EV_LCP_PK_LEN is the minimum memory space require for the event header(command) and length. (4 bytes)
    struct iw_event *myEvent = (struct iw_event*)malloc(IW_EV_LCP_PK_LEN);
    memcpy((char*)myEvent, dataBuffer, IW_EV_LCP_PK_LEN);

    // check if this event is a valid event or not by checking if the event length is equal or greater then the minimum even length
    if(myEvent->len < IW_EV_LCP_PK_LEN){
        return -1;
    }

    // the next event position is
    int nextEventPos = myEvent->len;

    // to get the device mac address check if the event cmd equal to SIOCGIWAP
    // the first event in the beacon or the probe response is the mac address, so each time we get this event it means the possibility of a new device
    if(myEvent->cmd == SIOCGIWAP){
        // add new device in the list of devices
        device *myDevice = appendDevice(listDevices);
        // get the mac address and store it in the new device
        getMacAddr(myEvent, dataBuffer, myDevice);
    }

    // to get the device mac address check if the event cmd equal to SIOCGIWESSID
    if(myEvent->cmd == SIOCGIWESSID){
        device *myDevice = getLastDevice(listDevices);
        // get the device ssid and store it in the latest device which is the current device we just append it to the list
        getEssid(myEvent, dataBuffer, myDevice);
    }

    return nextEventPos;
}


// scan wifi function for wireless devices
int scanWifiForDevices(int mySocketFD, struct ifaddrs *myInterface, device **listDevices){
    // declare buffer of type char (8 bits) to store the scan result
    char *scanResultBuffer = NULL;
    struct iwreq myRequest;
    memset(&myRequest, 0, sizeof(struct iwreq));
    strncpy(myRequest.ifr_ifrn.ifrn_name, myInterface->ifa_name, IFNAMSIZ);
    myRequest.u.data.pointer = NULL;

    // trigger scan
    if(ioctl(mySocketFD, SIOCSIWSCAN, &myRequest) == -1){
        printf("Error in trigger scan.\n");
        return -1;
    }
    // wait for at least 250 ms before you get the scan result
    sleep(1); // sleep for 1 sec

    // get/listen for the scan result 5 times
    int scanCounter;
    for(scanCounter=0; scanCounter<5; scanCounter++){
        // for each scan realloc new memory for the scan result buffer
        char *newScanResultBuffer = NULL;
        newScanResultBuffer = (char*)realloc(scanResultBuffer, IW_SCAN_MAX_DATA);
        if(newScanResultBuffer == NULL){
            if(scanResultBuffer){
                printf("Error no memory.\n");
                free(scanResultBuffer);
                return -1;
            }
        }else{
            scanResultBuffer = newScanResultBuffer;
        }

        myRequest.u.data.pointer = scanResultBuffer;
        myRequest.u.data.flags = 0;
        myRequest.u.data.length = IW_SCAN_MAX_DATA;
        // get scan result. in case of failure skip to next scan scan result
        if(ioctl(mySocketFD, SIOCGIWSCAN, &myRequest) == -1){
            continue;
        }

        // process scan result data if exist
        if(myRequest.u.data.length){
            int eventPos = 0;
            while(eventPos<myRequest.u.data.length && eventPos>=0){
                // extract event from wifi scan result
                int eventLength = eventExtrac(scanResultBuffer+eventPos, listDevices);
                if(eventLength == -1){
                    return -1;
                }
                eventPos+=eventLength;
            }
        }

    }
    return 0;
}

// return 0 incase of a wireless interface or -1 if not
int isWireless(int mySocketFD, struct ifaddrs *myInterface){
    struct iwreq *myRequest;
    myRequest = malloc(sizeof(struct iwreq));
    memset(myRequest, 0, sizeof(struct iwreq));
    strncpy(myRequest->ifr_ifrn.ifrn_name, myInterface->ifa_name, IFNAMSIZ);
    if(ioctl(mySocketFD, SIOCGIWNAME, myRequest) == 0){
        return 0;
    }
    return -1;
}

int main(){
    struct ifaddrs *myInterfaces, *myInterfacesIterator;
    int mySocketFD;
    device *listDevices = NULL;

    mySocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(mySocketFD == -1){
        printf("Error in creating socket.\n");
        return -1;
    }

    if(getifaddrs(&myInterfaces) == -1){
        printf("Error in get interfaces.\n");
        return -1;
    }

    myInterfacesIterator = myInterfaces;
    while(myInterfacesIterator!=NULL){
        if(myInterfacesIterator->ifa_addr->sa_family == AF_INET){
            if(isWireless(mySocketFD, myInterfacesIterator) == 0){
                printf("Wireless interface name is %s.\n", myInterfacesIterator->ifa_name);
                // scan wifi for wireless devices
                scanWifiForDevices(mySocketFD, myInterfacesIterator, &listDevices);
            }
        }
        myInterfacesIterator = myInterfacesIterator->ifa_next;
    }

    device *deviceIterator = revomeRedundancy(listDevices);
    while(deviceIterator != NULL){
        printf("device essid is %s and its mac address is ", deviceIterator->essid);
        int i;
        for(i=0; i<5; i++){
            printf("%02x:", (char)deviceIterator->macAddr[i]);
        }
        printf("%02x .\n", (char)deviceIterator->macAddr[5]);
        deviceIterator = deviceIterator->nextDevices;
    }
}
