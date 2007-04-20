/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright � 2006 Marek Szyprowski
 * Copyright � 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "fat_fs.h"
#include "fat_protos.h"

/* find the named file in the directory referenced by dirlock, and delete it.
 * if the file is a directory, it will only be deleted if its empty */
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen) {
    LONG err;
    BPTR b;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;
    UBYTE checksum;
    ULONG order, cluster;

    D(bug("[fat] deleting file '%.*s' in directory at cluster %ld\n", namelen, name, dirlock->ioh.first_cluster));

    /* can't delete the dot entries */
    if (name[0] == '.' && (namelen == 1 || (name[1] == '.' && namelen == 2)))
        return ERROR_DELETE_PROTECTED;

    /* obtain a lock on the file. we need an exclusive lock as we don't want
     * to delete the file if its in use */
    if ((err = TryLockObj(dirlock, name, namelen, EXCLUSIVE_LOCK, &b)) != 0) {
        D(bug("[fat] couldn't obtain exclusive lock on named file\n"));
        return err;
    }
    lock = BADDR(b);

    /* if its a directory, we have to make sure its empty */
    if (lock->attr & ATTR_DIRECTORY) {
        D(bug("[fat] file is a directory, making sure its empty\n"));

        if ((err = InitDirHandle(lock->ioh.sb, lock->ioh.first_cluster, &dh)) != 0) {
            FreeLock(lock);
            return err;
        }

        /* loop over the entries, starting from entry 2 (the first real
         * entry). skipping unused ones, we look for the end-of-directory
         * marker. if we find it, the directory is empty. if we find a real
         * name, its in use */
        de.index = 1;
        while ((err = GetDirEntry(&dh, de.index+1, &de)) == 0) {
            /* skip unused entries */
            if (de.e.entry.name[0] == 0xe5)
                continue;

            /* end of directory, its empty */
            if (de.e.entry.name[0] == 0x00)
                break;

            /* otherwise the directory is still in use */
            ReleaseDirHandle(&dh);
            FreeLock(lock);
            return ERROR_DIRECTORY_NOT_EMPTY;
        }

        ReleaseDirHandle(&dh);
    }

    /* open the containing directory */
    if ((err = InitDirHandle(dirlock->ioh.sb, dirlock->ioh.first_cluster, &dh)) != 0) {
        FreeLock(lock);
        return err;
    }

    /* get the entry for the file */
    GetDirEntry(&dh, lock->dir_entry, &de);

    /* calculate the short name checksum before we trample on the name */
    CALC_SHORT_NAME_CHECKSUM(de.e.entry.name, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* mark the short entry free */
    de.e.entry.name[0] = 0xe5;
    UpdateDirEntry(&de);

    /* now we loop over the previous entries, looking for matching long name
     * entries and killing them */
    order = 1;
    while ((err = GetDirEntry(&dh, de.index-1, &de)) == 0) {

        /* see if this is a matching long name entry. if its not, we're done */
        if (!((de.e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ||
            (de.e.long_entry.order & ~0x40) != order ||
            de.e.long_entry.checksum != checksum)

            break;

        /* kill it */
        de.e.entry.name[0] = 0xe5;
        UpdateDirEntry(&de);

        order++;
    }

    /* directory entries are free */
    ReleaseDirHandle(&dh);

    /* now free the clusters the file was using */
    cluster = lock->ioh.first_cluster;
    while (cluster >= 0 && cluster < glob->sb->eoc_mark) {
        ULONG next_cluster = GET_NEXT_CLUSTER(glob->sb, cluster);
        SET_NEXT_CLUSTER(glob->sb, cluster, 0);
        cluster = next_cluster;
    }

    /* this lock is now completely meaningless */
    FreeLock(lock);

    return 0;
}
