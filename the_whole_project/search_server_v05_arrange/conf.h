



       #include <sys/socket.h>//func:soket()
       #include <sys/ioctl.h>//func:ioctl()
       #include <errno.h>//func :perror()
       #include <string.h>//strcpy
       #include <stdio.h>//sprintf
       #include <net/if.h>//ifreq
       #include <netinet/in.h>//sockaddr_in
       #include <arpa/inet.h>//inet_ntoa
       #include <stdlib.h>//EXIT_FAILURE
       #include <db.h>
       #include <unistd.h> //fork
       #include <sys/types.h>  //fork
       #include <sys/stat.h>
       #include <fcntl.h>   //open
#define MAXBUF        1024
#define MAXPATH        128
char buffer[MAXBUF + 1];
char ip[128];
char port[128];
char back[128];
char home_dir[128];
char ethname[128];
int get_addr(char *str)//由接口名称获得ip地址
{
        int inet_sock;
        struct ifreq ifr;
        inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
        strcpy(ifr.ifr_name, str);
        if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
        {
		//	wrtinfomsg("bind");
			perror("bind");
			exit(EXIT_FAILURE);
		}
        sprintf(ip,"%s", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
}

int get_arg (char *cmd)//读取参数的函数
{

		FILE* fp;
        char buffer[1024];
        size_t bytes_read;
        char* match;
        fp = fopen ("search.conf", "r");
        bytes_read = fread (buffer, 1, sizeof (buffer), fp);
        fclose (fp);

        if (bytes_read == 0 || bytes_read == sizeof (buffer))
                return 0;
        buffer[bytes_read] = '\0';

        if(!strncmp(cmd,"home_dir",8))
        {
                match = strstr (buffer, "home_dir=");
                if (match == NULL)
                        return 0;
                bytes_read=sscanf(match,"home_dir=%s",home_dir);
                return bytes_read;
        }

        else if(!strncmp(cmd,"port",4))
        {
                match = strstr (buffer, "port=");
                if (match == NULL)
                        return 0;
                bytes_read=sscanf(match,"port=%s",port);
                return bytes_read;
        }
       else if(!strncmp(cmd,"ethname",7))
        {
                match = strstr (buffer, "ethname=");
                if (match == NULL)
                        return 0;
                bytes_read=sscanf(match,"ethname=%s",ethname);
                return bytes_read;
        }
        //else if(!strncmp(cmd,"ip",2))
        //{
                //match = strstr (buffer, "ip=");
                //if (match == NULL)
                        //return 0;
                //bytes_read=sscanf(match,"ip=%s",ip);
                //return bytes_read;
        //}
        else if(!strncmp(cmd,"back",4))
        {
                match = strstr (buffer, "back=");
                if (match == NULL)
                        return 0;
                bytes_read=sscanf(match,"back=%s",back);
                return bytes_read;
        }
	 else
			return 0;
}
