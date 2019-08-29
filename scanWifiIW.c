#include <stdio.h>
#include <time.h>
#include <ifaddrs.h>
#include <iwlib.h>

int main(void) {
  wireless_scan_head head;
  wireless_scan *result;
  iwrange range;
  int sock;

  /* Open socket to kernel */
  sock = iw_sockets_open();
  
  struct ifaddrs *myInterfaces;
  
  if(getifaddrs(&myInterfaces) == -1){
    return -1;
  }

  struct ifaddrs *myInterfacesIterator = myInterfaces;
  while(myInterfacesIterator != NULL){
    wireless_config  myWirelessConfig;
    // we can use the iw_get_basic_config or print_iface_version_info to check if this interface is a wireless interface or not.
    if(myInterfacesIterator->ifa_addr->sa_family == AF_INET){
      if(iw_get_basic_config(sock,(const char*) myInterfacesIterator->ifa_name, &myWirelessConfig) != -1){
        
        printf("interface name is %s \n", (char*) myInterfacesIterator->ifa_name);
        /* Get some metadata to use for scanning */
        if (iw_get_range_info(sock, myInterfacesIterator->ifa_name, &range) < 0) {
          printf("Error during iw_get_range_info. Aborting.\n");
          exit(2);
        }

        /* Perform the scan */
        if (iw_scan(sock, myInterfacesIterator->ifa_name, range.we_version_compiled, &head) < 0) {
          printf("Error during iw_scan. Aborting.\n");
          exit(2);
        }

        /* Traverse the results */
        result = head.result;
        while (NULL != result) {
          printf("%s\n", result->b.essid);
          result = result->next;
        }
        
      
      }
    }
    myInterfacesIterator = myInterfacesIterator->ifa_next;
  }

  exit(0);
}
