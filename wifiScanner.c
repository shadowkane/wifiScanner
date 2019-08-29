
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for malloc
#include <unistd.h> // to close the socket
// add ifaddrs.h library to get all network interfaces using getifaddrs function
// this function will store all informations (name, address, mask, etc ...) in ifaddrs pointer
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/wireless.h> // for the SIOCGIWNAME and the struct iwreq
#include <sys/ioctl.h> // for the ioctl function
// for error
#include <sys/errno.h>
extern int errno;

typedef struct linkedDevices{
	char *essid; // ssid of the AP in string
	char *macAddr; //the AP mac address, (only the first 6 byte are the mac address)
	struct linkedDevices *nextDevice;
} devices;

// append the new ssid to the list if not exist. device exist mean the essid is already found, only in this program because usualy you can find multi devices with the same essid but i don't know what make the distiguish between them.
void appendDevice(devices **listDevice, char *essid){
	devices *newDevice = (devices*) malloc(sizeof(devices));
	newDevice->essid = essid;
	newDevice->nextDevice = NULL;
	
	devices *deviceIterator = *listDevice;
	
	if(*listDevice == NULL){
		*listDevice = newDevice;
	}
	else{
		while(deviceIterator->nextDevice != NULL){
			// check all essid in list
			if(strcmp(deviceIterator->essid, essid)==0){
				return;
			}
			deviceIterator = deviceIterator->nextDevice;
		}
		// check the last essid in list
		if(strcmp(deviceIterator->essid, essid)==0){
			return;
		}
		deviceIterator->nextDevice = newDevice;
	}

} 

// this function will create new device and append it at the buttom of the list
devices* addNewDevice(devices **listDevice){
	devices *newDevice = (devices*) malloc(sizeof(devices));
	newDevice->essid = NULL;
	newDevice->macAddr = NULL;
	newDevice->nextDevice = NULL;
	
	devices *deviceIterator = *listDevice;
	
	if(*listDevice == NULL){
		printf("create first device\n");
		*listDevice = newDevice;
	}
	else{
		printf("add new device\n");
		while(deviceIterator->nextDevice != NULL){
			deviceIterator = deviceIterator->nextDevice;
		}
		deviceIterator->nextDevice = newDevice;
	}
	// return the new device pointer
	return newDevice;
} 

// return the lastes device
devices* getLastDevice(devices **listDevice){
	devices *deviceIterator = *listDevice;
	// if list devices is empty return -1 (error) because if we didn't create the system in the mac addres event then something is wrong
	if(*listDevice != NULL){
		while(deviceIterator->nextDevice != NULL){
			deviceIterator = deviceIterator->nextDevice;
		}
	}
	// return the new device pointer
	return deviceIterator;
}

// remove doubled devices
devices* removeRedundency(devices *list_devices){
	printf("start remove function \n");
	if(list_devices == NULL){
		printf("it's null \n");
		return NULL;
	}
	devices *newListDevice = (devices*)malloc(sizeof(devices));
	newListDevice->essid = list_devices->essid;
	newListDevice->macAddr = list_devices->macAddr;
	newListDevice->nextDevice = NULL;
	
	devices *iteratorDevices = list_devices;
	while(iteratorDevices != NULL){
		devices* iteratorNewDevices = newListDevice;
		short found = 0;
		while(iteratorNewDevices->nextDevice != NULL){
			if(strcmp(iteratorNewDevices->macAddr, iteratorDevices->macAddr) == 0){
				found = 1;
				break;
			}
			iteratorNewDevices = iteratorNewDevices->nextDevice;
		}
		if(strcmp(iteratorNewDevices->macAddr, iteratorDevices->macAddr) == 0){
			found = 1;
		}
		if(found == 0){
			iteratorNewDevices->nextDevice = (devices*)malloc(sizeof(devices));
			iteratorNewDevices->nextDevice->essid = iteratorDevices->essid;
			iteratorNewDevices->nextDevice->macAddr = iteratorDevices->macAddr;
			iteratorNewDevices->nextDevice->nextDevice = NULL;
		}
		iteratorDevices = iteratorDevices->nextDevice;
	}
	return newListDevice;
}

// return 0 if it's a wireless interface or -1 if it's not
int checkIfWirless(char *interfaceName){
    //because we want to check if the device (interface) is wirless or not, so we will use the system call ioctl to request for information from this interface for that we need the ioctl to communicate with a file description of a socket.
    int mySocket;
    // this is the structure of the ioctl request or payload
    struct iwreq myReq;
    memset(&myReq, 0, sizeof(myReq));
    // struct iwreq is the same as struct ifreq but just redefined (u can look for the structure definition in internet). so u can just cast the struct ifreq to struct iwreq.
    // which mean we can use variable ifr_name (as the struct ifreq) to set the request name or use ifr_ifrn.ifrn_name (in struct iwreq) to set the name. it's the same thing and both will work
    strncpy(myReq.ifr_name, interfaceName, IFNAMSIZ);
    // create the endpoint communication using socket function passing the ipv4 as familly type for the domain name and using a stream communication bysed byte and the default protocol for the protocol family
    // it will return its file descriptor number, that FD will allow us to access its file, that FD can't be negative
    if(mySocket = socket(AF_INET, SOCK_STREAM, 0) == -1){
        printf("    couldn't create socket for this domain!");
        return -1;
    }
    // after creating a socket we call the ioctl function inpu output controller which is a system call that manipulate the underlying device paramaters (set or get them, mean they can be an in or out parameters) of the special file we will pass at the first argument(file description),
    // the next argument would be the request code (type int) which is the command we want to apply like to say get something to set something, in our example we use: SIOGCIWNAME, check ioclt list for full request list
    // SIOCGIWNAME /* get name == wireless protocol */
    /* SIOCGIWNAME is used to verify the presence of Wireless Extensions.
     * Common values : "IEEE 802.11-DS", "IEEE 802.11-FH", "IEEE 802.11b"...
     * Don't put the name of your driver there, it's useless. */
    //using ioctl function with SINCGIWNAME as parameter will tell us if the interface is a wirless or not by returning 0 if true.
    // the third argument not necessary because it depend on our request, for example if we want to set a parameter then we pass that parameter, and its type will be (void*)
    if(ioctl(mySocket, SIOCGIWNAME, &myReq) == 0){
        printf("    it's a wirless interface\n");
        // if this is a wireless network so the ireq_data in myReq will not be empty. so we can get the name of the protocol from the ireq_data.name.
        // what really happen is ioclt return 0 (no error) because it checks the name of the protocol and verefy if it's a wirless extension or not
        printf("    its protocol is %s\n", myReq.u.name);
        close(mySocket);
        return 0;
    }
    // if it wasn't a wireless network then close socket then return -1
    close(mySocket);
    return -1;
}

/***************** get all wifi devices essid ********************/

/* what to do to get all wifi devices essid (like AP) in area network */
// set and trigger wifi scan
// get wifi scan feedback multi-times
// for each time get a scan feedback save it in char* buffer
// from that buffer extract events
// for each event get only event with cmd = SIOCGIWESSID.
// for each essid event extract the essid string
// store all essid in linked list

/* steps to scan wifi and get all essid */
// 1. trigger wifi scan using ioctl function with SIOCSIWSCAN
// 2. get wifi scan using ioctl function with SIOCGIWSAN
// 3. store it in buffer (char *)
// 4. extract  and process events. create function to extract and process one event in a time by passing and return the next event position in the buffer
// 5. for each event get essid events, check if the received event from the previouse step is an essid event or not by checking its cmd if equal to SIOCGIWESSID or not.
// 6. for each essid event get essid string, create function that extract the essid string from the event
// 7. store each essid string in linked list. store the extracted string in a linked list of all detected devices
// 8. repeate the last 6 steps (get wifi scan to store essid) multi times, repeate the scan (the get section, because it's just a listen) multi-time to cover the maximum of devices in area.

// extract the essid from event and store it.
void getEssid(struct iw_event *myEvent, char *buffer, devices **device){
	// resize the event memory to store the iw_pointer varaible (pointer to the essid string, length and flag) using the realloc function.
	// the event struction contains the length (2 bytes), the cmd (2 bytes) and data of type union iwreq_data. in our case the data is the essid variable which is a struct iw_point which contains the pointer to the essid string (4 bytes maybe), the length(2 bytes) and the flag(2 bytes). so we need to add more size to myEvent to store this variable.
	// IW_EV_POINT_OFF is the length of the pointer and it's the offset where we should start copy data to.
	// IW_EV_POINT_LEN is the length of the event of type point without the pointer length. (IW_EV_LCP_PK_LEN + (size of struct iw_point) - IW_EV_POINT_OFF)
	// IW_EV_POINT_PK_LEN is IW_EV_LCP_PK_LEN + 4 bytes and IW_EV_LCP_PK_LEN is 4 bytes
	// to not lose the original data, realloc a new event. if it doesn't change to null, then it's safe the change the old event to the new event.(the realloc will maintain the original data of that memery)
	struct iw_event *newEvent = (struct iw_event*)realloc((struct iw_event*)myEvent, IW_EV_POINT_PK_LEN+IW_EV_POINT_OFF);
	if(newEvent == NULL){
		return;
	}
	myEvent = newEvent;
	// copy the length and the flag from the buffer to the event. the size of this data is total size of the event minus the size of the even header and minus the size of the pointer.
	// we don't have the size of the event of type pointer but we have the size of the event of type pointer - the size of the poiter "IW_EV_LCP_PK_LEN"
	// the size of length and flag
	int extraLen = IW_EV_POINT_LEN - IW_EV_LCP_PK_LEN;
	// the location of length and offset is the copy distination start after the event pointer + the pointer size(start offset).
	memcpy((char*) myEvent + IW_EV_LCP_PK_LEN + IW_EV_POINT_OFF, buffer+IW_EV_LCP_PK_LEN, extraLen);
	char *essidName = (char*)malloc(myEvent->u.essid.length+1);
	memset(essidName, '\0' , myEvent->u.essid.length+1);
	memcpy((char*)essidName, buffer + IW_EV_POINT_PK_LEN, (unsigned short) myEvent->u.essid.length);
	
	myEvent->u.essid.pointer = essidName;
	(*device)->essid = essidName;
	/*(*device)->essid = (char*)malloc(myEvent->u.essid.length+1 * sizeof(char));
	memset((*device)->essid, '\0' , myEvent->u.essid.length+1);
	strncpy((*device)->essid, essidName, myEvent->u.essid.length);*/
	
	printf("essid length %u\n", myEvent->u.essid.length);
	printf("essid flags %u\n", myEvent->u.essid.flags);	
	printf("essid name %s\n", myEvent->u.essid.pointer);
}	

// get AP mac address
void getMacAddr(struct iw_event *myEvent, char *buffer, devices **device){
	// to resize the event to the length of the header of an event plus the size of the sockaddr struction, use IW_EV_ADDR_LEN
	struct iw_event *newEvent = (struct iw_event*)realloc((struct iw_event*)myEvent, IW_EV_ADDR_PK_LEN);
	if(newEvent == NULL){
		return;
	}
	myEvent = newEvent;
	memcpy((char*)myEvent+IW_EV_LCP_PK_LEN, buffer+IW_EV_LCP_PK_LEN, IW_EV_ADDR_PK_LEN-IW_EV_LCP_PK_LEN);

	// save the mac addres (6 bytes) in the device
	(*device)->macAddr = (char*)malloc(6);
	memcpy((*device)->macAddr, myEvent->u.ap_addr.sa_data, 6*sizeof(char));
	(*device)->macAddr = myEvent->u.ap_addr.sa_data;
	//print result just to check
	printf("the AP mac address is=");
	int i=0;
	for(i=0;i<5;i++){
		printf("%02x:",(char)myEvent->u.ap_addr.sa_data[i]);
	}
	printf("\n");
}

// extract and process one event from buffer and return the next event position in that buffer.
int extractWifiScanEvent(char *buffer, devices **list_devices){
	// create and allocate memory space for event.
	// the minimum memory space we need is IW_EV_LCP_PK_LEN (4bytes) (event length without the union iwreq_data length), this will contain the event length and event command (2 bytes each). we will call it event header in comments
	struct iw_event *myEvent = (struct iw_event*)malloc(IW_EV_LCP_PK_LEN);
	// copy the event header from the buffer to myEvent.(use char type as size because 1 char = 1 byte)
	memcpy((char*)myEvent, buffer,IW_EV_LCP_PK_LEN);
	// check if this event is an correct event by checking the length of the event (the second 2byte in header represent the full length of the event) if it's logical or not. (a logical event is an event with a length > event header length)
	// if it's incorrect event, then leave the whole buffer because it's incorrect.
	if(myEvent->len < IW_EV_LCP_PK_LEN){
		return -1;
	}
	
	int nextPos = myEvent->len;
	
	// check if this even is an AP mac address event
	// the first event in the beacon or the probe response is the mac address. so each time we get this even it mean it's a new device
	if(myEvent->cmd == SIOCGIWAP){
		//add new device to the list of devices
		devices *device = NULL; 
		device = addNewDevice(list_devices);	
		getMacAddr(myEvent, buffer, &device);
	}
	// check if this even is an essid event
	if(myEvent->cmd == SIOCGIWESSID){
		devices *device = NULL;
		device = getLastDevice(list_devices);
		if(device!= NULL){
			getEssid(myEvent, buffer, &device);
		}
	}
	
	//free(myEvent);
	return nextPos;
}

// scan network to get list of available access points
int getWirelessDevices(struct ifaddrs *myInterface, devices **list_devices){
    // use buffer and newBuffer to realloc new memory for the next result and free the old one;
    char * buffer = NULL;
    char * newBuffer = NULL;
    // the default initialization of the iwreq
    struct iwreq myReq;
    memset(&myReq, 0, sizeof(myReq));
    strncpy(myReq.ifr_name, myInterface->ifa_name, IFNAMSIZ);

    //first of all trigger scan
    // set the data pointer to null, flags and length are already 0
    myReq.u.data.pointer = NULL;

    // prepare the socket we will pass its FD to the ioctl
    int socketFD;
    switch(myInterface->ifa_addr->sa_family){
        case AF_INET:
            printf("open socket of family type ipv4\n");
            if(socketFD = socket(AF_INET, SOCK_STREAM, 0) == -1){
                printf("can't create socket\n");
                return -1;
            }
            break;
        case AF_INET6:
            printf("open socket of family type ipv6\n");
            if(socketFD = socket(AF_INET6, SOCK_STREAM, 0) == -1){
                printf("can't create socket\n");
                return -1;
            }
            break;
        default:
            printf("wrong interface family(not ipv4 or ipv6)\n");
            return -1;
    }

    // initiate scan using the SIOCSIWSCAN request
    if(ioctl(socketFD, SIOCSIWSCAN, &myReq) == -1){
        fprintf(stderr, "something wrong %d\n", errno);
        close(socketFD);
        return -1;
    }
    // we need to wait at least 250 ms before we check scan result
    sleep(1); // 1 sec

    // get scan results
    int counter = 0; // to keep listen for result for a periode of time
    scanAgain:
    newBuffer = (char *)realloc(buffer, IW_SCAN_MAX_DATA);
    // if the newBuffer is NULL mean it failed the realloc so keep our original buffer
    if(newBuffer == NULL){
        if(buffer){
            free(buffer);
        }
    }
    // if we successfuly realloc new buffer, we change the original buffer to the new one
    else{
        buffer = newBuffer;
    }

    // initialize the myReq variable so we don't get bad address (error 14) in ioctl function when we want to get scan
    myReq.u.data.pointer = buffer; // reserve one block of memory
    myReq.u.data.flags = 0;
    myReq.u.data.length = IW_SCAN_MAX_DATA; // for ipv4 it's 14 and ipv6 it's 40
    if(ioctl(socketFD, SIOCGIWSCAN, &myReq) == -1){
        fprintf(stderr, "something wrong %d\n", errno);
        // if the error was 11 which mean try again, wait for a while and then back to get scan again
        if(errno == 11){
            sleep(2);
            goto scanAgain;
        }
        close(socketFD);
        return -1;
    }

    // check if we have results from scan
    if(myReq.u.data.length){
		        int i = 0;
        printf("[ \n");
        for(i=0; i < myReq.u.data.length; i++){
            printf("%02X:",buffer[i]);
        }
        for(i=0; i < myReq.u.data.length; i++){
            printf("%c:",(char*)buffer[i]);
        }
        printf("] \n");
		
   
        // process data
        struct iw_event *myEventHeader;
        

        //myEven_header = (struct iw_event*)buffer;
        //myEven = (struct iw_event *) buffer;
        printf("memory size %d\n", IW_EV_LCP_PK_LEN);
        int c = (int)myReq.u.data.length;
        printf("total number = %d, total loop number %d\n", c, c/IW_EV_LCP_PK_LEN);
        int cmdCounter = 0;
        // process each even in buffer.
        //myEven = myEven_header;
        //printf("%d = %d and %d = %d\n", (int)myEven, (int)myEven_header, (int)myEven+IW_EV_LCP_PK_LEN, (int) myEven_header + myReq.u.data.length);
        //while(myEven+IW_EV_LCP_PK_LEN <= myEven_header + myReq.u.data.length ){
        int eventPos = 0;
                
        while(eventPos<myReq.u.data.length && eventPos>=0){
			eventPos += extractWifiScanEvent(buffer + eventPos, list_devices);
		}
    }

    counter++;
    sleep(1);

    if(counter < 15){
        goto scanAgain;
    }

    close(socketFD);
    return 0;
}

int main(){
    printf("Start scan\n");

    struct ifaddrs *myInterfaces, *myInterfacesStartPointer, *myWirelessInterfaces, *myWirelessInterfacesStartPointer;

    if(getifaddrs(&myInterfacesStartPointer) == -1){
        perror("can't get local address\n");
        return -1;
    }
    myWirelessInterfacesStartPointer = NULL;
    myWirelessInterfaces = myWirelessInterfacesStartPointer;
    myInterfaces = myInterfacesStartPointer;
    int i=0;
    while(myInterfaces->ifa_next != NULL){
        printf("%d: \n",i );
        printf("    %s\n", myInterfaces->ifa_name);
        // one of the informations in the ifaddrs struct is the network address of that interface with type struct sockaddr
        // the struct sockaddr contains 2 information, the address family and the 14 byre of protocol address (address itself and the port number) this address can be ipv4 if the familly is AF_INET or ipv6 if the familly is AF_INET6
        if(myInterfaces->ifa_addr != NULL){
            if(myInterfaces->ifa_addr->sa_family == AF_INET){
                printf("    it's an ipv4\n");
                struct sockaddr_in *socketAddr = (struct sockaddr_in *) myInterfaces->ifa_addr;
                char *ip = inet_ntoa(socketAddr->sin_addr);
                printf("    %s\n", ip);
                if(checkIfWirless(myInterfaces->ifa_name) == 0){
                    if(myWirelessInterfacesStartPointer == NULL){
                        myWirelessInterfacesStartPointer = myInterfaces;
                        myWirelessInterfaces = myWirelessInterfacesStartPointer;
                    }else{
                        myWirelessInterfaces->ifa_next = myInterfaces;
                    }
                }
            }
            else if(myInterfaces->ifa_addr->sa_family == AF_INET6){
                printf("    it's an ipv6\n");
                struct sockaddr_in6 *socketAddr6 = (struct sockaddr_in6 *) myInterfaces->ifa_addr;
                char ip6[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &socketAddr6->sin6_addr, ip6, INET6_ADDRSTRLEN);
                printf("    %s\n", ip6);
                if(checkIfWirless(myInterfaces->ifa_name) == 0){
                    if(myWirelessInterfacesStartPointer == NULL){
                        myWirelessInterfacesStartPointer = myInterfaces;
                        myWirelessInterfaces = myWirelessInterfacesStartPointer;
                    }else{
                        myWirelessInterfaces->ifa_next = myInterfaces;
                    }
                }
            }
            else if(myInterfaces->ifa_addr->sa_family == AF_PACKET){
                printf("    it's a packet\n");
            }
            else{
                printf("    unknow address family\n");
            }
        }
        i++;
        myInterfaces = myInterfaces->ifa_next;
    }
    // make sure the ifa_next elment of the last wireless interface is pointed to null, because the last network interface could be not wireless so the next element in the previouse wireless interface will point to non wireless interface
    if(myWirelessInterfacesStartPointer != NULL){
        myWirelessInterfaces->ifa_next = NULL;
    }

    printf("--------------------scan network------------------\n");
    devices *list_devices = NULL;
    myWirelessInterfaces = myWirelessInterfacesStartPointer;
    while(myWirelessInterfaces != NULL){
        if(getWirelessDevices(myWirelessInterfaces, &list_devices) == 0){
            printf("we found wireless devices\n");
        }
        myWirelessInterfaces = myWirelessInterfaces->ifa_next;
    }
    
    list_devices = removeRedundency(list_devices);
    
    devices  *device_interator = list_devices;
	while(device_interator != NULL){
		printf("detected AP %s\n", device_interator->essid);
		printf("the AP mac address is =");
		int i=0;
		for(i=0;i<5;i++){
			printf("%02x:",(char)device_interator->macAddr);
		}
		printf("\n");
		device_interator = device_interator->nextDevice;
	}
	
    printf("End scan\n");
    return 0;
}
