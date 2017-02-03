/*
 * gcc -Wall -o getstatus getstatus.c -lasound
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>

#define TIMEFMT		"%a %d.%m.%y %H:%M"

char *BAT_0 = " ";  /* if [0%, 5%[ 	*/ 
char *BAT_1 = " ";  /* if [5%, 25%[ 	*/ 
char *BAT_2 = " ";  /* if [25%, 55%[ 	*/ 
char *BAT_3 = " ";  /* if [55%, 95%[ 	*/ 
char *BAT_4 = " ";  /* if >= 95%     	*/
char *BAT_5 = " ";  /* if charging   	*/
char *ERR = "";     /* error */

char *VOL_0 = "";   /* if volume off  */
char *VOL_1 = ""; 	 /* if volume down */
char *VOL_2 = " ";  /* if volume up   */

char *WIFI  = " ";  /* if wifi */ 
char *WIRED = " ";  /* if wired */ 
char *DOWN  = "";   /* connection down */

char *batnow = "/sys/class/power_supply/BAT1/energy_now";
char *batmax = "/sys/class/power_supply/BAT1/energy_full";
char *batsta = "/sys/class/power_supply/BAT1/status";

char date[64];
char ip[64];
char net_icon[32];
char bat_icon[32];
char vol_icon[32];
int bat_percent;
int vol_percent;

void get_batstatus(void)
{
	FILE *fid = 0; 
	
	long bat_now = 0; 
	long bat_max = 0;
	
	char batstats[12];
	
	if ((fid = fopen(batnow, "r"))) {
		
		fscanf(fid, "%ld\n", &bat_now);
		fclose(fid);

		fid = fopen(batmax, "r");
		fscanf(fid, "%ld\n", &bat_max);
		fclose(fid);

		fid = fopen(batsta, "r");
		fscanf(fid, "%s\n", batstats);
		fclose(fid);

		bat_percent = bat_now / (bat_max / 100);

		if (strcmp(batstats, "Charging") == 0)
			strcpy(bat_icon, BAT_5);
		else if (0 <= bat_percent && bat_percent < 5)
			strcpy(bat_icon, BAT_0);
		else if (5 <= bat_percent && bat_percent < 25)
			strcpy(bat_icon, BAT_1); 
		else if (25 <= bat_percent && bat_percent < 55)	
		    strcpy(bat_icon, BAT_2);		
		else if (55 <= bat_percent && bat_percent < 95)
			strcpy(bat_icon, BAT_3);
		else 
			strcpy(bat_icon, BAT_4);
	
	} else {
		bat_percent = 0;
		strcpy(bat_icon, ERR);
	}
}

/* TODO: write warning message */
void warn(char *s)
{
	;
}

void get_timedate(void)
{
	time_t t;
	struct tm *tm;
	char *s = date;
	
	if ((t = time(NULL)) == (time_t)-1) {
		warn("time");
		strcpy(s, "!");
	}
	if (!(tm = localtime(&t))) {
		warn("localtime");
		strcpy(s, "!");
	}
	if (strftime(s, 64, TIMEFMT, tm) == 0) {
		warn("strftime");
		strcpy(s, "!");
	}
}

/* TODO: do cable network */
void get_ipaddr(void)
{
	int fd;
	struct ifreq ifr;
	char *s = ip;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET; /* IPv4 address */
	strncpy(ifr.ifr_name, "wlp2s0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);
	strcpy(s, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	
	strcpy(net_icon, WIFI);
}

void get_volume(void)
{
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;

// To find card and subdevice: /proc/asound/, aplay -L, amixer controls
    snd_hctl_open(&hctl, "hw:0", 0);
    snd_hctl_load(hctl);

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

// amixer controls
    snd_ctl_elem_id_set_name(id, "Master Playback Volume");

    snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);

    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, id);

    snd_hctl_elem_read(elem, control);
    vol_percent = (int)snd_ctl_elem_value_get_integer(control,0);

    snd_hctl_close(hctl);

	if (vol_percent >= 50) {
		strcpy(vol_icon, VOL_2);
	} else if (25 < vol_percent && vol_percent < 50) {
		strcpy(vol_icon, VOL_1);
	} else {
		strcpy(vol_icon, VOL_0);	
	}
}

int main(void)
{
	get_batstatus();
	get_timedate();
	get_ipaddr();
	get_volume();
	printf("| %s %s | %s %d%% | %s %d%% | %s\n", net_icon, ip, vol_icon, vol_percent, bat_icon, bat_percent, date);

	return 0;
}
