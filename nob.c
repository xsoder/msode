#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

#define BUILD "build"

static const char *gdb = "-ggdb";

#ifdef __linux__
    static const char *compiler = "gcc";
    static const char *ar = "ar";
    static const char *ar_flag="-r";
    static const char *raylib = "-Ideps/raylib/include";
    static const char *hot_reload = "./src/hotreload-linux.c";
    static const char *hot_obj = "build/hotreload-linux.o";
#else
    static const char *compiler = "x86_64-w64-mingw32-gcc";
    static const char *ar = "x86_64-w64-mingw32-ar";
    static const char *ar_flag="rsc";
    static const char *raylib = "-Ideps/raylib-5.5_win64_mingw-w64/include";
    static const char *hot_reload = "./src/hotreload-windows.c";
    static const char *hot_obj = "build/hotreload-windows.o";
#endif

static const char *obj = "build/libplug.o";
static const char *lib = "build/libplug.a";
static const char *exe = "build/msode";
static const char *plug_lib =  "-l:libplug.a";
static const char *ray_lib =  "-l:libraylib.a";

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    #if PLATFORM_LINUX
    nob_cmd_append(&cmd, "bash", "configure");
    if (!nob_cmd_run(&cmd)) return 1;
    #endif
    
    if(!nob_mkdir_if_not_exists(BUILD)) nob_log(NOB_ERROR, "Could not create directory");

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Ideps/quickui/src", raylib);
    nob_cmd_append(&cmd, "-c", "./deps/quickui/src/quickui.c", "-o", "build/quickui.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Ideps/quickui/src", raylib);
    nob_cmd_append(&cmd, "-c", "./src/plug.c", "-o", "build/plug.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, ar, ar_flag, "build/libplug.a", "build/quickui.o", "build/plug.o");
    if (!nob_cmd_run(&cmd)) return 1;
    
    #ifdef _WIN32
    nob_cmd_append(&cmd, "x86_64-w64-mingw32-ranlib", "build/libplug.a");
    if (!nob_cmd_run(&cmd)) return 1;
    #endif //_WIN32

    // BUILDING MAIN EXECUTABLE
    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Ideps/quickui/src", raylib);
    nob_cmd_append(&cmd, "-c", "./src/msode.c", "-o", "build/msode.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Ideps/quickui/src", raylib);

    nob_cmd_append(&cmd, "-c", hot_reload, "-o", hot_obj);
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
        
    #if __linux__
    nob_cmd_append(&cmd, "-O2",
        "-o",
        exe,
        "build/msode.o",
        "build/hotreload-linux.o",
        "-Lbuild",
        "-Ldeps/raylib/lib",
        plug_lib,
        ray_lib,
        "-lm");
    #else
    nob_cmd_append(&cmd, "-static", "-O2",
        "-o",
        exe,
        "build/msode.o",
        "build/hotreload-windows.o",
        "-Lbuild", "-Wl,--whole-archive",
        "-l:libplug.a",
        "-Wl,--no-whole-archive",
        "-Ldeps/raylib-5.5_win64_mingw-w64/lib",
        "-lraylib",
        "-lm",
        "-lwinmm",
        "-lgdi32",
        "-lopengl32",
        "-luser32",
        "-lkernel32",
        "-lcomdlg32",
        "-lglu32",
        "-Wl,-subsystem,console");
    #endif    

    if (!nob_cmd_run(&cmd)) return 1;

    return 0;
}
