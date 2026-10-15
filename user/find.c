#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

static int
matches(const char *pattern, const char *name)
{
    const char *star = 0;
    const char *retry = 0;

    while (*name) {
        if (*pattern == '*') {
            star = pattern++;
            retry = name;
        } else if (*pattern == *name) {
            pattern++;
            name++;
        } else if (star) {
            pattern = star + 1;
            name = ++retry;
        } else {
            return 0;
    }
    }

    while (*pattern == '*')
        pattern++;
    return *pattern == '\0';
}

static int
matches_type(int file_type, int requested_type)
{
    if (requested_type == 0)
        return 1;
    if (requested_type == 'f')
        return file_type == T_FILE || file_type == T_DEVICE;
    return file_type == T_DIR;
}

static int
walk(char *path, int path_len, const char *pattern, int requested_type,
         int starting_dir)
{
    int fd;
    struct dirent de;
    struct stat st;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (starting_dir)
            fprintf(2, "find: cannot open %s\n", path);
        return starting_dir ? -1 : 0;
    }

    if (fstat(fd, &st) < 0) {
        close(fd);
        return 0;
    }
    if (st.type != T_DIR) {
        close(fd);
        return 0;
    }

    int entries_seen = 0;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        int name_len = 0;
        int separator;
        int child_len;
        int result;
        int skipped;

        entries_seen++;

        if (de.inum == 0)
            continue;
        while (name_len < DIRSIZ && de.name[name_len] != '\0')
            name_len++;
        if ((name_len == 1 && de.name[0] == '.') ||
                (name_len == 2 && de.name[0] == '.' && de.name[1] == '.'))
            continue;

        separator = path_len > 0 && path[path_len - 1] != '/';
        child_len = path_len + separator + name_len;
        if (child_len >= MAXPATH) {
            fprintf(2, "find: path too long\n");
            close(fd);
            return -1;
        }

        if (separator)
            path[path_len] = '/';
        memmove(path + path_len + separator, de.name, name_len);
        path[child_len] = '\0';

        if (stat(path, &st) == 0) {
            if (matches(pattern, path + path_len + separator) &&
                    matches_type(st.type, requested_type))
                printf("%s\n", path);

            if (st.type == T_DIR) {
                close(fd);
                fd = -1;
                result = walk(path, child_len, pattern, requested_type, 0);
                path[path_len] = '\0';
                if (result < 0) {
                    return result;
                }
                fd = open(path, O_RDONLY);
                if (fd < 0)
                    return 0;
                for (skipped = 0; skipped < entries_seen; skipped++) {
                    if (read(fd, &de, sizeof(de)) != sizeof(de)) {
                        close(fd);
                        return 0;
                    }
                }
            }
    }

        path[path_len] = '\0';
    }

    close(fd);
    return 0;
}

int
main(int argc, char *argv[])
{
    char path[MAXPATH];
    int path_len;
    int requested_type = 0;

    if (argc != 3 && argc != 5) {
        fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
        exit(1);
    }
    if (argc == 5) {
        if (strcmp(argv[3], "-type") != 0 ||
                (argv[4][0] != 'f' && argv[4][0] != 'd') || argv[4][1] != '\0') {
            fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
            exit(1);
        }
        requested_type = argv[4][0];
    }

    path_len = strlen(argv[1]);
    if (path_len >= MAXPATH) {
        fprintf(2, "find: path too long\n");
        exit(1);
    }
    memmove(path, argv[1], path_len + 1);

    if (walk(path, path_len, argv[2], requested_type, 1) < 0)
        exit(1);
    exit(0);
}