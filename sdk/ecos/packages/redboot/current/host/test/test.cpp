// TEST.CPP -- test program for LOOPBACK.SYS
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stdafx.h"

int argc = 0;
#define MAX_ARGV 32
char *argv[MAX_ARGV];

//
// Scan through an input line and break it into "arguments".  These
// are space delimited strings.  Return a structure which points to
// the strings, similar to a Unix program.
// Note: original input is destroyed by replacing the delimiters with 
// null ('\0') characters for ease of use.
//
void //struct cmd *
parse(char *line, int *argc, char **argv)
{
    char *cp = line;
    char *pp;
    int indx = 0;

    while (*cp) {
        // Skip leading spaces
        while (*cp && *cp == ' ') cp++;
        if (!*cp) {
            break;  // Line ended with a string of spaces
        }
        argv[indx++] = cp;
        while (*cp) {
            if (*cp == ' ') {
                *cp++ = '\0';
                break;
            } else if (*cp == '"') {
                // Swallow quote, scan till following one
                if (argv[indx-1] == cp) {
                    argv[indx-1] = ++cp;
                }
                pp = cp;
                while (*cp && *cp != '"') {
                    if (*cp == '\\') {
                        // Skip over escape - allows for escaped '"'
                        cp++;
                    }
                    // Move string to swallow escapes
                    *pp++ = *cp++;
                }
                if (!*cp) {
                    printf("Unbalanced string!\n");
                } else {
                    if (pp != cp) *pp = '\0';
                    *cp++ = '\0';
                    break;
                }
            } else {
                cp++;
            }
        }
    }
    *argc = indx;
    //return cmd_search(__RedBoot_CMD_TAB__, &__RedBoot_CMD_TAB_END__, argv[0]);
}

static struct {
	unsigned int DataTransferLength;
#define CMD_FLAGS_DIRECTION 0x01
	unsigned char Flags;
} cmd;

static struct {
	unsigned int DataResidue;
#define STATUS_SUCCESS 0x01
#define STATUS_FAILURE 0x00
	unsigned char Status;
} stat;

#define CMD_TRANSFER	0
#define FILE_TRANSFER	1
static int state = CMD_TRANSFER;
static char cmdline[256];

int
main(int argc, char * argv[])
{
	HANDLE hdevice;
	for (;;) {
		hdevice = CreateFile("\\\\.\\LOOPBACK", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hdevice != INVALID_HANDLE_VALUE) {
			break;
		}
	}
	printf("Opened LOOPBACK device\n");

	FILE * fp = NULL;
	for (;;) {
		DWORD count;
		if (ReadFile(hdevice, &cmd, sizeof(cmd), &count, NULL)) {
			assert(count == sizeof(cmd));
			if (cmd.Flags & CMD_FLAGS_DIRECTION) {
				// Device is sending data
				char * buf = (char *)malloc(cmd.DataTransferLength + 1);
				assert(buf != 0);
				if (ReadFile(hdevice, buf, cmd.DataTransferLength, &count, NULL)) {
					assert(count == cmd.DataTransferLength);
					buf[cmd.DataTransferLength] = 0;
					printf("%s", buf);
					stat.DataResidue = 0;
					stat.Status = STATUS_SUCCESS;
				}
				else {
					// Read data failed
					printf("Read data failed\n");
					stat.Status = STATUS_FAILURE;
				}
				free(buf);
			}
			else {
				// We need to send data to device
				char * buf = (char *)malloc(cmd.DataTransferLength);
				assert(buf != 0);

				int n;
				if (state == CMD_TRANSFER) {
					//TODO Assume that device asks for more data than user provides
					gets(buf);
					// Parse input for possible load command
					strcpy(cmdline, buf);
					cmdline[strlen(buf)] = 0;
					parse(cmdline, &argc, &argv[0]);
					if (strcmp(argv[0], "load") == 0) {
						//TODO Get the filename the right way
						fp = fopen(argv[argc - 1], "r");
						state = FILE_TRANSFER;
					}
					strcat(buf, "\r\n");
					n = strlen(buf);
				}
				else if (state == FILE_TRANSFER) {
					n = fread(buf, 1, cmd.DataTransferLength, fp);
					if (n < (int)cmd.DataTransferLength) {
						fclose(fp);
						state = CMD_TRANSFER;
					}
				}

				if (WriteFile(hdevice, buf, cmd.DataTransferLength, &count, NULL)) {
					assert(count == cmd.DataTransferLength);
					stat.DataResidue = cmd.DataTransferLength - n;
					stat.Status = STATUS_SUCCESS;
				}
				else {
					// Write data failed
					printf("Write data failed\n");
					stat.Status = STATUS_FAILURE;
				}
				free(buf);
			}
			// Send status
			if (WriteFile(hdevice, &stat, sizeof(stat), &count, NULL)) {
				assert(count == sizeof(stat));
			}
			else {
				// Send status failed
				printf("Send status failed\n");
			}
		}
		else {
			// Read command failed
			printf("Read command failed\n");
		}
	}
	return 0;
}
