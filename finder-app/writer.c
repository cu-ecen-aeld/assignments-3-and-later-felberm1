#include <syslog.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	openlog("writer_log", LOG_PID, LOG_USER);

	if (argc != 3)
	{
		syslog(LOG_ERR, "Number of arguments wrong: 1. path to file including filename, 2. text to write in file");
		closelog();
		return 1;
	}

	char *writefile = argv[1];
	char *writestr = argv[2];

	FILE *file = fopen(writefile, "w");
	if (file == NULL)
	{
		syslog(LOG_ERR, "Failed to create file %s.", writefile);
		closelog();
		return 1;
	}

	fprintf(file, "%s", writestr);
	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
	fclose(file);

	closelog();
	return 0;

}
