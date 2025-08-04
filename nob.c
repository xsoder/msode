#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

#include "nob.h"
#include <dirent.h>
#include <errno.h>

#define DEPS_DIR "deps"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    // GET DEPENDENCIES
    DIR *dir = opendir(DEPS_DIR);
    if(dir) {
        nob_log(NOB_INFO, "DEPENDENCY DIR ALREADY EXISTS SKIPPING ...");
        closedir(dir);
    }
    else if(ENOENT == errno){
        nob_cmd_append(&cmd, "./autogen");
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
    }
    else return 1;


    // BUILDING THE PROJECT
    nob_cmd_append(&cmd, "cc", "-Wall","-Wextra", "-o", "msode", "msode.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-l:libraylib.a");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}
