/* Todo: 
 * - reminders  (reminders to do jobs, popup window, more frequent throughout the day until marked as completed)
 * - notifications */
#include <pipewire/pipewire.h>
#include <physfs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

void cleanup(void);
void sigint(int signum);
void sigkill(int signum);

static const char *wdaystr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *mdaypostfix[] = { "st", "nd", "rd" };
static const char *monstr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char status[64];
static pid_t dwlb_pid, wireplumber_pid;

void
cleanup(void) {
	if (dwlb_pid) kill(dwlb_pid, SIGTERM);
	if (wireplumber_pid) kill(wireplumber_pid, SIGTERM);
}

void
sigint(int signum) {
	if (dwlb_pid) kill(dwlb_pid, SIGINT);
	if (wireplumber_pid) kill(wireplumber_pid, SIGINT);
	putc('\n', stdout);
	exit(EXIT_SUCCESS);
}

void
sigkill(int signum) {
	if (dwlb_pid) kill(dwlb_pid, SIGKILL);
	if (wireplumber_pid) kill(wireplumber_pid, SIGKILL);
	exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[]) {
	static struct termios term;
	char timestr[6] = "00:00";
	const char *s;
	float volume;
	time_t timer;
	struct tm tm;
	struct timespec tc;

	/* Setup */
	signal(SIGINT, &sigint);
	signal(SIGKILL, &sigkill);
	atexit(&cleanup);

	/* Setup pipes */
	int dwlb_fd[2];

	pipe(dwlb_fd);

	dwlb_pid = fork();
	if (dwlb_pid == 0) {
		dup2(dwlb_fd[0], STDIN_FILENO);
		close(dwlb_fd[1]);
		execvp("dwlb", (char *[]){ "dwlb", "-status-stdin", "all", NULL });
		return 0;
	}

	close(dwlb_fd[0]);
	dup2(dwlb_fd[1], STDOUT_FILENO);

#ifdef LAPTOP
	/* Setup power supply file pointers */
	FILE *bat_now_fp, *bat_full_fp;
	unsigned int now, full;

	bat_now_fp = fopen("/sys/class/power_supply/BAT1/energy_now", "r");
	fscanf(bat_now_fp, "%u", &now);

	bat_full_fp = fopen("/sys/class/power_supply/BAT1/energy_full", "r");
	fscanf(bat_full_fp, "%u", &full);
	fclose(bat_full_fp);
#endif

	while (1) {
		if (clock_gettime(CLOCK_MONOTONIC, &tc) == -1)
			die("clock_gettime");
		time(&timer);
		if (localtime_r(&timer, &tm) == NULL)
			die("localtime_r");

		/* Volume */
		//int wireplumber_fd[2];
		//pipe(wireplumber_fd);
		//wireplumber_pid = fork();
		//if (wireplumber_pid == 0) {
		//	close(wireplumber_fd[0]);
		//	dup2(wireplumber_fd[1], STDOUT_FILENO);
		//	execvp("wpctl", (char *[]){ "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL });
		//	exit(EXIT_FAILURE);
		//}
		//dup2(wireplumber_fd[0], STDIN_FILENO);
		//close(wireplumber_fd[1]);
		//wait(&wireplumber_pid);
		//fscanf(stdin, "%*s%f\n", &volume);

#ifdef LAPTOP
		/* Laptop charge */
		lseek(fileno(bat_now_fp), 0, SEEK_SET);
		fscanf(bat_now_fp, "%u", &now);
#endif

		/* Only do calculations if the time has changed */
		static int old_min = -1;
		if (old_min != tm.tm_min) {
			old_min = tm.tm_min;

			/* Clock string */
			timestr[4] = (tm.tm_min % 10) + '0';
			timestr[3] = (tm.tm_min - (timestr[4] - '0')) / 10 + '0';
			timestr[1] = (tm.tm_hour % 10) + '0';
			timestr[0] = (tm.tm_hour - (timestr[1] - '0')) / 10 + '0';
			if ((tm.tm_mday < 11 || tm.tm_mday > 13) && (tm.tm_mday % 10 > 0) &&
					(tm.tm_mday % 10 < 4)) {
				s = mdaypostfix[(tm.tm_mday % 10) - 1];
			} else {
				s = "th";
			}

#ifdef LAPTOP
			printf("Bat %d%%   --    ", (int)(((float)now / (float)full) * 100));
#endif
#ifdef VOL
			printf("Vol %d%%   --    ", (int)(volume * 100));
#endif
			printf("%s %d%s %s %d  ::  %s (UTC+%d)\n", wdaystr[tm.tm_wday], tm.tm_mday, s, monstr[tm.tm_mon], tm.tm_year + 1900, timestr, (int)(tm.tm_gmtoff / 3600));
			fflush(stdout);
		}

		tc.tv_sec++;
		tc.tv_nsec = 0;
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tc, NULL) != 0)
			die("clock_nanosleep");
	}

	return 0;
}
