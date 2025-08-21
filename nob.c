#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

#include "nob.h"

#define BUILD "build"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    nob_cmd_append(&cmd, "bash", "configure");
    if (!nob_cmd_run(&cmd)) return 1;

    if(!nob_mkdir_if_not_exists(BUILD)) 
    nob_log(NOB_ERROR, "Could not create directory");

    const char *lib = "build/libplug.so";
    const char *exe = "build/msode";

    // BUILDING LIB-PLUG
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb",  "-Ideps/quickui/src","-fPIC", "-shared", "-o", lib,  "./deps/quickui/src/quickui.c","./src/plug.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib", "-ldl");
    if (!nob_cmd_run(&cmd)) return 1;

    // BUILDING THE PROJECT
    nob_cmd_append(&cmd, "cc", "-Wall","-Wextra", "-ggdb", "-Ideps/quickui/src","-o", exe, "./deps/quickui/src/quickui.c", "./src/msode.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib", "-ldl");
    nob_cmd_append(&cmd, "-Lbuild/");
    if (!nob_cmd_run(&cmd)) return 1;

    return 0;


}
