#include <stdio.h>

#include "help.h"

void display_help(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("          MP3 TAG READER & EDITOR\n");
    printf("===============================================\n");

    printf("\nVIEW:\n");
    printf("./a.out -v sample.mp3\n");

    printf("\nEDIT:\n");
    printf("./a.out -e -t \"New Title\" sample.mp3\n");
    printf("./a.out -e -a \"New Artist\" sample.mp3\n");
    printf("./a.out -e -A \"New Album\" sample.mp3\n");
    printf("./a.out -e -y \"2026\" sample.mp3\n");
    printf("./a.out -e -g \"Rock\" sample.mp3\n");
    printf("./a.out -e -c \"My Comment\" sample.mp3\n");

    printf("\nOPTIONS:\n");
    printf("-v    View MP3 tags\n");
    printf("-e    Edit MP3 tags\n");
    printf("-t    Edit Title\n");
    printf("-a    Edit Artist\n");
    printf("-A    Edit Album\n");
    printf("-y    Edit Year\n");
    printf("-g    Edit Genre\n");
    printf("-c    Edit Comment\n");
    printf("-h    Display Help\n");

    printf("\n===============================================\n");
}