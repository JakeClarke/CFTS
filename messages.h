#ifndef MESSAGES_H
#define MESSAGES_H

#define CMD_T int

const CMD_T CMD_GET = 1;
const CMD_T CMD_PUT = 2;
const CMD_T CMD_CD = 3;
const CMD_T CMD_SHUTDOWN = 4;
const CMD_T CMD_RESTART = 5;
const CMD_T CMD_EXEC = 6;
const CMD_T CMD_BYE = 0;

const CMD_T SERVE_BYE = 0;
const CMD_T SERVE_FILE = 1;

#endif