#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

#include "./src/nob.h"
#include <dirent.h>

#define DEPS_DIR "deps"
#define BUILD "build"

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

    if(!nob_mkdir_if_not_exists(BUILD)) 
    nob_log(NOB_ERROR, "Could not create directory");

    const char *lib = "build/libplug.so";
    const char *exe = "build/msode";

    // BUILDING LIB-PLUG
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-fPIC", "-shared", "-o", lib, "./src/plug.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib", "-ldl");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = 0;

    // BUILDING THE PROJECT
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-o", exe, "./src/msode.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib", "-ldl");
    nob_cmd_append(&cmd, "-Lbuild/");
    if (!nob_cmd_run_sync(cmd)) return 1;

    return 0;


}
