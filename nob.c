#include <string.h>
#define NOB_IMPLEMENTATION
#include "nob.h"


int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char* program = nob_shift_args(&argc, &argv);

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "cc");
	nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb", "-pedantic");
	nob_cmd_append(&cmd, "-o", "main");
	nob_cmd_append(&cmd, "main.c");
	if (!nob_cmd_run_sync(cmd)) return 1;

	if (argc > 0) {
		const char* subcmd = nob_shift_args(&argc, &argv);

		if (strcmp(subcmd, "run") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "./main");
			nob_da_append_many(&cmd, argv, argc);
			if (!nob_cmd_run_sync(cmd)) return 1;
		} else if (strcmp(subcmd, "clean") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "rm", "main");
			if (!nob_cmd_run_sync(cmd)) return 1;
		} else {
			fprintf(stderr, "Unknown subcommand: %s\n", subcmd);
			return 1;
		}
	}

	return 0;
}

