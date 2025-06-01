/* Todo: 
 * - reminders  (reminders to do jobs, popup window, more frequent throughout the day until marked as completed)
 * - notifications */
#include <physfs.h>
#include <fcntl.h>
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

void cleanup(void);
void sigint(int signum);
void sigkill(int signum);

static const char *wdaystr[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *mdaypostfix[] = { "st", "nd", "rd" };
static const char *monstr[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char status[64];
static pid_t dwlb_pid;

void
cleanup(void)
{
	if (dwlb_pid) kill(dwlb_pid, SIGTERM);
}

void
sighandler(int signum)
{
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

void
set_title(char *title)
{
	pid_t dwlb_pid = fork();
	if (dwlb_pid == 0) {
		execvp("dwlb", (char *[]){ "dwlb", "-title", "all", title, NULL });
		exit(1);
	}
	waitid(P_PID, dwlb_pid, NULL, WEXITED | WSTOPPED);
}

void
get_volume(char *volstr)
{
	int wpctl_filedes[2];
	pipe(wpctl_filedes);

	pid_t wpctl_pid = fork();
	if (wpctl_pid == 0)
	{
		close(wpctl_filedes[0]);
		dup2(wpctl_filedes[1], 1);
		execvp("wpctl", (char *[]){ "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL });
		exit(1);
	}
	waitid(P_PID, wpctl_pid, NULL, WEXITED | WSTOPPED);

	struct pollfd poll_fds = { .fd = wpctl_filedes[0], .events = POLLIN };
	int poll_result = poll(&poll_fds, 1, 0);
	if (poll_result == 0) {
		volstr[0] = '\0';
		fprintf(stderr, "get_volume(): failed to read from wpctl\n");
		goto cleanup;
	}

	FILE *input_fp = fdopen(wpctl_filedes[0], "r");
	fscanf(input_fp, "Volume: %s", volstr);

	/* Cleanup */
	fclose(input_fp);
	clearerr(stdin);
cleanup:
	close(wpctl_filedes[0]);
	close(wpctl_filedes[1]);
}

int
str_utf8_len(char *str)
{
	int width = 0;

	while (str[width] != '\0')
	{
		/* Ascii character */
		if (str[width] <= 0x7f) {
			width++;
			continue;
		}

		/* Decode UTF-8 header */
		if (str[width] <= 0xdf) { /* 2 bytes */
			width += 2;
		}
		else if (str[width] <= 0xef) { /* 3 bytes */
			width += 3;
		}
		else if (str[width] <= 0xf7) { /* 4 bytes */
			width += 4;
		}
		else { /* Uh oh... */
			printf("Fuck off with ur non utf-8 compatible string... >:(\n");
			fprintf(stderr, "Fuck off with ur non utf-8 compatible string... >:(\n" \
				"String: %s\nIndex: %d\nIn func: `str_size_to_len()`\n", str, width);
			exit(1);
		}
	}

	return width;
}

void
write_utf8(char *str, int n)
{
	/* Count number of bytes to be written */
	int bytes = 0;
	for (int n_utf8_chars = 0;
			n_utf8_chars < n;
			n_utf8_chars++)
	{
		/* Ascii character */
		if (str[bytes] <= 0x7f) {
			bytes++;
			continue;
		}

		/* Decode UTF-8 header */
		if (str[bytes] <= 0xdf) { /* 2 bytes */
			bytes += 2;
		}
		else if (str[bytes] <= 0xef) { /* 3 bytes */
			bytes += 3;
		}
		else if (str[bytes] <= 0xf7) { /* 4 bytes */
			bytes += 4;
		}
		else { /* Uh oh... */
			printf("Fuck off with ur non utf-8 compatible string... >:(\n");
			fprintf(stderr, "Fuck off with ur non utf-8 compatible string... >:(\n" \
				"String: %s\nIndex: %d\nIn func: `write_utf8()`\n", str, bytes);
			exit(1);
		}
	}

	/* Write the thing */
	fwrite(str, 1, bytes, stdout);
}

int
scrolling_text_offset(char *utf8_str, int timer)
{
	static int offset = -1;

	return 0;
}

/*
 * Returns length of string in bytes
 */
int
get_mpd_song(char *mpd_str, size_t max_len)
{
	int mpc_filedes[2];
	pipe(mpc_filedes);

	pid_t mpc_pid = fork();
	if (mpc_pid == 0)
	{
		close(mpc_filedes[0]);
		dup2(mpc_filedes[1], 1);
		execvp("mpc", (char *[]){ "mpc", "current", NULL });
		exit(1);
	}
	waitid(P_PID, mpc_pid, NULL, WEXITED | WSTOPPED);

	struct pollfd poll_fds = { .fd = mpc_filedes[0], .events = POLLIN };
	int poll_result = poll(&poll_fds, 1, 10);
	if (poll_result == 0) {
		mpd_str[0] = '\0';
		close(mpc_filedes[0]);
		close(mpc_filedes[1]);
		fprintf(stderr, "get_mpd_song(): failed to read from mpc\n");
		return 0;
	}

	FILE *input_fp = fdopen(mpc_filedes[0], "r");
	size_t size = max_len;
	char *line = NULL;
	int len = getline(&line, &size, input_fp);
	if (len > max_len)
		len = max_len;
	strncpy(mpd_str, line, len);
	mpd_str[len - 1] = '\0';

	/* Cleanup */
	fclose(input_fp);
	clearerr(stdin);
	close(mpc_filedes[0]);
	close(mpc_filedes[1]);
	fprintf(stderr, "get_mpd_song(): read %s from mpc\n", mpd_str);

	return len;
}

int
main(int argc, char *argv[])
{
	static struct termios term;
	char timestr[6] = "00:00";
	const char *s;
	char volume[8];
	char mpd_str[128];
	time_t timer;
	struct tm tm;
	struct timespec tc;

	int dev_null_fd = open("/dev/null", 0);
	dup2(dev_null_fd, STDERR_FILENO);
	close(dev_null_fd);

	/* Setup */
	signal(SIGINT, &sighandler);
	signal(SIGKILL, &sighandler);
	atexit(&cleanup);
	set_title("^fg()^bg()  │");

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

	clock_gettime(CLOCK_MONOTONIC, &tc);
	while (1) {
		time(&timer);
		localtime_r(&timer, &tm);

#ifdef LAPTOP
		/* Laptop charge */
		lseek(fileno(bat_now_fp), 0, SEEK_SET);
		fscanf(bat_now_fp, "%u", &now);
#endif

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

		int mpd_strsize = get_mpd_song(mpd_str, sizeof(mpd_str));
		if (mpd_strsize > 0)
		{
			int mpd_strlen = str_utf8_len(mpd_str);
			/* Ensure title of current song will fit in bar */
			if (mpd_strlen < 67) {
				printf("^fg(e0def4)Playing:  %s    ^fg()", mpd_str);
			}
			else { /* Scroll text */
				static int scroll_timer = 0, offset = 0;

				printf("^fg(e0def4)Playing:  ");
				write_utf8(&mpd_str[offset], 67);
				printf("    ^fg()");

				/*
				 * Increment scroll_timer and calculate string index
				 */
				scroll_timer++;
				offset = scrolling_text_offset(mpd_str, scroll_timer);

				if (scroll_timer >= mpd_strsize - 67)
					scroll_timer = 0;
			}
		}

#ifdef LAPTOP
		printf("│    ^fg(e0def4)Bat %d%%    ^fg()", (int)(((float)now / (float)full) * 100));
#endif

		get_volume(volume);
		printf("│    ^fg(e0def4)Vol %s%%    ^fg()", volume);

		printf("│    ^fg(e0def4)%s %d%s %s %d - %s (UTC+%d)  \n", wdaystr[tm.tm_wday], tm.tm_mday, s, monstr[tm.tm_mon], tm.tm_year + 1900, timestr, (int)(tm.tm_gmtoff / 3600));
		fflush(stdout);

		tc.tv_nsec += 50000000;
		if (tc.tv_nsec >= 100000000) {
			tc.tv_nsec = 0;
			tc.tv_sec++;
		}
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tc, NULL);
	}

	return 0;
}
