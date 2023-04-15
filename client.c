char *validate(char *cmd) {
    char *res = malloc(sizeof(char) * 1024);
    char *resArray[2] = { NULL, NULL };
    int i;
    
    if (sscanf(cmd, "findfile %ms", &resArray[0]) == 1) {
        sprintf(res, "echo -e $(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1)", resArray[0]);
    }
    else if (sscanf(cmd, "sgetfiles %ms %ms%*c", &resArray[0], &resArray[1]) == 2) {
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -size +%sc -size -%sc)", resArray[0], resArray[1]);
    }
    else if (sscanf(cmd, "dgetfiles %ms %ms%*c", &resArray[0], &resArray[1]) == 2) {
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -newermt \"%s\" ! -newermt \"%s\")", resArray[0], resArray[1]);
    }
    else if (sscanf(cmd, "getfiles %[^\n]", res) == 1 || sscanf(cmd, "gettargz %[^\n]", res) == 1) {
        char *token;
        char files[1024] = "";
        int count = 0;
        token = strtok(res, " ");
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            if (strcmp(cmd, "getfiles") == 0) {
                sprintf(tmp, "-name %s ", token);
            }
            else if (strcmp(cmd, "gettargz") == 0) {
                sprintf(tmp, "-iname \"*.%s\" ", token);
            }
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    }
    else if (strcmp(cmd, "quit") == 0) {
        sprintf(res, "%s", cmd);
    }
    else {
        sprintf(res, "Invalid Command");
    }
    
    // Free all allocated memory
    for (i = 0; i < 2; i++) {
        if (resArray[i] != NULL) {
            free(resArray[i]);
        }
    }
    
    return res;
}
