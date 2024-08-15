/* Todo: 
 * - reminders  (reminders to do jobs, popup window,
 *           more frequent throughout the day until marked as completed)
 * - notifications */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static const char *wdaystr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *mdaypostfix[] = { "st", "nd", "rd" };
static const char *monstr[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char status[64];
static struct termios term;

void
sigint(int signum) {
	exit(0);
}

int
main(int argc, char *argv[]) {
	char timestr[6] = "00:00";
	float volume;

	int pipefd[2];
	pipe(pipefd);
	if (fork() == 0) {
		dup2(pipefd[0], STDIN_FILENO);
		execvp("dwlb", (char *[]){ "dwlb", "-status-stdin", "all", NULL });
		return 0;
	}

	//printf("\n");
	dup2(pipefd[1], STDOUT_FILENO);

	int volfd[2];
	pipe(volfd);
	time_t timer;
	struct tm tm;
	struct timespec tp;
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &tp);
		time(&timer);
		localtime_r(&timer, &tm);

		/* Volume string */
		if (fork() == 0) {
			dup2(volfd[1], STDOUT_FILENO);
			execvp("wpctl", (char *[]){ "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL });
			close(STDOUT_FILENO);
			return 0;
		}
		dup2(volfd[0], STDIN_FILENO);
		scanf("%*s%f", &volume);
		close(STDIN_FILENO);

		/* Clock string */
		if (timestr[4] - '0' != tm.tm_min % 10) {
			timestr[4] = (tm.tm_min % 10) + '0';
			timestr[3] = (tm.tm_min - (timestr[4] - '0')) / 10 + '0';
		}
		if (timestr[1] - '0' != tm.tm_hour % 10) {
			timestr[1] = (tm.tm_hour % 10) + '0';
			timestr[0] = (tm.tm_hour - (timestr[1] - '0')) / 10 + '0';
		}

		printf("%d%%   %s %d%s %s %d   %s\n", (int)(volume * 100), wdaystr[tm.tm_wday], tm.tm_mday,
				(unsigned int)(tm.tm_mday % 10 - 1) < 3 ? mdaypostfix[tm.tm_mday % 10] : "th",
				monstr[tm.tm_mon], tm.tm_year + 1900, timestr);
		fflush(stdout);

		tp.tv_sec++;
		tp.tv_nsec = 0;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL);
	}

	return 0;
}
