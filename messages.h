#ifndef MESSAGES_H
#define MESSAGES_H

typedef int CMD_T;

const CMD_T CMD_GET = 1;
const CMD_T CMD_PUT = 2;
const CMD_T CMD_CD = 3;
const CMD_T CMD_SHUTDOWN = 4;
const CMD_T CMD_RESTART = 5;
const CMD_T CMD_EXEC = 6;
const CMD_T CMD_BYE = 0;

const CMD_T SERVE_BYE = 0;
const CMD_T SERVE_FILE = 1;

const CMD_T SERVE_GET_BEGIN = 10;
const CMD_T SERVE_GET_ERROR_NOTFOUND = 11;
const CMD_T SERVE_GET_ERROR_DENIED = 12;
const CMD_T SERVE_GET_ERROR_CANNOTCREATE = 12;

const CMD_T SERVE_PUT_BEGIN = 20;
const CMD_T SERVE_PUT_ERROR_CANNOTCREATE = 21;

const CMD_T SERVE_CD_SUCCESS = 30;
const CMD_T SERVE_CD_FAILED = 31;

const CMD_T SERVE_EXEC_BEGIN = 41;
const CMD_T SERVE_EXEC_END = 0; //null is probably the best to send, it shouldn't be mistaken for anything else...

const CMD_T LOGIN_SUCCESS = 100;
const CMD_T LOGIN_FAIL = 101;

const CMD_T ACCESS_DENIED = 403;

#endif