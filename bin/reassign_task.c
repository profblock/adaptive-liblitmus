#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#include "litmus.h"
#include "migration.h"
#include "common.h"

const char *usage_msg =
	"Usage: reassign_task OPTIONS PID\n"
	"    -a ID             reassign task to cluster/partition ID\n"
	"    -s DELAY          stress test (migrate randomly every DELAY ms)\n"
	"    PID               PID of the task to reassign\n"
	"\n";

void usage(char *error) {
	fprintf(stderr, "%s\n%s", error, usage_msg);
	exit(1);
}

#define OPTSTR "s:ha:"

int main(int argc, char** argv)
{
	int ret, opt;

	struct rt_task param;

	pid_t target;
	int cluster = -1;
	int want_stress_test = 0;
	lt_t delay = 0;

	while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
		switch (opt) {
		case 's':
			want_stress_test = 1;
			/* delay given in ms, scaled to seconds */
			delay = atof(optarg) * 1E6;
			break;
		case 'a':
			cluster = atoi(optarg);
			break;
		case 'h':
			usage("");
			break;
		case ':':
			usage("Argument missing.");
			break;
		case '?':
		default:
			usage("Bad argument.");
			break;
		}
	}

	if (argc - optind < 1)
		usage("PID missing");

	target   = atoi(argv[optind + 0]);
	if (!target)
		usage("invalid PID");


	init_rt_task_param(&param);

	/* First, let's get the task's current parameters. */
	ret = get_rt_task_param(target, &param);
	if (ret < 0)
		bail_out("could not get the task's current RT parameters");

	if (cluster != -1) {
		/* move task to new cluster */
		param.cpu = domain_to_first_cpu(cluster);
		printf(":: assigning %d to P%u\n", target, param.cpu);
		ret = set_rt_task_param(target, &param);
		if (ret < 0)
			bail_out("could not set new RT parameters");
	}

	srand(time(NULL));
	while (want_stress_test) {
		lt_sleep(delay);
		param.cpu = rand() % num_online_cpus();
		printf(":: assigning %d to P%u\n", target, param.cpu);
		ret = set_rt_task_param(target, &param);
		if (ret < 0)
			bail_out("could not set new RT parameters");
	}

	return 0;
}
