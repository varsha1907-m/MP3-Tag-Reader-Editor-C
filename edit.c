#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"

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

static void put_frame_size(unsigned char *size_buf, int size)
{
    size_buf[0] = (size >> 24) & 0xFF;
    size_buf[1] = (size >> 16) & 0xFF;
    size_buf[2] = (size >> 8) & 0xFF;
    size_buf[3] = size & 0xFF;
}

static int get_frame_id(char *option, char *frame_id)
{
    if (strcmp(option, "-t") == 0)
    {
        strcpy(frame_id, "TIT2");
        return 1;
    }

    if (strcmp(option, "-a") == 0)
    {
        strcpy(frame_id, "TPE1");
        return 1;
    }

    if (strcmp(option, "-A") == 0)
    {
        strcpy(frame_id, "TALB");
        return 1;
    }

    if (strcmp(option, "-y") == 0)
    {
        strcpy(frame_id, "TYER");
        return 1;
    }

    if (strcmp(option, "-g") == 0)
    {
        strcpy(frame_id, "TCON");
        return 1;
    }

    if (strcmp(option, "-c") == 0)
    {
        strcpy(frame_id, "COMM");
        return 1;
    }

    return 0;
}

static int copy_bytes(FILE *src, FILE *dest, long count)
{
    unsigned char buffer[4096];

    while (count > 0)
    {
        long current;

        current = count;

        if (current > sizeof(buffer))
            current = sizeof(buffer);

        if (fread(buffer, 1, current, src) != current)
            return 0;

        if (fwrite(buffer, 1, current, dest) != current)
            return 0;

        count = count - current;
    }

    return 1;
}

static int write_new_frame(FILE *dest,
                           char *frame_id,
                           unsigned char *old_size_buf,
                           unsigned char *flags,
                           char *new_data)
{
    unsigned char *data;

    int old_frame_size;
    int new_frame_size;

    old_frame_size = get_frame_size(old_size_buf);

    if (strcmp(frame_id, "COMM") == 0)
    {
        /*
         * Encoding  = 1 byte
         * Language  = 3 bytes
         * Description = 1 byte
         * Comment   = remaining bytes
         */

        new_frame_size = 5 + strlen(new_data);
    }
    else
    {
        /*
         * Encoding = 1 byte
         * Text = remaining bytes
         */

        new_frame_size = 1 + strlen(new_data);
    }

    if (new_frame_size > old_frame_size)
    {
        printf("ERROR: New data is too large\n");
        printf("Maximum allowed text length: %d bytes\n",
               old_frame_size - 1);

        return 0;
    }

    data = calloc(old_frame_size, 1);

    if (data == NULL)
    {
        printf("ERROR: Memory allocation failed\n");
        return 0;
    }

    if (strcmp(frame_id, "COMM") == 0)
    {
        data[0] = 0;

        data[1] = 'E';
        data[2] = 'N';
        data[3] = 'G';

        data[4] = '\0';

        strcpy((char *)(data + 5), new_data);
    }
    else
    {
        data[0] = 0;

        strcpy((char *)(data + 1), new_data);
    }

    put_frame_size(old_size_buf, old_frame_size);

    fwrite(frame_id, 1, 4, dest);
    fwrite(old_size_buf, 1, 4, dest);
    fwrite(flags, 1, 2, dest);
    fwrite(data, 1, old_frame_size, dest);

    free(data);

    return 1;
}

Status edit_file(char *filename, char *option, char *new_data)
{
    FILE *src;
    FILE *dest;

    unsigned char header[10];

    char required_frame[5];

    int tag_size;
    long tag_end;

    int found = 0;

    if (get_frame_id(option, required_frame) == 0)
    {
        printf("ERROR: Invalid edit option\n");
        printf("Use -t, -a, -A, -y, -g or -c\n");

        return failure;
    }

    if (strlen(new_data) == 0)
    {
        printf("ERROR: New data cannot be empty\n");
        return failure;
    }

    src = fopen(filename, "rb");

    if (src == NULL)
    {
        printf("ERROR: Unable to open MP3 file\n");
        return failure;
    }

    dest = fopen("temp.mp3", "wb");

    if (dest == NULL)
    {
        printf("ERROR: Unable to create temporary file\n");

        fclose(src);

        return failure;
    }

    if (fread(header, 1, 10, src) != 10)
    {
        printf("ERROR: Unable to read ID3 header\n");

        fclose(src);
        fclose(dest);
        remove("temp.mp3");

        return failure;
    }

    if (header[0] != 'I' ||
        header[1] != 'D' ||
        header[2] != '3')
    {
        printf("ERROR: ID3 tag not found\n");

        fclose(src);
        fclose(dest);
        remove("temp.mp3");

        return failure;
    }

    /*
     * Copy ID3 header.
     */
    fwrite(header, 1, 10, dest);

    tag_size = get_tag_size(header);

    tag_end = 10 + tag_size;

    while (ftell(src) < tag_end)
    {
        char frame_id[5];

        unsigned char size_buf[4];
        unsigned char flags[2];

        int frame_size;

        if (fread(frame_id, 1, 4, src) != 4)
            break;

        if (frame_id[0] == 0 &&
            frame_id[1] == 0 &&
            frame_id[2] == 0 &&
            frame_id[3] == 0)
        {
            /*
             * Copy remaining padding.
             */

            fseek(src, -4, SEEK_CUR);

            if (!copy_bytes(src, dest, tag_end - ftell(src)))
            {
                fclose(src);
                fclose(dest);
                remove("temp.mp3");

                return failure;
            }

            break;
        }

        frame_id[4] = '\0';

        if (fread(size_buf, 1, 4, src) != 4)
            break;

        if (fread(flags, 1, 2, src) != 2)
            break;

        frame_size = get_frame_size(size_buf);

        if (ftell(src) + frame_size > tag_end)
        {
            printf("ERROR: Invalid frame size\n");

            fclose(src);
            fclose(dest);
            remove("temp.mp3");

            return failure;
        }

        if (strcmp(frame_id, required_frame) == 0)
        {
            /*
             * Skip old frame data.
             */

            fseek(src, frame_size, SEEK_CUR);

            /*
             * Write new frame.
             */

            if (!write_new_frame(dest,
                                 frame_id,
                                 size_buf,
                                 flags,
                                 new_data))
            {
                fclose(src);
                fclose(dest);
                remove("temp.mp3");

                return failure;
            }

            found = 1;
        }
        else
        {
            /*
             * Copy frame header.
             */

            fwrite(frame_id, 1, 4, dest);
            fwrite(size_buf, 1, 4, dest);
            fwrite(flags, 1, 2, dest);

            /*
             * Copy frame data.
             */

            if (!copy_bytes(src, dest, frame_size))
            {
                fclose(src);
                fclose(dest);
                remove("temp.mp3");

                return failure;
            }
        }
    }

    /*
     * Copy actual MP3 audio data.
     */

    while (1)
    {
        unsigned char buffer[4096];

        size_t bytes;

        bytes = fread(buffer, 1, sizeof(buffer), src);

        if (bytes == 0)
            break;

        fwrite(buffer, 1, bytes, dest);
    }

    fclose(src);
    fclose(dest);

    if (found == 0)
    {
        printf("ERROR: Required tag not found\n");

        remove("temp.mp3");

        return failure;
    }

    /*
     * Replace original file.
     */

    if (remove(filename) != 0)
    {
        printf("ERROR: Unable to remove original file\n");

        remove("temp.mp3");

        return failure;
    }

    if (rename("temp.mp3", filename) != 0)
    {
        printf("ERROR: Unable to rename temporary file\n");

        return failure;
    }

    printf("\nTag updated successfully!\n");

    return success;
}