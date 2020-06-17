#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/*Function to print all files within the current directory */
void listDir() {
    struct dirent *directory;
    DIR *dir = opendir("."); //Open current working directory

    if (dir == NULL) { //Error if unable to open directory
        size_t bufsize = BUFSIZ;
        char *line = "Error! Could not open directory\n";
        write(2, line, bufsize);

    } else {
        //Loop through current working directory and output all file names
        char *line = NULL;
        while ((directory = readdir(dir)) != NULL) {
            line = directory->d_name;
            write(1, line, strlen(line));
            write(1, " ", 2);
        }
        write(1, "\n", 2);
    }
    closedir(dir); //Close current working directory
}

/* Function to print current working directory */
void showCurrentDir() {
    char *buffer = NULL; //Buffer for line
    buffer = getcwd(NULL, 0); //Retrieve line into buffer
    write(1, buffer, strlen(buffer));
    write(1, "\n", 2);
    free(buffer); //Free dynamic buffer
}

/* Function to create a new directory */
void makeDir(char *dirName) {
   int val = mkdir(dirName, 0777); //Command to create with user permissions
   if (val == -1) { //Error if unable to create
     char *line = "Error! Could not create directory\n";
     write(1, line, strlen(line));
   }
}

/* Function to change directory */
void changeDir(char *dirName) { 
   int error = chdir(dirName); //Change directory to specified name
   if (error == -1) { //Error if unable to change
        char *line = "Error! Could not change directory.\n";
        write(1, line, strlen(line));    
   }
}

/* Function to copy a file from src to dest */
void copyFile(char *sourcePath, char *destinationPath) {
    char *ret;
    ret = strrchr(sourcePath, '/');

    char src[4096];
    char dst[4096];

    //Copy the structure of source and destination
    strcpy(src, sourcePath);
    strcpy(dst, destinationPath);

    //Check destination for folder type
    int folder = 0;
    if (open(destinationPath, O_DIRECTORY) == -1) {
        folder = 0;
    } else {
        folder = 1;
    }
    //Open input file
    int file = open(sourcePath, O_RDONLY);

    char *dest;
    if (file == -1) {
        char *error = "Error! Unable to open source file.\n";
        write(1, error, strlen(error));
    } else {
        if ((ret == NULL) && ((strcmp(dst, ".") == 0) || (strcmp(dst, "..") == 0))) {
            dest = strcat(dst, "/");
            dest = strcat(dst, src);
            int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (dstfile == -1) {
                close(file);
                char *error = "Error! Unable to open destination file.\n";
                write(1, error, strlen(error));
            } else {
                ssize_t count;
                char buffer[BUFSIZ];
                while ((count = read(file , buffer, sizeof(buffer))) != 0) {
                    write(dstfile, buffer, count);
                }
                close(dstfile);
            }
        } else if ((ret != NULL) && ((strcmp(dst, ".") == 0) || (strcmp(dst, "..") == 0))) {
            dest = strcat(dst, ret);
            int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (dstfile == -1) {
                close(file);
                char *error = "Error! Unable to open destination file.\n";
                write(1, error, strlen(error));
            } else {
                ssize_t count;
                char buffer[BUFSIZ];
                while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                    write(dstfile, buffer, count);
                }
                close(dstfile);
            }
        } else if (folder) {
            int item = strlen(dst);
            if (dst[item - 1] == '/') {
                if (ret != NULL) {
                    ret++;
                    dest = strcat(dst, ret);

                    int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (dstfile == -1) {
                        close(file);
                        char *error = "Error! Unable to open destination file.\n";
                        write(1, error, strlen(error));
                    } else {
                        ssize_t count;
                        char buffer[BUFSIZ];
                        while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                            write(dstfile, buffer, count);
                        }
                        close(dstfile);
                    }
                } else {
                    dest = strcat(dst, src);
                    int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (dstfile == -1) {
                        close(file);
                        char *error = "Error! Unable to open destination file.\n";
                        write(1, error, strlen(error));
                    } else {
                        ssize_t count;
                        char buffer[BUFSIZ];
                        while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                            write(dstfile,buffer, count);
                        }
                        close(dstfile);
                    }

                }
            } else {
                if (ret != NULL) {
                    dest = strcat(dst, ret);
                    int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (dstfile == -1) {
                        close(file);
                        char *error = "Error! Unable to open destination file.\n";
                        write(1, error, strlen(error));
                    } else {
                        ssize_t count;
                        char buffer[BUFSIZ];
                        while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                            write(dstfile, buffer, count);
                        }
                        close(dstfile);
                    }
                } else {
                    dest = strcat(dst, "/");
                    dest = strcat(dst, src);
                    int dstfile = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if (dstfile == -1) {
                        close(file);
                        char *error = "Error! Unable to open destination file.\n";
                        write(1, error, strlen(error));
                    } else {
                        ssize_t count;
                        char buffer[BUFSIZ];
                        while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                            write(dstfile, buffer, count);
                        }
                        close(dstfile);
                    }
                }
            }
        } else {
            int dstfile = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (dstfile == -1) {
                close(file);
                char *error = "Error! Unable to open destination file.\n";
                write(1, error, strlen(error));
            } else {
                ssize_t count;
                char buffer[BUFSIZ];
                while ((count = read(file, buffer, sizeof(buffer))) != 0) {
                    write(dstfile, buffer, count);
                }
                close(dstfile);
            }
        }
        close(file);
    }
}

/* Function to move a file or rename a file */
void moveFile(char *sourcePath, char *destinationPath) {
    int value;
    char *ret;
    ret = strrchr(sourcePath, '/');
    
    char src[4096];
    char dst[4096];

    strcpy(src, sourcePath);
    strcpy(dst, destinationPath);

    int folder = 0;
    if (open(destinationPath, O_DIRECTORY) == -1) {
        folder = 0;
    } else {
        folder = 1;
    }
    char *dest;

    if ((ret == NULL) && ((strcmp(dst, ".") == 0) || (strcmp(dst, "..") == 0))) {
        dest = strcat(dst, "/");
        dest = strcat(dst, src);
        value = rename(src, dest);
        if (value == -1) {
            char *error = "Error! Unable to move files.\n";
            write(1, error, strlen(error));
        }
        
    } else if ((ret != NULL) && ((strcmp(dst, ".") == 0) || (strcmp(dst, "..") == 0))) {
        dest = strcat(dst, ret);
        value = rename(src, dest);
        if (value == -1) {
            char *error = "Error! Unable to move files.\n";
            write(1, error, strlen(error));
        }
        
    } else if (folder) {
       int item = strlen(dst);
       if (dst[item - 1] == '/') { // mv input.txt name/
            if (ret != NULL) { // /input
                ret++;
                dest = strcat(dst, ret); //name/input
                value = rename(src, dest);
                if (value == -1) {
                    char *error = "Error! Unable to move files.\n";
                    write(1, error, strlen(error));
                }

            } else { // input
                dest = strcat(dst, src); // name/input
                value = rename(src, dest);
                if (value == -1) {
                    char *error = "Error! Unable to move files.\n";
                    write(1, error, strlen(error));
                }
            }
       } else {
            if (ret != NULL) {
                dest = strcat(dst, ret);
                value = rename(sourcePath, dest);
                if (value == -1) {
                    char *error = "Error! Unable to move files.\n";
                    write(1, error, strlen(error));
                }
            } else {
                dest = strcat(dst, "/");
                dest = strcat(dst, sourcePath);
                value = rename(src, dest);
                if (value == -1) {
                    char *error = "Error! Unable to move files.\n";
                    write(1, error, strlen(error));
                }
            }
       }
    } else {
        value = rename(src, dst);
        if (value == -1) {
            char *error = "Error! Unable to move files.\n";
            write(1, error, strlen(error));
        }
    }
 
}

/* Delete a file function */
void deleteFile(char *filename) {
    int value;
    value = unlink(filename); //Remove link
    if (value == -1) { //Throw error if unable to remove
        char *error = "Error! Unable to delete file.\n";
        write(1, error, strlen(error));
    }
}

/* Function to display file contents to stdout */
void displayFile(char *filename) {
    int file = open(filename, O_RDONLY); //Open file for reading
    if (file == -1) {
        char *error = "Error! Could not open the file.\n";
        write(1, error, strlen(error));
    } else {
        ssize_t count;
        char buffer[BUFSIZ]; //Loop through and print contents
        while ((count = read(file, buffer, sizeof(buffer))) != 0) {
            write(1, buffer, count);
        }
        close(file);
    }
}
