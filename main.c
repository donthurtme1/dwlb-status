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
#include <poll.h>
#include <sys/socket.h>

#define VOLUME

void cleanup(void);
void sigint(int signum);
void sigkill(int signum);

static const char *wdaystr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *mdaypostfix[] = { "st", "nd", "rd" };
static const char *monstr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char status[64];
static pid_t dwlb_pid;

void
cleanup(void) {
	if (dwlb_pid) kill(dwlb_pid, SIGTERM);
}

void
sighandler(int signum) {
	switch (signum) {
		case SIGINT:
			if (dwlb_pid) kill(dwlb_pid, SIGINT);
			exit(EXIT_SUCCESS);
			break;
		case SIGKILL:
			if (dwlb_pid) kill(dwlb_pid, SIGKILL);
			exit(EXIT_SUCCESS);
			break;
	}
}

int
main(int argc, char *argv[]) {
	static struct termios term;
	char timestr[6] = "00:00";
	const char *s;
	char *volume;
	time_t timer;
	struct tm tm;
	struct timespec tc;

	/* Setup */
	signal(SIGINT, &sighandler);
	signal(SIGKILL, &sighandler);
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

#ifdef VOLUME
	/* Open pipewire socket */
#endif

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
		clock_gettime(CLOCK_MONOTONIC, &tc);
		time(&timer);
		localtime_r(&timer, &tm);

#ifdef VOLUME
		/* Volume */
		int pulse_fd[2];
		pipe(pulse_fd);
		int pulse_pid = fork();
		if (pulse_pid == 0) {
			close(pulse_fd[0]);
			dup2(pulse_fd[1], STDOUT_FILENO);
			execvp("pulsemixer", (char *[]){ "pulsemixer", "--get-volume", "--id", "60", NULL });
			exit(EXIT_FAILURE);
		}
		dup2(pulse_fd[0], STDIN_FILENO);
		close(pulse_fd[1]);
		size_t sizezezeze;
		if (wait(&pulse_pid) < 0)
			kill(pulse_pid, SIGKILL);
		int len = getline(&volume, &sizezezeze, stdin);
		volume[len - 1] = '\0';
		close(pulse_fd[0]);
#endif

#ifdef LAPTOP
		/* Laptop charge */
		lseek(fileno(bat_now_fp), 0, SEEK_SET);
		fscanf(bat_now_fp, "%u", &now);
#endif

		/* Only do calculations if the time has changed */
		static int old_min = -1;
		static float old_volume = -1;
		if (1) {
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
#ifdef VOLUME
			printf("Vol %s%%   --    ", volume);
#endif
			printf("%s %d%s %s %d  ::  %s (UTC+%d)\n", wdaystr[tm.tm_wday], tm.tm_mday, s, monstr[tm.tm_mon], tm.tm_year + 1900, timestr, (int)(tm.tm_gmtoff / 3600));
			fflush(stdout);
		}

		tc.tv_sec++;
		tc.tv_nsec = 0;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tc, NULL);

	}

	return 0;
}
