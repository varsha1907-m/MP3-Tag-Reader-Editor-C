#include <stdio.h>
#include <string.h>

#include "operations.h"
#include "view.h"
#include "edit.h"
#include "help.h"

OperationType check_operation_type(char *symbol)
{
    if (strcmp(symbol, "-v") == 0)
        return view;

    if (strcmp(symbol, "-e") == 0)
        return edit;

    if (strcmp(symbol, "-h") == 0)
        return help;

    return unsupported;
}

int main(int argc, char *argv[])
{
    OperationType operation;

    if (argc < 2)
    {
        printf("ERROR: No operation given\n");
        printf("Use -h for help\n");
        return failure;
    }

    operation = check_operation_type(argv[1]);

    if (operation == view)
    {
        if (argc != 3)
        {
            printf("Usage: ./a.out -v <file.mp3>\n");
            return failure;
        }

        if (view_file(argv[2]) == failure)
            return failure;
    }

    else if (operation == edit)
    {
        if (argc != 5)
        {
            printf("Usage: ./a.out -e <-t/-a/-A/-y/-g/-c> <new_data> <file.mp3>\n");
            return failure;
        }

        if (edit_file(argv[4], argv[2], argv[3]) == failure)
            return failure;
    }

    else if (operation == help)
    {
        if (argc != 2)
        {
            printf("Usage: ./a.out -h\n");
            return failure;
        }

        display_help();
    }

    else
    {
        printf("ERROR: Unsupported operation\n");
        printf("Use -h for help\n");
        return failure;
    }

    return success;
}