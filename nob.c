#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

#include "./src/nob.h"
#include <dirent.h>
#include <errno.h>

#define DEPS_DIR "deps"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    // GETTING THE DEPS DIR
    DIR *dir = opendir(DEPS_DIR);
    if(dir) {
        nob_log(NOB_INFO, "DEPENDENCY DIR ALREADY EXISTS SKIPPING ...");
        closedir(dir);
    }    
    else if(ENOENT == errno){
        // nob_cmd_append(&cmd, "./autogen");
        nob_cmd_append(&cmd, "cc", "-Wall","-Wextra", "-o", "dep", "./src/deps.c");
        nob_cmd_append(&cmd, "-larchive", "-lcurl" ,"-lzip");
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
        nob_cmd_append(&cmd, "./dep");
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
        if(!nob_delete_file("./dep")) return 1;
    }
    else return 1;

    // BUILDING THE PROJECT
    nob_cmd_append(&cmd, "cc", "-Wall","-Wextra", "-o", "msode", "./src/msode.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-l:libraylib.a");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}
