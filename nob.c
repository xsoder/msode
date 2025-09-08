#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

#define BUILD "build"

static const char *gdb = "-ggdb";

#define PLATFORM_LINUX 1
#define STATIC_BUILD 1

#if PLATFORM_LINUX
static const char *compiler = "gcc";
#else
static const char *compiler = "x86_64-w64-mingw32-gcc";
#endif



int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    #if PLATFORM_LINUX
    nob_cmd_append(&cmd, "bash", "configure");
    if (!nob_cmd_run(&cmd)) return 1;
    #endif
    
    if(!nob_mkdir_if_not_exists(BUILD)) 
    nob_log(NOB_ERROR, "Could not create directory");

    #if ENABLE_HOT_RELOAD
    static const char *lib = "build/libplug.so";
    #else
    static const char *obj = "build/libplug.o";
    static const char *lib = "build/libplug.a";
    #endif //CONFIG_H
    static const char *exe = "build/msode";

    #if PLATFORM_LINUX
    static const char *ar = "ar";
    static const char *ar_flag="-r";
    #else
    static const char *ar = "x86_64-w64-mingw32-ar";
    static const char *ar_flag="rsc";
    #endif

    // BUILDING LIB-PLUG
    #if PLATFORM_LINUX
    static const char *raylib = "-Ideps/raylib/include";
    #else
    static const char *raylib = "-Ideps/raylib-5.5_win64_mingw-w64/include";
    #endif

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Isrc_build", raylib);
    nob_cmd_append(&cmd, "-c", "src_build/quickui.c", "-o", "build/quickui.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
    nob_cmd_append(&cmd, "-Isrc_build", raylib);
    nob_cmd_append(&cmd, "-c", "./src/plug.c", "-o", "build/plug.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, ar, ar_flag, "build/libplug.a", "build/quickui.o", "build/plug.o");
    if (!nob_cmd_run(&cmd)) return 1;
    
    #if !PLATFORM_LINUX
    nob_cmd_append(&cmd, "x86_64-w64-mingw32-ranlib", "build/libplug.a");
    if (!nob_cmd_run(&cmd)) return 1;
    #endif

    // BUILDING
    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
        
    nob_cmd_append(&cmd, "-Isrc_build", raylib);
    nob_cmd_append(&cmd, "-c", "./src/msode.c", "-o", "build/msode.o");
    if (!nob_cmd_run(&cmd)) return 1;

    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
        
    nob_cmd_append(&cmd, "-Isrc_build", raylib);
    #if PLATFORM_LINUX
    const char *hot_reload = "./src/hotreload-linux.c";
    static const char *hot_obj = "build/hotreload-linux.o";
    #else 
    const char *hot_reload = "./src/hotreload-windows.c";
    static const char *hot_obj = "build/hotreload-windows.o";
    #endif

    nob_cmd_append(&cmd, "-c", hot_reload, "-o", hot_obj);
    if (!nob_cmd_run(&cmd)) return 1;

    // link
    #if STATIC_BUILD
    const char *plug_lib =  "-l:libplug.a";
    const char *ray_lib =  "-l:libraylib.a";
    #else
    const char *ray_lib =  "-lraylib";
    const char *plug_lib =  "-lplug";
    #endif
    
    nob_cmd_append(&cmd, compiler,
        gdb,
        "-Wall",
        "-Wextra",
    );
        
    #if PLATFORM_LINUX
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
