/* Todo: 
 * - reminders  (reminders to do jobs, popup window,
 *           more frequent throughout the day until marked as completed)
 * - notifications */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

static const char *wdaystr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *mdaypostfix[] = { "st", "nd", "rd" };
static const char *monstr[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char status[64];
static pid_t dwlbpid, wppid;

void
cleanup(void) {
	if (dwlbpid)
		kill(dwlbpid, SIGTERM);
	if (wppid)
		kill(wppid, SIGTERM);
}

void
sigint(int signum) {
	if (dwlbpid)
		kill(dwlbpid, SIGINT);
	if (wppid)
		kill(wppid, SIGINT);
	putc('\n', stdout);
	exit(0);
}

void
sigkill(int signum) {
	if (dwlbpid)
		kill(dwlbpid, SIGKILL);
	if (wppid)
		kill(wppid, SIGKILL);
	exit(0);
}

int
main(int argc, char *argv[]) {
	static struct termios term;
	char timestr[6] = "00:00";
	float volume;
	time_t timer;
	struct tm tm;
	struct timespec tp;
	int dwlbfd[2], wpfd[2]; /* dwlb and wireplumber fd's */

	/* Setup signals */
	signal(SIGINT, &sigint);
	signal(SIGKILL, &sigkill);
	atexit(&cleanup);

	/* Setup pipes */
	pipe(dwlbfd);
	pipe(wpfd);

	if ((dwlbpid = fork()) == 0) {
		dup2(dwlbfd[0], STDIN_FILENO);
		execvp("dwlb", (char *[]){ "dwlb", "-status-stdin", "all", NULL });
		return 0;
	}

	dup2(dwlbfd[1], STDOUT_FILENO);
	dup2(dwlbfd[1], STDERR_FILENO);
	while (1) {
		if (clock_gettime(CLOCK_MONOTONIC, &tp) == -1)
			die("clock_gettime");
		time(&timer);
		if (localtime_r(&timer, &tm) == NULL)
			die("localtime_r");

		/* Volume */
		/*
		if ((wppid = fork()) == 0) {
			dup2(wpfd[1], STDOUT_FILENO);
			execvp("wpctl", (char *[]){ "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL });
			return 0;
		}
		dup2(wpfd[0], STDIN_FILENO);
		fscanf(stdin, "%*s%f\n", &volume);
		*/

		/* Clock string */
		timestr[4] = (tm.tm_min % 10) + '0';
		timestr[3] = (tm.tm_min - (timestr[4] - '0')) / 10 + '0';
		timestr[1] = (tm.tm_hour % 10) + '0';
		timestr[0] = (tm.tm_hour - (timestr[1] - '0')) / 10 + '0';

		printf("%s %d%s %s %d  %s\n", wdaystr[tm.tm_wday], tm.tm_mday,
				(unsigned int)(tm.tm_mday % 20 - 1) < 3 ? mdaypostfix[tm.tm_mday % 20 - 1] : "th",
				monstr[tm.tm_mon], tm.tm_year + 1900, timestr);
		fflush(stdout);

		tp.tv_sec++;
		tp.tv_nsec = 0;
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) != 0)
			die("clock_nanosleep");
	}

	kill(dwlbpid, SIGINT);
	return 0;
}
