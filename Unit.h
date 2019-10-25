#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>


void initialize_parameter_lists();
void build_property_ctrlist(char *buff, int32_t size);
void build_mode_ctrlist(char *buff, int32_t size);
