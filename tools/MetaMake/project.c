/* MetaMake - A Make extension
   Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//#define DEBUG_PROJECT

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif

#include "project.h"
#include "var.h"
#include "mem.h"
#include "dep.h"
#include "mmake.h"

#if defined(DEBUG_PROJECT)
#   define debug(a) a
#else
#   define debug(v)
#endif

#if defined(POSIX_EXEC)
# if defined(__APPLE__)
#   include <sys/syslimits.h>
# else
#   include <limits.h>
# endif
#endif

struct List projects;
static struct Project * defaultprj = NULL;

static void
readvars (struct Project * prj)
{
    struct List deps;
    struct Node * node, * next;
    struct Dep * dep;

    debug(printf("MMAKE:project.c->readvars(Project @ 0x%p)\n", prj));

    if (!prj->readvars)
        return;

    prj->readvars = 0;

    printf ("[MMAKE] Read vars...\n");

    setvar (&prj->vars, "TOP", prj->buildtop);
    setvar (&prj->vars, "SRCDIR", prj->srctop);
    setvar (&prj->vars, "CURDIR", "");

    ForeachNode(&prj->globalvarfiles, node)
    {
        char * fn;
        FILE * fh;
        char line[256];
        char * name, * value, * ptr;

        fn = xstrdup (substvars (&prj->vars, node->name));
        fh = fopen (fn, "r");

        /* if the file doesn't exist execute prj->genglobalvarfile */
        if (!fh && prj->genglobalvarfile)
        {
            char * gen = xstrdup (substvars (&prj->vars, prj->genglobalvarfile));

            printf ("[MMAKE] Generating %s...\n", fn);

            if (!execute (prj, gen,
#if defined(POSIX_EXEC)
                NULL, NULL,
#endif
                "-", "-", ""))
            {
                error ("Error while creating \"%s\" with \"%s\"", fn, gen);
                exit (10);
            }
            else
                fh = fopen (fn, "r");

            xfree (gen);
        }

        if (!fh)
        {
            error ("readvars():fopen(): Opening \"%s\" for reading", fn);
            return;
        }

        xfree (fn);

        while (fgets (line, sizeof(line), fh))
        {
            if (*line == '\n' || *line == '#') continue;
            line[strlen(line)-1] = 0;

            ptr = line;
            while (isspace (*ptr)) ptr++;
            name = ptr;
            while (*ptr && !isspace(*ptr) && *ptr != ':' && *ptr != '=')
                ptr ++;

            if (*ptr)
                *ptr++ = 0;

            while (isspace(*ptr) || *ptr == ':' || *ptr == '=')
                ptr ++;

            value = ptr;

            while (*ptr && *ptr != '#')
                ptr ++;

            *ptr = 0;

            if (debug)
                printf ("[MMAKE] %s=%s\n", name, substvars (&prj->vars, value));

            setvar (&prj->vars, name, substvars (&prj->vars, value));
        }

        fclose (fh);
    }

    /* handle prj->genmakefiledeps */
    NewList(&deps);
    /* algorithm has changed from:
     * copying nodes to deps list and then generating dep nodes
     * which will be put back to genmakefiledeps list
     * to:
     * generating new dep nodes and putting them into the deps list,
     * then copy them back into genmakefiledeps list
     * reason for change:
     * in recent versions of gcc (>= 4.6) the first loop over all
     * genmakefiledeps list nodes was optimized away, so deps list
     * was always empty and the second loop never did anything */
    ForeachNodeSafe (&prj->genmakefiledeps, node, next)
    {
        Remove (node);
        dep = newdepnode (substvars (&prj->vars, node->name));
        AddTail (&deps, dep);
        xfree (node->name);
        xfree (node);
    }

    ForeachNodeSafe (&deps, node, next)
    {
        Remove (node);
        AddTail (&prj->genmakefiledeps, node);
    }

    if (debug)
    {
        printf ("[MMAKE] project %s.genmfdeps=\n", prj->node.name);
        printlist (&prj->genmakefiledeps);
    }

    if (debug)
    {
        printf ("[MMAKE] project %s.vars=", prj->node.name);
        printvarlist (&prj->vars);
    }
}

static struct Project *
initproject (char * name)
{
    struct Project * prj = new (struct Project);

    memset (prj, 0, sizeof(struct Project));

    debug(printf("MMAKE:project.c->initproject('%s')\n", name));
    debug(printf("MMAKE:project.c->initproject: Project node @ 0x%p\n", prj));

    if (!defaultprj)
    {
        prj->maketool = xstrdup ("make \"TOP=$(TOP)\" \"SRCDIR=$(SRCDIR)\" \"CURDIR=$(CURDIR)\"");
        prj->defaultmakefilename = xstrdup ("Makefile");
        prj->srctop = mm_srcdir;
        prj->buildtop = mm_builddir;
        prj->defaulttarget = xstrdup ("all");
        prj->genmakefilescript = NULL;
        prj->genglobalvarfile = NULL;
    }
    else
    {
        prj->maketool = xstrdup (defaultprj->maketool);
        prj->defaultmakefilename = xstrdup (defaultprj->defaultmakefilename);
        prj->srctop = xstrdup (defaultprj->srctop);
        prj->buildtop = xstrdup (defaultprj->buildtop);
        prj->defaulttarget = xstrdup (defaultprj->defaulttarget);
        SETSTR (prj->genmakefilescript, defaultprj->genmakefilescript);
        SETSTR (prj->genglobalvarfile, defaultprj->genglobalvarfile);
    }

    prj->node.name = xstrdup (name);

    prj->readvars = 1;

    NewList(&prj->globalvarfiles);
    NewList(&prj->genmakefiledeps);
    NewList(&prj->ignoredirs);
    NewList(&prj->vars);
    NewList(&prj->extramakefiles);

    return prj;
}

static void
freeproject (struct Project * prj)
{
    assert (prj);

    cfree (prj->node.name);
    cfree (prj->maketool);
    cfree (prj->defaultmakefilename);
    if (prj->srctop != mm_srcdir)
        cfree (prj->srctop);
    if (prj->buildtop != mm_builddir)
        cfree (prj->buildtop);
    cfree (prj->defaulttarget);
    cfree (prj->genmakefilescript);
    cfree (prj->genglobalvarfile);

    if (prj->cache)
        closecache (prj->cache);

    freelist(&prj->globalvarfiles);
    freelist (&prj->genmakefiledeps);
    freelist (&prj->ignoredirs);
    freevarlist (&prj->vars);
    freelist (&prj->extramakefiles);

    xfree (prj);
}

static void
callmake (struct Project * prj, const char * tname, struct Makefile * makefile)
{
    static char buffer[4096];
    const char * path = buildpath (makefile->dir);
    int t;

    debug(printf("MMAKE:project.c->callmake()\n"));

    if (makefile->generated)
        ASSERT(chdir (prj->buildtop) == 0);
    else
        ASSERT(chdir (prj->srctop) == 0);
    if (path[0] != 0)
        ASSERT(chdir (path) == 0);

    setvar (&prj->vars, "CURDIR", path);
    setvar (&prj->vars, "TARGET", tname);

    buffer[0] = '\0';

    if (quiet)
        strcat (buffer, "-s ");

    for (t=0; t<mflagc; t++)
    {
        strcat (buffer, mflags[t]);
        strcat (buffer, " ");
    }

    if (strcmp (makefile->node.name, "Makefile")!=0 && strcmp (makefile->node.name, "makefile")!=0)
    {
        strcat (buffer, "--file=");
        strcat (buffer, makefile->node.name);
        strcat (buffer, " ");
    }

    strcat (buffer, tname);

#if !defined(POSIX_EXEC)
    if (!quiet)
    {
        if (path && path[0] != 0)
            printf ("[MMAKE] Making %s in %s\n", tname, path);
        else
            printf ("[MMAKE] >> Making %s\n", tname);
    }
#endif

    if (!execute (prj, prj->maketool,
#if defined(POSIX_EXEC)
        tname, path,
#endif
        "-", "-", buffer))
    {
        error ("Error while running make in %s", path);
        exit (10);
    }
}


void
initprojects (void)
{
    char * optionfile;
    char * home;
    char line[256];
    FILE * optfh = NULL;
    struct Project * project;

    debug(printf("MMAKE:project.c->initprojects()\n"));

    NewList(&projects);
    defaultprj = project = initproject ("default");
    AddTail(&projects, project);


    /* Try "$MMAKE_CONFIG" */
    if ((optionfile = getenv ("MMAKE_CONFIG")))
        optfh = fopen (optionfile, "r");

    /* Try "$HOME/.mmake.config" */
    if (!optfh)
    {
        if ((home = getenv("HOME")))
        {
            optionfile = xmalloc (strlen(home) + sizeof("/.mmake.config") + 1);
            sprintf (optionfile, "%s/.mmake.config", home);
            optfh = fopen (optionfile, "r");
            free (optionfile);
        }
    }

    /* Try with $CWD/.mmake.config" */
    if (!optfh)
        optfh = fopen (".mmake.config", "r");

    /* Try with "$CWD/mmake.config */
    if (!optfh)
        optfh = fopen ("mmake.config", "r");

    /* Give up */
    if (!optfh)
    {
        fprintf (stderr,
                "[MMAKE] Please set the HOME or MMAKE_CONFIG env var (with setenv or export)\n"
                );
        error ("Opening mmake.config for reading");
        exit (10);
    }

    while (fgets (line, sizeof(line), optfh))
    {
        if (*line == '\n' || *line == '#') continue;
        line[strlen(line)-1] = 0;

        if (*line == '[') /* look for project name */
        {
            char * name, * ptr;

            name = ptr = line+1;
            while (*ptr && *ptr != ']')
                ptr ++;

            *ptr = 0;

            debug(printf("MMAKE:project.c->initprojects: Adding '%s' from MMAKE_CONFIG\n", name));

            project = initproject (name);

            AddTail(&projects,project);
        }
        else
        {
            char * cmd, * args, * ptr;

            cmd = line;
            while (isspace (*cmd))
                cmd ++;

            args = cmd;
            while (*args && !isspace(*args))
            {
                *args = tolower (*args);
                args ++;
            }
            if (*args)
                *args++ = 0;
            while (isspace (*args))
                args ++;

            ptr = args;

            while (*ptr && *ptr != '\n')
                ptr ++;

            *ptr = 0;

            if (!strcmp (cmd, "add"))
            {
                struct Node * n;
                n = newnode(args);
                AddTail(&project->extramakefiles, n);
            }
            else if (!strcmp (cmd, "ignoredir"))
            {
                struct Node * n;
                n = newnode(args);
                AddTail(&project->ignoredirs, n);
            }
            else if (!strcmp (cmd, "defaultmakefilename"))
            {
                SETSTR(project->defaultmakefilename,args);
            }
            else if (!strcmp (cmd, "top"))
            {
                SETSTR(project->srctop,args);
            }
            else if (!strcmp (cmd, "defaulttarget"))
            {
                SETSTR(project->defaulttarget,args);
            }
            else if (!strcmp (cmd, "genmakefilescript"))
            {
                SETSTR(project->genmakefilescript,args);
            }
            else if (!strcmp (cmd, "genmakefiledeps"))
            {
                struct Node * dep;
                int depc, t;
                char ** deps = getargs (args, &depc, NULL);

                debug(printf("MMAKE/project.c: genmakefiledeps depc=%d\n", depc));

                for (t=0; t<depc; t++)
                {
                    dep = addnodeonce (&project->genmakefiledeps, deps[t]);
                }
            }
            else if (!strcmp (cmd, "globalvarfile"))
            {
                struct Node *n = newnode(args);

                if (n)
                    AddTail(&project->globalvarfiles, n);
            }
            else if (!strcmp (cmd, "genglobalvarfile"))
            {
                SETSTR(project->genglobalvarfile,args);
            }
            else if (!strcmp (cmd, "maketool"))
            {
                SETSTR(project->maketool,args);
            }
            else
            {
                setvar(&project->vars, cmd, args);
            }
        }
    }

    fclose (optfh);

    /* Clean up memory from getargs */
    getargs (NULL, NULL, NULL);

    if (debug)
    {
        printf ("[MMAKE] known projects: ");
        printlist (&projects);
    }
}

void
expungeprojects (void)
{
    struct Project *prj, *next;

    ForeachNodeSafe (&projects, prj, next)
    {
        Remove (prj);
        freeproject (prj);
    }
}

struct Project *
findproject (const char * pname)
{
    return FindNode (&projects, pname);
}

struct Project *
getfirstproject (void)
{
    struct Project * prj = GetHead (&projects);

    if (prj && prj == defaultprj)
        prj = GetNext (prj);

    return prj;
}

int
execute (struct Project * prj,
        const char * cmd,
#if defined(POSIX_EXEC)
        const char * tname,
        const char * path,
#endif
        const char * in,
        const char * out,
        const char * args)
{
    char buffer[4096];
#if defined(POSIX_EXEC)
    char cmdout[PATH_MAX];
    FILE *cmdpipe;
#endif
    char * cmdstr;
    int rc;

    debug(printf("MMAKE:project.c->execute(cmd '%s')\n", cmd));

    strcpy (buffer, cmd);
    strcat (buffer, " ");

    if (strcmp (in, "-"))
    {
        strcat (buffer, "<");
        strcat (buffer, in);
        strcat (buffer, " ");
    }

    if (strcmp (out, "-"))
    {
        strcat (buffer, ">");
        strcat (buffer, out);
        strcat (buffer, " ");
    }

    strcat (buffer, args);

    cmdstr = substvars (&prj->vars, buffer);

    debug(printf("MMAKE:project.c->execute: parsed cmd '%s'\n", buffer));

    if (verbose)
        printf ("[MMAKE] Executing %s...\n", cmdstr);

#if !defined(POSIX_EXEC)
    rc = system (cmdstr);
#else
    cmdpipe = popen(cmdstr, "r");
    if (cmdpipe != NULL) {
        int msg = 0;
        while (fgets(cmdout, PATH_MAX, cmdpipe) != NULL) {
            if (strstr(cmdout, ": Nothing to be done for") != NULL)
                continue;
            if (!msg)
            {
                if (!quiet)
                {
                    if (tname && path && path[0] != 0)
                        printf ("[MMAKE] Making %s in %s\n", tname, path);
                    else if (tname)
                        printf ("[MMAKE] >> Making %s\n", tname);
                }
                msg = 1;
            }
            printf("%s", cmdout);
        }
        rc = pclose(cmdpipe);
    }
    else
        rc = 2;
#endif

    if (rc)
    {
        printf ("[MMAKE] %s failed: %d\n", cmdstr, rc);
    }

    return !rc;
}

#define VOUT(x)

void
maketarget (FILE * deplogfh, struct Project * prj, char * tname, unsigned int depth, unsigned int offset)
{
    struct Target * target, * subtarget;
    struct Node * node;
    struct MakefileRef * mfref;
    struct MakefileTarget * mftarget;
    struct List deps;
    unsigned int updatecnt = 0, targetcnt = 0;

    NewList (&deps);

    ASSERT(chdir (prj->srctop) == 0);

    readvars (prj);

    if (!prj->cache)
        prj->cache = activatecache (prj);

    if (!*tname)
        tname = prj->defaulttarget;

    target = FindNode (&prj->cache->targets, tname);

    if (!target)
    {
        if ((!strcmp(mm_envtarget, tname)) || (verbose))
            printf ("[MMAKE][%u]  Nothing known about target %s in project %s\n", depth, tname, prj->node.name);
        if (mm_faillogfh)
        {
            fputs(tname, mm_faillogfh);
            fputs("\n", mm_faillogfh);
        }
        return;
    }

    target->updated = 1;

    ForeachNode (&target->makefiles, mfref)
    {
        mftarget = FindNode (&mfref->makefile->targets, tname);

        ForeachNode (&mftarget->deps, node)
            addnodeonce (&deps, node->name);
    }

#if (0)
    if (deplogfh)
        fprintf(deplogfh, "[MMAKE][%u]  Subtargets -:\n", depth);
#endif
    ForeachNode (&deps, node)
    {
        subtarget = FindNode (&prj->cache->targets, node->name);

        if (subtarget)
        {
            VOUT(printf ("[MMAKE][%u]          '%s'", depth, node->name);)
            if (!subtarget->updated)
            {
                VOUT(printf (" (needs update)");)
                ForeachNode (&subtarget->makefiles, mfref)
                {
                    //printdirnodemftarget (mfref->makefile->dir);
                    VOUT(printf ("\n[MMAKE][%u]            -> %s", depth, buildpath(mfref->makefile->dir));)
                    //printdirnode (mfref->makefile->dir, );
                }
                updatecnt++;
            }
            VOUT(printf ("\n");)
        }
    }

    if (updatecnt)
    {
        int first = 0;

#if (0)
        if (deplogfh)
        {
            int tmpcnt;
            for (tmpcnt = 0; tmpcnt < offset; tmpcnt ++)
                fputs(" ", deplogfh);
            fprintf(deplogfh, " * %u dependancies to satisfy\n", updatecnt);
        }
#endif
        ForeachNode (&deps, node)
        {
            subtarget = FindNode (&prj->cache->targets, node->name);

            if (!subtarget)
            {
                if ((!strcmp(mm_envtarget, node->name)) || (verbose))
                    printf ("[MMAKE][%u] Nothing known about subtarget %s in project %s\n", depth, node->name, prj->node.name);
                if (mm_faillogfh)
                {
                    fputs(node->name, mm_faillogfh);
                    fputs("\n", mm_faillogfh);
                }
            }
            else if (!subtarget->updated)
            {
                int inclen;
                VOUT(printf ("[MMAKE][%u] %s subtarget %s\n", depth, tname, node->name);)
                if (first == 0)
                {
                    first = 1;
                    if (depth == 0)
                    {
                        if (deplogfh)
                            fprintf(deplogfh, "[MMAKE][%u] >> %s", depth, node->name);
                        inclen = strlen(node->name) + 14;
                    }
                    else
                    {
                        if (deplogfh)
                            fprintf(deplogfh, " > [%u] %s", depth, node->name);
                        inclen = strlen(node->name) + 7;
                    }
                }
                else
                {
                    if (deplogfh)
                    {
                        int tmpcnt;
                        if ((depth == 0) || (offset == 0))
                            offset = 7;
                        for (tmpcnt = 0; tmpcnt < offset; tmpcnt ++)
                            fputs(" ", deplogfh);
                        if (depth == 0)
                            fprintf(deplogfh,  "[%u]  > %s", depth, node->name);
                        else
                            fprintf(deplogfh,  " > [%u] %s", depth, node->name);
                    }
                    inclen = strlen(node->name) + 7;
                }
                maketarget (deplogfh, prj, node->name, depth + 1, offset + inclen);
            }
        }
    }
    if (deplogfh)
         fputs("\n", deplogfh);

    freelist (&deps);

    ForeachNode (&target->makefiles, mfref)
    {
        if (!mfref->virtualtarget)
        {
            targetcnt++;
        }
    }

    if (targetcnt)
    {
        if (deplogfh)
        {
            int tmpcnt;
            for (tmpcnt = 0; tmpcnt < offset; tmpcnt ++)
                fputs(" ", deplogfh);
            fprintf(deplogfh, " - %u makefile(s) for target %s\n", targetcnt, tname);
        }
        VOUT(printf ("[MMAKE][%u]  Building %s.%s\n", depth, prj->node.name, tname);)

        ForeachNode (&target->makefiles, mfref)
        {
            if (!mfref->virtualtarget)
            {
                callmake (prj, tname, mfref->makefile);
            }
        }
    }
}
