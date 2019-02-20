#include <glib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

GMainLoop *loop;
guint poll_id = -1;

struct dirpaths
{
    std::string chardev;
    std::string scriptsdir;
};

void exec_scripts(const std::string &path)
{
    DIR *dir;
    struct dirent *entry;
    std::string fname, script;
    bool found = false;

    if ((dir = opendir(path.c_str())) == NULL)
    {
        fprintf(stderr, "couldn't open %s\n", path.c_str());
        return;
    }

    do
    {
        if ((entry = readdir(dir)) != NULL)
        {
            fname.assign(entry->d_name);

            if (std::string::npos != fname.find(".sh"))
            {
                found = true;
                printf("Found script %s\n", fname.c_str());
                script = path;
                script.append("/").append(std::string(entry->d_name));

                // run suspend and resume on each script
                system(std::string(script.c_str()).append(" suspend").c_str());
                system(std::string(script.c_str()).append(" resume").c_str());
            }
        }
    }
    while (entry != NULL);

    closedir(dir);

    if (!found)
    {
        fprintf(stderr, "Found no scripts to run. Will exit\n");
        if (loop)
        {
            g_main_loop_quit(loop);
        }
    }
    return;
}

gboolean timeout_func(gpointer user_data)
{
    FILE *dev = NULL;
    dirpaths *paths = (dirpaths*)user_data;
    char c;

    if (!paths)
    {
        fprintf(stderr, "Invalid user data\n");
        exit(-1);
    }

    dev = fopen(paths->chardev.c_str(), "r");

    if (dev)
    {
        c = (char)fgetc(dev);
        fclose(dev);

        if ('R' == c)
        {
            printf("Resume signalled\n");
            exec_scripts(paths->scriptsdir);
        }
        else
        {
            // printf("Got %c\n", c);
        }
    }
    else
    {
        fprintf(stderr, "Failed to open char device! Will exit\n");
        return FALSE;
    }

    return TRUE;
}

void usage(const char *pn)
{
    fprintf(stderr, "Usage: %s -d <chardev> -s <scripts_dir>\n",  pn);
    exit(-1);
}

int main(int argc, char** argv)
{
    int opt;
    char *chrdev = 0, *sdir = 0;
    dirpaths *dirs = NULL;

    while ((opt = getopt(argc, argv, "d:s:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                chrdev = optarg;;
                break;
            case 's':
                sdir = optarg;
                break;
            default:
                usage(argv[0]);
        }
    }

    if (!chrdev || !sdir)
    {
        usage(argv[0]);
    }
    else
    {
        dirs = new dirpaths();

        if (!dirs)
        {
            fprintf(stderr, "Failed to allocate mem\n");
            exit(-1);
        }

        dirs->chardev.assign(chrdev);
        dirs->scriptsdir.assign(sdir);
    }

    loop = g_main_loop_new(NULL, FALSE);

    if (!loop) {
        fprintf(stderr, "Failed to start daemon\n");
        exit(-1);
    }

    poll_id = g_timeout_add(2000 /* ms */, timeout_func, dirs);

    g_main_loop_run(loop);

    g_source_remove(poll_id);
    g_main_loop_unref(loop);
    delete dirs;

    return 0;
}
