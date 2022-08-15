#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

enum usertype
{
    NULL_USER,
    ADMINISTRATOR,
    NORMAL,
};

enum cmdtype        //命令码
{
    CMD_NULL,       //空命令，不做任何操作
    CLI_LOGIN,      //登录命令
    CLI_LOGOUT,     //登出命令
    CLI_CHANGE,
    CLI_INSERT,
    CLI_DELETE,
    CLI_SEARCH_OWN,
    CLI_SEARCH_ALL,
    CLI_SEARCH_NAME,
    CLI_SEARCH_HISTORY,
    SER_ACK,
    SER_ACKEND,
    SER_SUCCESS,    //操作成功,无返回数据
    SER_ERR,        //操作失败
};

enum errtpye      //错误类型
{
    ERR_NULL,
    ERR_PASSWD,
    ERR_NONUSER,
    ERR_NOADMIN,
};

enum entry
{
    STAFF_NULL,
    STAFF_ID,
    #define STAFF_BASE STAFF_ID
    STAFF_USERTYPE,
    STAFF_NAME,
    STAFF_PASSWORD,
    STAFF_CONTACT,
    STAFF_ADDRRESS,
    STAFF_OFFICE,
    STAFF_DATE,
    STAFF_AGE,
    STAFF_PAY,
    STAFF_LEVEL,
};

#include <stdint.h>
#include <time.h>

struct msg              //网络数据包格式基类
{
    uint32_t size;           //网络数据包长度
    enum cmdtype cmdcode;    //CLI_LOGOUT,CLI_SEARCH_*
};

struct userinfo
{
    uint32_t id;        //员工工号
    enum usertype usertype;
    char name[32];      //员工姓名
    char password[32];  //员工登录密码
    char contact[16];   //员工联系方式
    char address[128];  //员工家庭住址
    char office[32];    //员工职位
    char date[16];      //员工入职日期
    uint32_t age;       //员工年龄
    uint32_t pay;       //员工工资
    uint32_t level;     //员工等级
};

struct historylog
{
    time_t time;            //记录时间
    uint32_t id;            //产生记录的用户工号
    char event[192];        //历史记录事件描述
};

/*********************客户端数据包协议***********************/

struct msg_login
{
    struct msg msg;         //CLI_LOGIN
    uint32_t id;            //操作用户ID
    enum usertype usertype; //操作用户用户类型
    char password[32];      //员工登录密码
};

struct msg_logout
{
    struct msg msg;         //CLI_LOGOUT
    uint32_t id;            //操作用户ID
};

struct msg_delete
{
    struct msg msg;         //CLI_DELETE
    uint32_t id;            //操作用户ID
    enum usertype usertype; //操作用户用户类型

    uint32_t chid;          //被删除用户ID
};

struct msg_search
{
    struct msg msg;         //CLI_SEARCH_ALL, CLI_SEARCH_NAME(管理员用户使用)，CLI_SEARCH_OWN(普通用户使用)
    uint32_t id;            //操作用户ID
    enum usertype usertype; //操作用户用户类型

    char name[32];          //查找目标用户名(管理员用户使用)
};

struct msg_insert
{
    struct msg msg;         //SER_INSERT
    uint32_t id;            //操作用户ID
    enum usertype usertype; //操作用户用户类型

    struct userinfo userinfo;
};

struct msg_change           //数据修改网络数据包格式
{
    struct msg msg;         //CLI_CHANGE
    uint32_t id;            //操作用户ID
    enum usertype usertype; //操作用户用户类型

    uint32_t chid;          //被修改用户ID
    enum entry key;         //被修改项
    char value[128];        //修改值(以字符串形式)
};

/***********************************************************/
/*********************服务器端数据包协议*********************/

struct msg_ack
{
    struct msg msg;         //SER_SUCCESS, SER_ERR
    enum usertype usertype; //操作用户用户类型
    enum errtpye err;       //错误码，在SER_ERR下有效
};

struct msg_userinfo         //用户信息查询网络数据包格式
{
    struct msg msg;         //SER_ACK
    struct userinfo userinfo;
};

struct msg_historylog       //历史记录查询网络数据包格式
{
    struct msg msg;         //SER_ACK
    struct historylog log;
};
/***********************************************************/
#endif