#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "view.h"

static int get_tag_size(unsigned char *header)
{
    int size;

    size = ((header[6] & 0x7F) << 21) |
           ((header[7] & 0x7F) << 14) |
           ((header[8] & 0x7F) << 7) |
           (header[9] & 0x7F);

    return size;
}

static int get_frame_size(unsigned char *size_buf)
{
    int size;

    size = ((unsigned int)size_buf[0] << 24) |
           ((unsigned int)size_buf[1] << 16) |
           ((unsigned int)size_buf[2] << 8) |
           (unsigned int)size_buf[3];

    return size;
}

static void print_comment(char *data, int frame_size)
{
    int i;

    if (frame_size < 5)
    {
        printf("Comment : Invalid comment frame\n");
        return;
    }

    /*
     * COMM frame:
     * 1 byte  -> encoding
     * 3 bytes -> language
     * remaining -> description + comment
     */

    i = 4;

    while (i < frame_size && data[i] != '\0')
    {
        i++;
    }

    if (i < frame_size)
    {
        i++;
        printf("Comment : %s\n", data + i);
    }
    else
    {
        printf("Comment : No comment\n");
    }
}

Status view_file(char *filename)
{
    FILE *fp;

    unsigned char header[10];

    int tag_size;
    long tag_end;

    fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        printf("ERROR: Unable to open file\n");
        return failure;
    }

    if (fread(header, 1, 10, fp) != 10)
    {
        printf("ERROR: Unable to read ID3 header\n");
        fclose(fp);
        return failure;
    }

    if (header[0] != 'I' ||
        header[1] != 'D' ||
        header[2] != '3')
    {
        printf("ERROR: ID3 tag not found\n");
        fclose(fp);
        return failure;
    }

    printf("\n----------------------------------------\n");
    printf("          MP3 TAG READER\n");
    printf("----------------------------------------\n");

    printf("MP3 File : %s\n", filename);
    printf("ID3 Tag  : Found\n");
    printf("Version  : 2.%d.%d\n", header[3], header[4]);

    tag_size = get_tag_size(header);

    printf("Tag Size : %d bytes\n", tag_size);

    tag_end = 10 + tag_size;

    while (ftell(fp) < tag_end)
    {
        char frame_id[5];

        unsigned char size_buf[4];
        unsigned char flags[2];

        int frame_size;

        char *data;

        if (fread(frame_id, 1, 4, fp) != 4)
            break;

        if (frame_id[0] == 0 &&
            frame_id[1] == 0 &&
            frame_id[2] == 0 &&
            frame_id[3] == 0)
        {
            break;
        }

        frame_id[4] = '\0';

        if (fread(size_buf, 1, 4, fp) != 4)
            break;

        frame_size = get_frame_size(size_buf);

        if (fread(flags, 1, 2, fp) != 2)
            break;

        if (frame_size < 0 ||
            ftell(fp) + frame_size > tag_end)
        {
            printf("ERROR: Invalid frame size\n");
            fclose(fp);
            return failure;
        }

        data = malloc(frame_size + 1);

        if (data == NULL)
        {
            printf("ERROR: Memory allocation failed\n");
            fclose(fp);
            return failure;
        }

        if (fread(data, 1, frame_size, fp) != frame_size)
        {
            free(data);
            break;
        }

        data[frame_size] = '\0';

        if (strcmp(frame_id, "TIT2") == 0)
        {
            printf("Title    : %s\n", data + 1);
        }
        else if (strcmp(frame_id, "TPE1") == 0)
        {
            printf("Artist   : %s\n", data + 1);
        }
        else if (strcmp(frame_id, "TALB") == 0)
        {
            printf("Album    : %s\n", data + 1);
        }
        else if (strcmp(frame_id, "TYER") == 0)
        {
            printf("Year     : %s\n", data + 1);
        }
        else if (strcmp(frame_id, "TCON") == 0)
        {
            printf("Genre    : %s\n", data + 1);
        }
        else if (strcmp(frame_id, "COMM") == 0)
        {
            print_comment(data, frame_size);
        }
        else if (strcmp(frame_id, "APIC") == 0)
        {
            printf("Picture  : Attached picture found\n");
        }

        free(data);
    }

    printf("----------------------------------------\n");

    fclose(fp);

    return success;
}