/*
host_launcher.c - minimal launcher for libxash.so (dedicated server)
Loads the engine library and calls Host_Main
*/

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>

typedef void (*pfnInit)(int argc, char **argv, const char *progname, int bChangeGame, const char *pChangeGame);
typedef void (*pfnShutdown)(void);

int main(int argc, char **argv)
{
	void *hEngine;
	pfnInit Host_Main = NULL;
	pfnShutdown Host_Shutdown = NULL;
	char libpath[512];
	char exePath[512];
	char *exeDir;
	char errmsg[512];
	struct stat st;
	int ret;

	fprintf(stderr, "Xash3D Dedicated Server Launcher\n");
	fprintf(stderr, "==================================\n");
	fflush(stderr);

	/* Get the directory of the executable */
	ret = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
	if (ret > 0) {
		exePath[ret] = '\0';
		fprintf(stderr, "[*] Executable path: %s\n", exePath);
	} else {
		strncpy(exePath, argv[0], sizeof(exePath) - 1);
		exePath[sizeof(exePath) - 1] = '\0';
		fprintf(stderr, "[*] Using argv[0]: %s\n", exePath);
	}
	fflush(stderr);

	exeDir = dirname(exePath);
	fprintf(stderr, "[*] Executable dir: %s\n", exeDir);
	fflush(stderr);

	/* Try to load libxash.so from the same directory as the executable */
	snprintf(libpath, sizeof(libpath), "%s/libxash.so", exeDir);
	fprintf(stderr, "[*] Trying to load: %s\n", libpath);
	fflush(stderr);

	/* Check if file exists */
	if (stat(libpath, &st) != 0) {
		fprintf(stderr, "[-] File not found at %s\n", libpath);
		fprintf(stderr, "[-] Trying system library path...\n");
		fflush(stderr);
		strcpy(libpath, "libxash.so");
	} else {
		fprintf(stderr, "[+] File exists (size: %lld bytes)\n", (long long)st.st_size);
		fflush(stderr);
	}

	/* Load the engine library */
	fprintf(stderr, "[*] Calling dlopen(\"%s\", RTLD_NOW)...\n", libpath);
	fflush(stderr);

	hEngine = dlopen(libpath, RTLD_NOW);
	if (!hEngine) {
		snprintf(errmsg, sizeof(errmsg), "[-] dlopen failed: %s\n", dlerror());
		fprintf(stderr, "%s", errmsg);
		fflush(stderr);
		return 1;
	}

	fprintf(stderr, "[+] Library loaded successfully at %p\n", hEngine);
	fflush(stderr);

	/* Get Host_Main entry point */
	fprintf(stderr, "[*] Looking for Host_Main export...\n");
	fflush(stderr);

	Host_Main = (pfnInit)dlsym(hEngine, "Host_Main");
	if (!Host_Main) {
		snprintf(errmsg, sizeof(errmsg), "[-] dlsym(Host_Main) failed: %s\n", dlerror());
		fprintf(stderr, "%s", errmsg);
		fflush(stderr);
		dlclose(hEngine);
		return 1;
	}

	fprintf(stderr, "[+] Found Host_Main at %p\n", (void*)Host_Main);
	fprintf(stderr, "[*] Starting engine with progname=xash...\n");
	fflush(stderr);

	/* Run the engine with correct parameters */
	Host_Main(argc, argv, "valve", 0, NULL);

	fprintf(stderr, "[*] Engine shutdown, cleaning up...\n");
	fflush(stderr);

	/* Cleanup */
	Host_Shutdown = (pfnShutdown)dlsym(hEngine, "Host_Shutdown");
	if (Host_Shutdown) {
		fprintf(stderr, "[*] Calling Host_Shutdown...\n");
		fflush(stderr);
		Host_Shutdown();
	}

	dlclose(hEngine);
	fprintf(stderr, "[+] Done\n");
	return 0;
}