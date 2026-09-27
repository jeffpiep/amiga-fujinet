/*
 * disk_test.c - fujinet-disk.device as a DOS volume (DN0:) on KS 1.3
 *
 * Two modes, run from the startup-sequence around C:Mount:
 *
 *   disk_test mount <unit> <uri>   put media in a unit (FUJINET_DISK_CMD_MOUNT)
 *   disk_test check                read DN0: through DOS, then report
 *
 * The driver's own fujinet-mount needs CreateNewProcTags (2.0+), so the
 * mount step is here, using Exec only.
 *
 * `check` verifies the volume name, the root directory and two files'
 * contents. emu/run.sh can only see the server log, so the verdict goes out
 * through the device under test: a mount request for host:/disk_test-PASS
 * (or -FAIL) on unit 1, which fujinet-nio logs by URI. Neither file exists;
 * the request fails harmlessly. A Guru sends no verdict.
 *
 * Requires: fujinet-disk.device resident on top of fujinet-nio.device,
 * DEVS:MountList with a 1.3-correct DN0 entry, C:Mount
 * (see contracts/amiga-adf-bootstrap.md, "Disk device (DN0:) on KS 1.3")
 * Compiler: m68k-amigaos-gcc (amiga-gcc), -mcrt=nix13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "fujinet_disk_device.h"

/* Must match the files the Makefile writes into the test image. */
#define VOLUME_NAME  "DISKTEST"
#define FILE_TEXT    "Hello from DN0 on KS 1.3\n"
#define ROOT_ENTRIES 2              /* hello.txt, sub */
#define VERDICT_UNIT 1

/* Send FUJINET_DISK_CMD_MOUNT for uri to unit; returns the io_Error. */
static int disk_mount(ULONG unit, const char *uri, int verbose)
{
    struct MsgPort *port;
    struct IOExtTD *io;
    int err = -1;

    port = CreatePort(NULL, 0);
    if (port == NULL) return err;
    io = (struct IOExtTD *)CreateExtIO(port, sizeof(*io));
    if (io == NULL) {
        DeletePort(port);
        return err;
    }
    if (OpenDevice((STRPTR)FUJINET_DISK_DEVICE_NAME, unit,
                   (struct IORequest *)io, 0) != 0) {
        printf("disk_test: OpenDevice unit %lu failed (%d)\n",
               (unsigned long)unit, (int)io->iotd_Req.io_Error);
    } else {
        io->iotd_Req.io_Command = FUJINET_DISK_CMD_MOUNT;
        io->iotd_Req.io_Data = (APTR)uri;
        io->iotd_Req.io_Length = strlen(uri) + 1;
        DoIO((struct IORequest *)io);
        err = io->iotd_Req.io_Error;
        if (verbose)
            printf("disk_test: mount unit %lu %s -> %d\n",
                   (unsigned long)unit, uri, err);
        CloseDevice((struct IORequest *)io);
    }
    DeleteExtIO((struct IORequest *)io);
    DeletePort(port);
    return err;
}

/* 1 if path holds exactly FILE_TEXT. */
static int file_matches(const char *path)
{
    char buf[64];
    LONG n;
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);

    if (fh == 0) {
        printf("disk_test: cannot open %s\n", path);
        return 0;
    }
    n = Read(fh, buf, sizeof(buf) - 1);
    Close(fh);
    if (n != (LONG)strlen(FILE_TEXT) || memcmp(buf, FILE_TEXT, n) != 0) {
        printf("disk_test: %s: wrong contents (%ld bytes)\n", path, (long)n);
        return 0;
    }
    printf("disk_test: %s OK\n", path);
    return 1;
}

/* 1 if DN0: is the expected volume with the expected root. */
static int volume_matches(void)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    int entries = 0;
    int ok = 0;

    /* FileInfoBlock must be longword aligned; AllocDosObject is 2.0+. */
    fib = AllocMem(sizeof(*fib), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib == NULL) return 0;
    lock = Lock((STRPTR)"DN0:", ACCESS_READ);
    if (lock == 0) {
        printf("disk_test: cannot lock DN0: (%ld)\n", (long)IoErr());
    } else {
        if (Examine(lock, fib)) {
            printf("disk_test: volume %s\n", fib->fib_FileName);
            ok = strcmp(fib->fib_FileName, VOLUME_NAME) == 0;
            while (ExNext(lock, fib)) ++entries;
            printf("disk_test: %d root entries\n", entries);
            if (entries != ROOT_ENTRIES) ok = 0;
        }
        UnLock(lock);
    }
    FreeMem(fib, sizeof(*fib));
    return ok;
}

int main(int argc, char *argv[])
{
    int pass;

    if (argc == 4 && strcmp(argv[1], "mount") == 0)
        return disk_mount(strtoul(argv[2], NULL, 0), argv[3], 1) == 0 ? 0 : 10;

    if (argc != 2 || strcmp(argv[1], "check") != 0) {
        printf("usage: disk_test mount <unit> <uri> | disk_test check\n");
        return 5;
    }

    pass = volume_matches();
    pass = file_matches("DN0:hello.txt") && pass;
    pass = file_matches("DN0:sub/again.txt") && pass;
    printf("disk_test: %s\n", pass ? "PASS" : "FAIL");
    (void)disk_mount(VERDICT_UNIT,
                     pass ? "host:/disk_test-PASS" : "host:/disk_test-FAIL", 0);
    return pass ? 0 : 10;
}
