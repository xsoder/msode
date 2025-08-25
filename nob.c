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

    #ifdef ENABLE_HOT_RELOAD
    const char *lib = "build/libplug.so";
    #else
    const char *obj = "libplug.o";
    const char *lib = "libplug.a";
    #endif //CONFIG_H
    const char *exe = "build/msode";

    // BUILDING LIB-PLUG
    #ifdef ENABLE_HOT_RELOAD
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-Ideps/quickui/src", "-Ideps/tinyfiledialog","-fPIC", "-shared", "-o", lib,  "./deps/tinyfiledialog/tinyfiledialogs.c", "./deps/quickui/src/quickui.c","./src/plug.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib", "-ldl");
    if (!nob_cmd_run(&cmd)) return 1;
    #else
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-Ideps/quickui/src", "-Ideps/tinyfiledialog","-fPIC", "-o", obj,  "./deps/tinyfiledialog/tinyfiledialogs.c", "./deps/quickui/src/quickui.c","./src/plug.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib");    
    if (!nob_cmd_run(&cmd)) return 1;
    nob_cmd_append(&cmd, "ar", "rcs", BUILD"/"lib, BUILD"/"obj);
    if (!nob_cmd_run(&cmd)) return 1;
    #endif //CONFIG_H

    // BUILDING THE PROJECT
    nob_cmd_append(&cmd, "cc", "-Wall","-Wextra", "-ggdb", "-Ideps/quickui/src","-o", exe, "./deps/quickui/src/quickui.c", "./src/msode.c");
    nob_cmd_append(&cmd, "./src/hotreload-linux.c");
    nob_cmd_append(&cmd, "-Ideps/raylib/include");
    nob_cmd_append(&cmd, "-Ldeps/raylib/lib", "-lm", "-lraylib");
    #ifdef ENABLE_HOT_RELOAD
    nob_cmd_append(&cmd, "-ldl");
    nob_cmd_append(&cmd, "-Lbuild/");
    #else
    nob_cmd_append(&cmd, "-Lbuild/");
    nob_cmd_append(&cmd, "-l:libplug.a");
    #endif
    if (!nob_cmd_run(&cmd)) return 1;

    return 0;


}
